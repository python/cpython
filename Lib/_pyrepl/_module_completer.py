from __future__ import annotations

import pkgutil
import sys
import token
import tokenize
from io import StringIO
from contextlib import contextmanager
from dataclasses import dataclass
from itertools import chain
from tokenize import TokenInfo

TYPE_CHECKING = False

if TYPE_CHECKING:
    from typing import Any, Iterable, Iterator, Mapping


def make_default_module_completer() -> ModuleCompleter:
    # Inside pyrepl, __package__ is set to None by default
    return ModuleCompleter(namespace={'__package__': None})


class ModuleCompleter:
    """A completer for Python import statements.

    Examples:
        - import <tab>
        - import foo<tab>
        - import foo.<tab>
        - import foo as bar, baz<tab>

        - from <tab>
        - from foo<tab>
        - from foo import <tab>
        - from foo import bar<tab>
        - from foo import (bar as baz, qux<tab>
    """

    def __init__(self, namespace: Mapping[str, Any] | None = None) -> None:
        self.namespace = namespace or {}
        self._global_cache: list[pkgutil.ModuleInfo] = []
        self._curr_sys_path: list[str] = sys.path[:]

    def get_completions(self, line: str) -> list[str] | None:
        """Return the next possible import completions for 'line'."""
        result = ImportParser(line).parse()
        if not result:
            return None
        try:
            return self.complete(*result)
        except Exception:
            # Some unexpected error occurred, make it look like
            # no completions are available
            return []

    def complete(self, from_name: str | None, name: str | None) -> list[str]:
        if from_name is None:
            # import x.y.z<tab>
            assert name is not None
            path, prefix = self.get_path_and_prefix(name)
            modules = self.find_modules(path, prefix)
            return [self.format_completion(path, module) for module in modules]

        if name is None:
            # from x.y.z<tab>
            path, prefix = self.get_path_and_prefix(from_name)
            modules = self.find_modules(path, prefix)
            return [self.format_completion(path, module) for module in modules]

        # from x.y import z<tab>
        return self.find_modules(from_name, name)

    def find_modules(self, path: str, prefix: str) -> list[str]:
        """Find all modules under 'path' that start with 'prefix'."""
        modules = self._find_modules(path, prefix)
        # Filter out invalid module names
        # (for example those containing dashes that cannot be imported with 'import')
        return [mod for mod in modules if mod.isidentifier()]

    def _find_modules(self, path: str, prefix: str) -> list[str]:
        if not path:
            # Top-level import (e.g. `import foo<tab>`` or `from foo<tab>`)`
            builtin_modules = [name for name in sys.builtin_module_names
                               if self.is_suggestion_match(name, prefix)]
            third_party_modules = [module.name for module in self.global_cache
                                   if self.is_suggestion_match(module.name, prefix)]
            return sorted(builtin_modules + third_party_modules)

        if path.startswith('.'):
            # Convert relative path to absolute path
            package = self.namespace.get('__package__', '')
            path = self.resolve_relative_name(path, package)  # type: ignore[assignment]
            if path is None:
                return []

        modules: Iterable[pkgutil.ModuleInfo] = self.global_cache
        for segment in path.split('.'):
            modules = [mod_info for mod_info in modules
                       if mod_info.ispkg and mod_info.name == segment]
            modules = self.iter_submodules(modules)
        return [module.name for module in modules
                if self.is_suggestion_match(module.name, prefix)]

    def is_suggestion_match(self, module_name: str, prefix: str) -> bool:
        if prefix:
            return module_name.startswith(prefix)
        # For consistency with attribute completion, which
        # does not suggest private attributes unless requested.
        return not module_name.startswith("_")

    def iter_submodules(self, parent_modules: list[pkgutil.ModuleInfo]) -> Iterator[pkgutil.ModuleInfo]:
        """Iterate over all submodules of the given parent modules."""
        specs = [info.module_finder.find_spec(info.name, None)
                 for info in parent_modules if info.ispkg]
        search_locations = set(chain.from_iterable(
            getattr(spec, 'submodule_search_locations', [])
            for spec in specs if spec
        ))
        return pkgutil.iter_modules(search_locations)

    def get_path_and_prefix(self, dotted_name: str) -> tuple[str, str]:
        """
        Split a dotted name into an import path and a
        final prefix that is to be completed.

        Examples:
            'foo.bar' -> 'foo', 'bar'
            'foo.' -> 'foo', ''
            '.foo' -> '.', 'foo'
        """
        if '.' not in dotted_name:
            return '', dotted_name
        if dotted_name.startswith('.'):
            stripped = dotted_name.lstrip('.')
            dots = '.' * (len(dotted_name) - len(stripped))
            if '.' not in stripped:
                return dots, stripped
            path, prefix = stripped.rsplit('.', 1)
            return dots + path, prefix
        path, prefix = dotted_name.rsplit('.', 1)
        return path, prefix

    def format_completion(self, path: str, module: str) -> str:
        if path == '' or path.endswith('.'):
            return f'{path}{module}'
        return f'{path}.{module}'

    def resolve_relative_name(self, name: str, package: str) -> str | None:
        """Resolve a relative module name to an absolute name.

        Example: resolve_relative_name('.foo', 'bar') -> 'bar.foo'
        """
        # taken from importlib._bootstrap
        level = 0
        for character in name:
            if character != '.':
                break
            level += 1
        bits = package.rsplit('.', level - 1)
        if len(bits) < level:
            return None
        base = bits[0]
        name = name[level:]
        return f'{base}.{name}' if name else base

    @property
    def global_cache(self) -> list[pkgutil.ModuleInfo]:
        """Global module cache"""
        if not self._global_cache or self._curr_sys_path != sys.path:
            self._curr_sys_path = sys.path[:]
            # print('getting packages')
            self._global_cache = list(pkgutil.iter_modules())
        return self._global_cache


