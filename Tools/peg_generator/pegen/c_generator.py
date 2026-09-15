"""Prepare a complete C parser description and emit it."""

import os.path
from typing import IO

from pegen import grammar
from pegen.c_generator_calls import (
    CCallMakerVisitor as CCallMakerVisitor,
)
from pegen.c_generator_calls import (
    FunctionCall as FunctionCall,
)
from pegen.c_generator_calls import (
    NodeTypes as NodeTypes,
)
from pegen.c_generator_file import CParserEmitter
from pegen.c_generator_model import CParser, CRule, CRuleSignature
from pegen.parser_generator import ParserGenerator

EXTENSION_PREFIX = """\
#include "pegen.h"
#include "pycore_ceval.h"

#if defined(Py_DEBUG) && defined(Py_BUILD_CORE)
#  define D(x) if (p->debug) { x; }
#else
#  define D(x)
#endif

#ifdef __wasi__
#  ifdef Py_DEBUG
#    define MAXSTACK 1000
#  else
#    define MAXSTACK 4000
#  endif
#else
#  define MAXSTACK 6000
#endif

"""


EXTENSION_SUFFIX = """
void *
_PyPegen_parse(Parser *p)
{
    // Initialize keywords
    p->keywords = reserved_keywords;
    p->n_keyword_lists = n_keyword_lists;
    p->soft_keywords = soft_keywords;

    return start_rule(p);
}
"""


class CParserGenerator(ParserGenerator):
    def __init__(
        self,
        grammar: grammar.Grammar,
        tokens: dict[int, str],
        exact_tokens: dict[str, int],
        non_exact_tokens: set[str],
        file: IO[str] | None,
        debug: bool = False,
        skip_actions: bool = False,
    ):
        super().__init__(grammar, set(tokens.values()), file)
        self.callmakervisitor: CCallMakerVisitor = CCallMakerVisitor(
            self, exact_tokens, non_exact_tokens
        )
        self._collected = False
        self.debug = debug
        self.skip_actions = skip_actions

    def generate(self, filename: str) -> None:
        parser = self.prepare(filename)
        CParserEmitter(parser, self.file).emit()

    def prepare(self, filename: str) -> CParser:
        self.collect_rules()
        lowerer = self.callmakervisitor.make_lowerer()
        rules = tuple(
            lowerer.prepare_rule(rule, skip_actions=self.skip_actions)
            for rule in self.all_rules.values()
        )
        headers = []
        if header := self.grammar.metas.get("header", EXTENSION_PREFIX):
            headers.append(header.rstrip("\n"))
        if subheader := self.grammar.metas.get("subheader", ""):
            headers.append(subheader)
        return CParser(
            source_name=os.path.basename(filename),
            headers=tuple(headers),
            keyword_groups=self._prepare_keywords(),
            soft_keywords=tuple(sorted(self.soft_keywords)),
            rules=rules,
            trailer=self._prepare_trailer(rules),
            debug=self.debug,
        )

    def collect_rules(self) -> None:
        # Keyword generation also uses this entry point without emitting C.
        if not self._collected:
            super().collect_rules()
            self._collected = True

    def _prepare_keywords(self) -> tuple[tuple[tuple[str, int], ...], ...]:
        if not self.keywords:
            return ()
        groups: list[list[tuple[str, int]]] = [
            [] for _ in range(max(map(len, self.keywords)) + 1)
        ]
        for keyword, token_type in self.keywords.items():
            groups[len(keyword)].append((keyword, token_type))
        return tuple(tuple(group) for group in groups)

    def _prepare_trailer(self, rules: tuple[CRule, ...]) -> str | None:
        if self.skip_actions:
            mode = 0
        else:
            start = next((rule.signature for rule in rules if rule.signature.name == "start"), None)
            match start:
                case None | CRuleSignature(return_type="mod_ty"):
                    mode = 2 if self.grammar.metas.get("bytecode") else 1
                case _:
                    mode = 0
        modulename = self.grammar.metas.get("modulename", "parse")
        if trailer := self.grammar.metas.get("trailer", EXTENSION_SUFFIX):
            return trailer.rstrip("\n") % dict(mode=mode, modulename=modulename)
        return None
