from collections import namedtuple
import re


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


class Symbol(_NTBase,
             namedtuple('Symbol',
                        'name kind external filename funcname declaration')):
    """Info for a single compilation symbol."""

    __slots__ = ()

    class KIND:
        VARIABLE = 'variable'
        FUNCTION = 'function'
        OTHER = 'other'

    def __new__(cls, name, kind=KIND.VARIABLE, external=None,
                filename=None, funcname=None, declaration=None):
        self = super().__new__(
                cls,
                name=str(name) if name else None,
                kind=str(kind) if kind else None,
                external=bool(external) if external is not None else None,
                filename=str(filename) if filename else None,
                funcname=str(funcname) if funcname else None,
                declaration=normalize_vartype(declaration),
                )
        return self

    def validate(self):
        """Fail if the object is invalid (i.e. init with bad data)."""
        if not self.name:
            raise TypeError('missing name')
        elif not NAME_RE.match(self.name):
            raise ValueError(f'name must be a name, got {self.name!r}')

        if not self.kind:
            raise TypeError('missing kind')
        elif self.kind not in vars(Symbol.KIND).values():
            raise ValueError(f'unsupported kind {self.kind}')

        if self.external is None:
            raise TypeError('missing external')

        if not self.filename and self.funcname:
            raise TypeError('missing filename')
        # filename, funcname and declaration can be not set.

        if not self.funcname:
            # funcname can be not set.
            pass
        elif not NAME_RE.match(self.funcname):
            raise ValueError(f'funcname must be a name, got{self.funcname!r}')

        # declaration can be not set.


class Variable(_NTBase,
               namedtuple('Variable', 'filename funcname name vartype')):
    """Information about a single variable declaration."""

    __slots__ = ()

    def __new__(cls, filename, funcname, name, vartype):
        self = super().__new__(
                cls,
                filename=str(filename) if filename else None,
                funcname=str(funcname) if funcname else None,
                name=str(name) if name else None,
                vartype=normalize_vartype(vartype) if vartype else None,
                )
        return self

    def validate(self):
        """Fail if the object is invalid (i.e. init with bad data)."""
        for field in self._fields:
            value = getattr(self, field)
            if value is None:
                if field == 'funcname':  # The field can be missing (net set).
                    continue
                raise TypeError(f'missing {field}')
            elif field not in ('filename', 'vartype'):
                if not NAME_RE.match(value):
                    raise ValueError(f'{field} must be a name, got {value!r}')
