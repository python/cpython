from collections import namedtuple

from c_analyzer_common.info import ID
from c_analyzer_common.util import classonly, _NTBase


def normalize_vartype(vartype):
    """Return the canonical form for a variable type (or func signature)."""
    # We allow empty strring through for semantic reasons.
    if vartype is None:
        return None

    # XXX finish!
    # XXX Return (modifiers, type, pointer)?
    return str(vartype)


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
