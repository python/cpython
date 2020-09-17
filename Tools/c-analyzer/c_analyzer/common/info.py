from collections import namedtuple
import re

from .util import classonly, _NTBase

# XXX need tests:
# * ID.match()


UNKNOWN = '???'

# Does not start with digit and contains at least one letter.
NAME_RE = re.compile(r'(?!\d)(?=.*?[A-Za-z])\w+', re.ASCII)


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
        if not NAME_RE.fullmatch(self.name):
            raise ValueError(
                    f'name must be an identifier, got {self.name!r}')

        # Symbols from a binary might not have filename/funcname info.

        if self.funcname:
            if not self.filename:
                raise TypeError('missing filename')
            if not NAME_RE.fullmatch(self.funcname) and self.funcname != UNKNOWN:
                raise ValueError(
                        f'name must be an identifier, got {self.funcname!r}')

        # XXX Require the filename (at least UNKONWN)?
        # XXX Check the filename?

    @property
    def islocal(self):
        return self.funcname is not None

    def match(self, other, *,
              match_files=(lambda f1, f2: f1 == f2),
              ):
        """Return True if the two match.

        At least one of the two must be completely valid (no UNKNOWN
        anywhere).  Otherwise False is returned.  The remaining one
        *may* have UNKNOWN for both funcname and filename.  It must
        have a valid name though.

        The caller is responsible for knowing which of the two is valid
        (and which to use if both are valid).
        """
        # First check the name.
        if self.name is None:
            return False
        if other.name != self.name:
            return False

        # Then check the filename.
        if self.filename is None:
            return False
        if other.filename is None:
            return False
        if self.filename == UNKNOWN:
            # "other" must be the valid one.
            if other.funcname == UNKNOWN:
                return False
            elif self.funcname != UNKNOWN:
                # XXX Try matching funcname even though we don't
                # know the filename?
                raise NotImplementedError
            else:
                return True
        elif other.filename == UNKNOWN:
            # "self" must be the valid one.
            if self.funcname == UNKNOWN:
                return False
            elif other.funcname != UNKNOWN:
                # XXX Try matching funcname even though we don't
                # know the filename?
                raise NotImplementedError
            else:
                return True
        elif not match_files(self.filename, other.filename):
            return False

        # Finally, check the funcname.
        if self.funcname == UNKNOWN:
            # "other" must be the valid one.
            if other.funcname == UNKNOWN:
                return False
            else:
                return other.funcname is not None
        elif other.funcname == UNKNOWN:
            # "self" must be the valid one.
            if self.funcname == UNKNOWN:
                return False
            else:
                return self.funcname is not None
        elif self.funcname == other.funcname:
            # Both are valid.
            return True

        return False
