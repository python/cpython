from collections import namedtuple
import os.path

from c_common import fsutil
from c_common.clsutil import classonly
import c_common.misc as _misc
from c_parser.info import (
    KIND,
    HighlevelParsedItem,
    Declaration,
    TypeDeclaration,
)
from c_parser.match import (
    is_type_decl,
)
from .match import (
    is_process_global,
)


IGNORED = _misc.Labeled('IGNORED')
UNKNOWN = _misc.Labeled('UNKNOWN')


class SystemType(TypeDeclaration):

    def __init__(self, name):
        super().__init__(None, name, None, None, _shortkey=name)


class Analyzed:
    _locked = False

    @classonly
    def is_target(cls, raw):
        if isinstance(raw, HighlevelParsedItem):
            return True
        else:
            return False

    @classonly
    def from_raw(cls, raw, **extra):
        if isinstance(raw, cls):
            if extra:
                # XXX ?
                raise NotImplementedError((raw, extra))
                #return cls(raw.item, raw.typedecl, **raw._extra, **extra)
            else:
                return info
        elif cls.is_target(raw):
            return cls(raw, **extra)
        else:
            raise NotImplementedError((raw, extra))

    @classonly
    def from_resolved(cls, item, resolved, **extra):
        if isinstance(resolved, TypeDeclaration):
            return cls(item, typedecl=resolved, **extra)
        else:
            typedeps, extra = cls._parse_raw_resolved(item, resolved, extra)
            if item.kind is KIND.ENUM:
                if typedeps:
                    raise NotImplementedError((item, resolved, extra))
            elif not typedeps:
                raise NotImplementedError((item, resolved, extra))
            return cls(item, typedeps, **extra or {})

    @classonly
    def _parse_raw_resolved(cls, item, resolved, extra_extra):
        if resolved in (UNKNOWN, IGNORED):
            return resolved, None
        try:
            typedeps, extra = resolved
        except (TypeError, ValueError):
            typedeps = extra = None
        if extra:
            # The resolved data takes precedence.
            extra = dict(extra_extra, **extra)
        if isinstance(typedeps, TypeDeclaration):
            return typedeps, extra
        elif typedeps in (None, UNKNOWN):
            # It is still effectively unresolved.
            return UNKNOWN, extra
        elif None in typedeps or UNKNOWN in typedeps:
            # It is still effectively unresolved.
            return typedeps, extra
        elif any(not isinstance(td, TypeDeclaration) for td in typedeps):
            raise NotImplementedError((item, typedeps, extra))
        return typedeps, extra

    def __init__(self, item, typedecl=None, **extra):
        assert item is not None
        self.item = item
        if typedecl in (UNKNOWN, IGNORED):
            pass
        elif item.kind is KIND.STRUCT or item.kind is KIND.UNION:
            if isinstance(typedecl, TypeDeclaration):
                raise NotImplementedError(item, typedecl)
            elif typedecl is None:
                typedecl = UNKNOWN
            else:
                typedecl = [UNKNOWN if d is None else d for d in typedecl]
        elif typedecl is None:
            typedecl = UNKNOWN
        elif typedecl and not isinstance(typedecl, TypeDeclaration):
            # All the other decls have a single type decl.
            typedecl, = typedecl
            if typedecl is None:
                typedecl = UNKNOWN
        self.typedecl = typedecl
        self._extra = extra
        self._locked = True

        self._validate()

    def _validate(self):
        item = self.item
        extra = self._extra
        # Check item.
        if not isinstance(item, HighlevelParsedItem):
            raise ValueError(f'"item" must be a high-level parsed item, got {item!r}')
        # Check extra.
        for key, value in extra.items():
            if key.startswith('_'):
                raise ValueError(f'extra items starting with {"_"!r} not allowed, got {extra!r}')
            if hasattr(item, key) and not callable(getattr(item, key)):
                raise ValueError(f'extra cannot override item, got {value!r} for key {key!r}')

    def __repr__(self):
        kwargs = [
            f'item={self.item!r}',
            f'typedecl={self.typedecl!r}',
            *(f'{k}={v!r}' for k, v in self._extra.items())
        ]
        return f'{type(self).__name__}({", ".join(kwargs)})'

    def __str__(self):
        try:
            return self._str
        except AttributeError:
            self._str, = self.render('line')
            return self._str

    def __hash__(self):
        return hash(self.item)

    def __eq__(self, other):
        if isinstance(other, Analyzed):
            return self.item == other.item
        elif isinstance(other, HighlevelParsedItem):
            return self.item == other
        elif type(other) is tuple:
            return self.item == other
        else:
            return NotImplemented

    def __gt__(self, other):
        if isinstance(other, Analyzed):
            return self.item > other.item
        elif isinstance(other, HighlevelParsedItem):
            return self.item > other
        elif type(other) is tuple:
            return self.item > other
        else:
            return NotImplemented

    def __dir__(self):
        names = set(super().__dir__())
        names.update(self._extra)
        names.remove('_locked')
        return sorted(names)

    def __getattr__(self, name):
        if name.startswith('_'):
            raise AttributeError(name)
        # The item takes precedence over the extra data (except if callable).
        try:
            value = getattr(self.item, name)
            if callable(value):
                raise AttributeError(name)
        except AttributeError:
            try:
                value = self._extra[name]
            except KeyError:
                pass
            else:
                # Speed things up the next time.
                self.__dict__[name] = value
                return value
            raise  # re-raise
        else:
            return value

    def __setattr__(self, name, value):
        if self._locked and name != '_str':
            raise AttributeError(f'readonly ({name})')
        super().__setattr__(name, value)

    def __delattr__(self, name):
        if self._locked:
            raise AttributeError(f'readonly ({name})')
        super().__delattr__(name)

    @property
    def decl(self):
        if not isinstance(self.item, Declaration):
            raise AttributeError('decl')
        return self.item

    @property
    def signature(self):
        # XXX vartype...
        ...

    @property
    def istype(self):
        return is_type_decl(self.item.kind)

    @property
    def is_known(self):
        if self.typedecl in (UNKNOWN, IGNORED):
            return False
        elif isinstance(self.typedecl, TypeDeclaration):
            return True
        else:
            return UNKNOWN not in self.typedecl

    def fix_filename(self, relroot=fsutil.USE_CWD, **kwargs):
        self.item.fix_filename(relroot, **kwargs)
        return self

    def as_rowdata(self, columns=None):
        # XXX finish!
        return self.item.as_rowdata(columns)

    def render_rowdata(self, columns=None):
        # XXX finish!
        return self.item.render_rowdata(columns)

    def render(self, fmt='line', *, itemonly=False):
        if fmt == 'raw':
            yield repr(self)
            return
        rendered = self.item.render(fmt)
        if itemonly or not self._extra:
            yield from rendered
            return
        extra = self._render_extra(fmt)
        if not extra:
            yield from rendered
        elif fmt in ('brief', 'line'):
            rendered, = rendered
            extra, = extra
            yield f'{rendered}\t{extra}'
        elif fmt == 'summary':
            raise NotImplementedError(fmt)
        elif fmt == 'full':
            yield from rendered
            for line in extra:
                yield f'\t{line}'
        else:
            raise NotImplementedError(fmt)

    def _render_extra(self, fmt):
        if fmt in ('brief', 'line'):
            yield str(self._extra)
        else:
            raise NotImplementedError(fmt)


