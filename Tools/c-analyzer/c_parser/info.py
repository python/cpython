from collections import namedtuple

from c_analyzer_common.info import UNKNOWN, NAME_RE, ID
from c_analyzer_common.util import classonly, _NTBase


def normalize_vartype(vartype):
    """Return the canonical form for a variable type (or func signature)."""
    # We allow empty strring through for semantic reasons.
    if vartype is None:
        return None

    # XXX finish!
    # XXX Return (modifiers, type, pointer)?
    return str(vartype)


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

        if not self.filename:
            raise TypeError('id missing filename')

        if self.vartype is None:
            raise TypeError('missing vartype')

    @property
    def isstatic(self):
        return 'static' in self.vartype.split()

    @property
    def isconst(self):
        return 'const' in self.vartype.split()