class ImportParser:
    """
    Parses incomplete import statements that are
    suitable for autocomplete suggestions.

    Examples:
        - import foo          -> Result(from_name=None, name='foo')
        - import foo.         -> Result(from_name=None, name='foo.')
        - from foo            -> Result(from_name='foo', name=None)
        - from foo import bar -> Result(from_name='foo', name='bar')
        - from .foo import (  -> Result(from_name='.foo', name='')

    Note that the parser works in reverse order, starting from the
    last token in the input string. This makes the parser more robust
    when parsing multiple statements.
    """
    _ignored_tokens = {
        token.INDENT, token.DEDENT, token.COMMENT,
        token.NL, token.NEWLINE, token.ENDMARKER
    }
    _keywords = {'import', 'from', 'as'}

    def __init__(self, code: str) -> None:
        self.code = code
        tokens = []
        try:
            for t in tokenize.generate_tokens(StringIO(code).readline):
                if t.type not in self._ignored_tokens:
                    tokens.append(t)
        except tokenize.TokenError as e:
            if 'unexpected EOF' not in str(e):
                # unexpected EOF is fine, since we're parsing an
                # incomplete statement, but other errors are not
                # because we may not have all the tokens so it's
                # safer to bail out
                tokens = []
        except SyntaxError:
            tokens = []
        self.tokens = TokenQueue(tokens[::-1])

    def parse(self) -> tuple[str | None, str | None] | None:
        if not (res := self._parse()):
            return None
        return res.from_name, res.name

    def _parse(self) -> Result | None:
        with self.tokens.save_state():
            return self.parse_from_import()
        with self.tokens.save_state():
            return self.parse_import()

    def parse_import(self) -> Result:
        if self.code.rstrip().endswith('import') and self.code.endswith(' '):
            return Result(name='')
        if self.tokens.peek_string(','):
            name = ''
        else:
            if self.code.endswith(' '):
                raise ParseError('parse_import')
            name = self.parse_dotted_name()
        if name.startswith('.'):
            raise ParseError('parse_import')
        while self.tokens.peek_string(','):
            self.tokens.pop()
            self.parse_dotted_as_name()
        if self.tokens.peek_string('import'):
            return Result(name=name)
        raise ParseError('parse_import')

    def parse_from_import(self) -> Result:
        stripped = self.code.rstrip()
        if stripped.endswith('import') and self.code.endswith(' '):
            return Result(from_name=self.parse_empty_from_import(), name='')
        if stripped.endswith('from') and self.code.endswith(' '):
            return Result(from_name='')
        if self.tokens.peek_string('(') or self.tokens.peek_string(','):
            return Result(from_name=self.parse_empty_from_import(), name='')
        if self.code.endswith(' '):
            raise ParseError('parse_from_import')
        name = self.parse_dotted_name()
        if '.' in name:
            self.tokens.pop_string('from')
            return Result(from_name=name)
        if self.tokens.peek_string('from'):
            return Result(from_name=name)
        from_name = self.parse_empty_from_import()
        return Result(from_name=from_name, name=name)

    def parse_empty_from_import(self) -> str:
        if self.tokens.peek_string(','):
            self.tokens.pop()
            self.parse_as_names()
        if self.tokens.peek_string('('):
            self.tokens.pop()
        self.tokens.pop_string('import')
        return self.parse_from()

    def parse_from(self) -> str:
        from_name = self.parse_dotted_name()
        self.tokens.pop_string('from')
        return from_name

    def parse_dotted_as_name(self) -> str:
        self.tokens.pop_name()
        if self.tokens.peek_string('as'):
            self.tokens.pop()
        with self.tokens.save_state():
            return self.parse_dotted_name()

    def parse_dotted_name(self) -> str:
        name = []
        if self.tokens.peek_string('.'):
            name.append('.')
            self.tokens.pop()
        if (self.tokens.peek_name()
            and (tok := self.tokens.peek())
            and tok.string not in self._keywords):
            name.append(self.tokens.pop_name())
        if not name:
            raise ParseError('parse_dotted_name')
        while self.tokens.peek_string('.'):
            name.append('.')
            self.tokens.pop()
            if (self.tokens.peek_name()
                and (tok := self.tokens.peek())
                and tok.string not in self._keywords):
                name.append(self.tokens.pop_name())
            else:
                break

        while self.tokens.peek_string('.'):
            name.append('.')
            self.tokens.pop()
        return ''.join(name[::-1])

    def parse_as_names(self) -> None:
        self.parse_as_name()
        while self.tokens.peek_string(','):
            self.tokens.pop()
            self.parse_as_name()

    def parse_as_name(self) -> None:
        self.tokens.pop_name()
        if self.tokens.peek_string('as'):
            self.tokens.pop()
            self.tokens.pop_name()


