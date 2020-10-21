from collections import namedtuple

from c_analyzer.common.info import ID
from c_analyzer.common.util import classonly, _NTBase


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

    def __hash__(self):
        return hash(self.id)

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
        elif self.kind not in vars(self.KIND).values():
            raise ValueError(f'unsupported kind {self.kind}')

        if self.external is None:
            raise TypeError('missing external')