class Analysis:

    _item_class = Analyzed

    @classonly
    def build_item(cls, info, resolved=None, **extra):
        if resolved is None:
            return cls._item_class.from_raw(info, **extra)
        else:
            return cls._item_class.from_resolved(info, resolved, **extra)

    @classmethod
    def from_results(cls, results):
        self = cls()
        for info, resolved in results:
            self._add_result(info, resolved)
        return self

    def __init__(self, items=None):
        self._analyzed = {type(self).build_item(item): None
                          for item in items or ()}

    def __repr__(self):
        return f'{type(self).__name__}({list(self._analyzed.keys())})'

    def __iter__(self):
        #yield from self.types
        #yield from self.functions
        #yield from self.variables
        yield from self._analyzed

    def __len__(self):
        return len(self._analyzed)

    def __getitem__(self, key):
        if type(key) is int:
            for i, val in enumerate(self._analyzed):
                if i == key:
                    return val
            else:
                raise IndexError(key)
        else:
            return self._analyzed[key]

    def fix_filenames(self, relroot=fsutil.USE_CWD, **kwargs):
        if relroot and relroot is not fsutil.USE_CWD:
            relroot = os.path.abspath(relroot)
        for item in self._analyzed:
            item.fix_filename(relroot, fixroot=False, **kwargs)

    def _add_result(self, info, resolved):
        analyzed = type(self).build_item(info, resolved)
        self._analyzed[analyzed] = None
        return analyzed