class ParseError(Exception):
    pass


@dataclass(frozen=True)
class Result:
    from_name: str | None = None
    name: str | None = None


class TokenQueue:
    """Provides helper functions for working with a sequence of tokens."""

    def __init__(self, tokens: list[TokenInfo]) -> None:
        self.tokens: list[TokenInfo] = tokens
        self.index: int = 0
        self.stack: list[int] = []

    @contextmanager
    def save_state(self) -> Any:
        try:
            self.stack.append(self.index)
            yield
        except ParseError:
            self.index = self.stack.pop()
        else:
            self.stack.pop()

    def __bool__(self) -> bool:
        return self.index < len(self.tokens)

    def peek(self) -> TokenInfo | None:
        if not self:
            return None
        return self.tokens[self.index]

    def peek_name(self) -> bool:
        if not (tok := self.peek()):
            return False
        return tok.type == token.NAME

    def pop_name(self) -> str:
        tok = self.pop()
        if tok.type != token.NAME:
            raise ParseError('pop_name')
        return tok.string

    def peek_string(self, string: str) -> bool:
        if not (tok := self.peek()):
            return False
        return tok.string == string

    def pop_string(self, string: str) -> str:
        tok = self.pop()
        if tok.string != string:
            raise ParseError('pop_string')
        return tok.string

    def pop(self) -> TokenInfo:
        if not self:
            raise ParseError('pop')
        tok = self.tokens[self.index]
        self.index += 1
        return tok
