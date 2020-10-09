from collections import namedtuple

from ..common.info import ID, UNKNOWN
from ..common.util import classonly, _NTBase


def normalize_vartype(vartype):
    """Return the canonical form for a variable type (or func signature)."""
    # We allow empty strring through for semantic reasons.
    if vartype is None:
        return None

    # XXX finish!
    # XXX Return (modifiers, type, pointer)?
    return str(vartype)


# XXX Variable.vartype -> decl (Declaration).

class Variable(_NTBase,
               namedtuple('Variable', 'id storage vartype')):
    """Information about a single variable declaration."""

    __slots__ = ()

    STORAGE = (
            'static',
            'extern',
            'implicit',
            'local',
            )

    @classonly
    def from_parts(cls, filename, funcname, name, decl, storage=None):
        varid = ID(filename, funcname, name)
        if storage is None:
            self = cls.from_id(varid, decl)
        else:
            self = cls(varid, storage, decl)
        return self

    @classonly
    def from_id(cls, varid, decl):
        from ..parser.declarations import extract_storage
        storage = extract_storage(decl, infunc=varid.funcname)
        return cls(varid, storage, decl)

    def __new__(cls, id, storage, vartype):
        self = super().__new__(
                cls,
                id=ID.from_raw(id),
                storage=str(storage) if storage else None,
                vartype=normalize_vartype(vartype) if vartype else None,
                )
        return self

    def __hash__(self):
        return hash(self.id)

    def __getattr__(self, name):
        return getattr(self.id, name)

    def _validate_id(self):
        if not self.id:
            raise TypeError('missing id')

        if not self.filename or self.filename == UNKNOWN:
            raise TypeError(f'id missing filename ({self.id})')

        if self.funcname and self.funcname == UNKNOWN:
            raise TypeError(f'id missing funcname ({self.id})')

        self.id.validate()

    def validate(self):
        """Fail if the object is invalid (i.e. init with bad data)."""
        self._validate_id()

        if self.storage is None or self.storage == UNKNOWN:
            raise TypeError('missing storage')
        elif self.storage not in self.STORAGE:
            raise ValueError(f'unsupported storage {self.storage:r}')

        if self.vartype is None or self.vartype == UNKNOWN:
            raise TypeError('missing vartype')

    @property
    def isglobal(self):
        return self.storage != 'local'

    @property
    def isconst(self):
        return 'const' in self.vartype.split()
