from collections import namedtuple
import re

from .util import classonly, _NTBase


UNKNOWN = '???'

NAME_RE = re.compile(r'^([a-zA-Z]|_\w*[a-zA-Z]\w*|[a-zA-Z]\w*)$')


class ID(_NTBase, namedtuple('ID', 'filename funcname name')):
    """A unique ID for a single symbol or declaration."""

    __slots__ = ()
    # XXX Add optional conditions (tuple of strings) field.
    #conditions = Slot()

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
        #cls.conditions.set(self, tuple(str(s) if s else None
        #                               for s in conditions or ()))
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
            if not NAME_RE.match(self.funcname) and self.funcname != UNKNOWN:
                raise ValueError(
                        f'name must be an identifier, got {self.funcname!r}')

        # XXX Require the filename (at least UNKONWN)?
        # XXX Check the filename?

    @property
    def islocal(self):
        return self.funcname is not None


#############################
# variables

def normalize_vartype(vartype):
    """Return the canonical form for a variable type (or func signature)."""
    # We allow empty strring through for semantic reasons.
    if vartype is None:
        return None

    # XXX finish!
    # XXX Return (modifiers, type, pointer)?
    return str(vartype)


def extract_storage(decl, *, isfunc=False):
    """Return (storage, vartype) based on the given declaration.

    The default storage is "implicit" or "local".
    """
    if decl == UNKNOWN:
        return decl, decl
    if decl.startswith('static '):
        return 'static', decl
        #return 'static', decl.partition(' ')[2].strip()
    elif decl.startswith('extern '):
        return 'extern', decl
        #return 'extern', decl.partition(' ')[2].strip()
    elif re.match('.*\b(static|extern)\b', decl):
        raise NotImplementedError
    elif isfunc:
        return 'local', decl
    else:
        return 'implicit', decl


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
        if storage is None:
            storage, decl = extract_storage(decl, isfunc=funcname)
        id = ID(filename, funcname, name)
        self = cls(id, storage, decl)
        return self

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
