from collections import namedtuple
import re


NAME_RE = re.compile(r'^([a-zA-Z]|_\w*[a-zA-Z]\w*|[a-zA-Z]\w*)$')


class StaticVar(namedtuple('StaticVar', 'filename funcname name vartype')):
    """Information about a single static variable."""

    __slots__ = ()

    @classmethod
    def normalize_vartype(cls, vartype):
        if vartype is None:
            return None

        # XXX finish!
        return str(vartype)

    @classmethod
    def _make(cls, iterable):  # The default _make() is not subclass-friendly.
        return cls.__new__(cls, *iterable)

    def __new__(cls, filename, funcname, name, vartype):
        self = super().__new__(
                cls,
                filename=str(filename) if filename else None,
                funcname=str(funcname) if funcname else None,
                name=str(name) if name else None,
                vartype=cls.normalize_vartype(vartype) if vartype else None,
                )
        return self

    # XXX Always validate?
    #def __init__(self, *args, **kwargs):
    #    self.validate()

    # XXX The default __repr__() is not subclass-friendly (where the name changes).
    #def __repr__(self):
    #    _, _, sig = super().__repr__().partition('(')
    #    return f'{self.__class__.__name__}({sig}'

    def validate(self):
        """Fail if the StaticVar is invalid (i.e. init with bad data)."""
        for field in self._fields:
            value = getattr(self, field)
            if value is None:
                if field == 'funcname':  # The field can be missing (net set).
                    continue
                raise TypeError(f'missing {field}')
            elif field not in ('filename', 'vartype'):
                if not NAME_RE.match(value):
                    raise ValueError(f'{field} must be a name, got {value!r}')

    # XXX Always validate?
    #def _replace(self, **kwargs):
    #    obj = super()._replace(**kwargs)
    #    obj.validate()
    #    return obj
