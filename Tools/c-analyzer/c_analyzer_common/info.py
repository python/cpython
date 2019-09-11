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
