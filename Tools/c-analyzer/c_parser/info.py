from collections import namedtuple
import re

from .util import classonly


NAME_RE = re.compile(r'^([a-zA-Z]|_\w*[a-zA-Z]\w*|[a-zA-Z]\w*)$')


def normalize_vartype(vartype):
    """Return the canonical form for a variable type (or func signature)."""
    # We allow empty strring through for semantic reasons.
    if vartype is None:
        return None

    # XXX finish!
    # XXX Return (modifiers, type, pointer)?
    return str(vartype)


class _NTBase:

    __slots__ = ()

    @classonly
    def from_raw(cls, raw):
        if not raw:
            return None
        elif isinstance(raw, cls):
            return raw
        elif isinstance(raw, str):
            return cls.from_string(raw)
        else:
            if hasattr(raw, 'items'):
                return cls(**raw)
            try:
                args = tuple(raw)
            except TypeError:
                pass
            else:
                return cls(*args)
        raise NotImplementedError

    @classonly
    def from_string(cls, value):
        """Return a new instance based on the given string."""
        raise NotImplementedError

    @classmethod
    def _make(cls, iterable):  # The default _make() is not subclass-friendly.
        return cls.__new__(cls, *iterable)

    # XXX Always validate?
    #def __init__(self, *args, **kwargs):
    #    self.validate()

    # XXX The default __repr__() is not subclass-friendly (where the name changes).
    #def __repr__(self):
    #    _, _, sig = super().__repr__().partition('(')
    #    return f'{self.__class__.__name__}({sig}'

    # To make sorting work with None:
    def __lt__(self, other):
        try:
            return super().__lt__(other)
        except TypeError:
            if None in self:
                return True
            elif None in other:
                return False
            else:
                raise

    def validate(self):
        return

    # XXX Always validate?
    #def _replace(self, **kwargs):
    #    obj = super()._replace(**kwargs)
    #    obj.validate()
    #    return obj


class ID(_NTBase, namedtuple('ID', 'filename funcname name')):
    """A unique ID for a single symbol or declaration."""

    # XXX Add optional conditions (tuple of strings) field.

    __slots__ = ()

    @classonly
    def from_raw(cls, raw):
        if not raw:
            return None
        if isinstance(raw, str):
            return cls(None, None, raw)
        try:
            name, = raw
            filename = None
        except ValueError:
            try:
                filename, name = raw
            except ValueError:
                return super().from_raw(raw)
        return cls(filename, None, name)

    def __new__(cls, filename, funcname, name):
        self = super().__new__(
                cls,
                filename=str(filename) if filename else None,
                funcname=str(funcname) if funcname else None,
                name=str(name) if name else None,
                )
        return self

    def validate(self):
        """Fail if the object is invalid (i.e. init with bad data)."""
        if not self.name:
            raise TypeError('missing name')
        else:
            if not NAME_RE.match(self.name):
                raise ValueError(
                        f'name must be an identifier, got {self.name!r}')

        # Symbols from a binary might not have filename/funcname info.

        if self.funcname:
            if not self.filename:
                raise TypeError('missing filename')
            if not NAME_RE.match(self.funcname):
                raise ValueError(
                        f'name must be an identifier, got {self.funcname!r}')

        # XXX Check the filename?


class Symbol(_NTBase, namedtuple('Symbol', 'id kind external')):
    """Info for a single compilation symbol."""

    __slots__ = ()

    class KIND:
        VARIABLE = 'variable'
        FUNCTION = 'function'
        OTHER = 'other'

    @classonly
    def from_name(cls, name, filename=None, kind=KIND.VARIABLE, external=None):
        """Return a new symbol based on the given name."""
        id = ID(filename, None, name)
        return cls(id, kind, external)

    def __new__(cls, id, kind=KIND.VARIABLE, external=None):
        self = super().__new__(
                cls,
                id=ID.from_raw(id),
                kind=str(kind) if kind else None,
                external=bool(external) if external is not None else None,
                )
        return self

    def __getattr__(self, name):
        return getattr(self.id, name)

    def validate(self):
        """Fail if the object is invalid (i.e. init with bad data)."""
        if not self.id:
            raise TypeError('missing id')
        else:
            self.id.validate()

        if not self.kind:
            raise TypeError('missing kind')
        elif self.kind not in vars(Symbol.KIND).values():
            raise ValueError(f'unsupported kind {self.kind}')

        if self.external is None:
            raise TypeError('missing external')


class Variable(_NTBase,
               namedtuple('Variable', 'id vartype')):
    """Information about a single variable declaration."""

    __slots__ = ()

    @classonly
    def from_parts(cls, filename, funcname, name, vartype):
        id = ID(filename, funcname, name)
        return cls(id, vartype)

    def __new__(cls, id, vartype):
        self = super().__new__(
                cls,
                id=ID.from_raw(id),
                vartype=normalize_vartype(vartype) if vartype else None,
                )
        return self

    def __getattr__(self, name):
        return getattr(self.id, name)

    def validate(self):
        """Fail if the object is invalid (i.e. init with bad data)."""
        if not self.id:
            raise TypeError('missing id')
        else:
            self.id.validate()

        if self.vartype is None:
            raise TypeError('missing vartype')

    @property
    def isstatic(self):
        return 'static' in self.vartype.split()

    @property
    def isconst(self):
        return 'const' in self.vartype.split()
