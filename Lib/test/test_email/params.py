"""Support for Parameterized Tests"""

import collections
from functools import wraps
from string import Formatter


class SafeFormatter(Formatter):

    def format(self, format_string, *args, **kw):
        self.args = args
        self.kw = kw
        return super().format(format_string, *args, **kw)

    def parse(self, format_string):
        for text, varname, spec, conv in super().parse(format_string):
            if varname and varname not in self.kw:
                spec = ':' + spec if spec else ''
                conv = '!' + conv if conv else ''
                text = text + '{' + varname + spec + conv + '}'
                varname, spec, conv = None, None, None
            yield text, varname, spec, conv

safe_format = SafeFormatter().format

def _fmt(fmtfunc, obj, subs):
    if hasattr(obj, 'format'):
        return safe_format(obj, **subs)
    try:
        i = iter(obj)
    except TypeError:
        return obj
    if hasattr(obj, 'items'):
        return type(obj)({k: fmtfunc(v, subs) for k, v in obj.items()})
    return type(obj)(fmtfunc(x, subs) for x in i)

def fmt(obj, subs):
    return _fmt(
        lambda obj, subs:
           safe_format(obj, **subs) if hasattr(obj, 'format') else obj,
        obj,
        subs,
        )

def fmtall(obj, subs):
    return _fmt(fmtall, obj, subs)


class C:

    """Call specification"""

    def __init__(self, *args, **kw):
        """Return object holding a concrete set of arguments for a callable.

        Store any positional arguments as a tuple in self.args, and any
        keyword arguments in as a dict in self.kw.

        """
        self.args = args
        self.kw = kw

    def __call__(self, func):
        """Call func using the concrete arguments from self.args and self.kw"""
        return func(*self.args, **self.kw)

    def __eq__(self, other):
        try:
            return self.args == other.args and self.kw == other.kw
        except AttributeError:
            return False

    def _repr(self, fname):
        args = ', '.join(repr(arg) for arg in self.args)
        kw = ', '.join(f'{k}={repr(v)}' for k, v in self.kw.items())
        return f"{fname}({', '.join(filter(None, (args, kw)))})"

    def __repr__(self):
        return self._repr(type(self).__name__)

    def repr_call(self, func):
        return self._repr(func.__name__)

    def fmt(self, **subs):
        return C(*fmt(self.args, subs), **fmt(self.kw, subs))

    def fmtall(self, **subs):
        return C(*fmtall(self.args, subs), **fmtall(self.kw, subs))


class Params(collections.UserDict):

    def __init__(self, *args, **kw):
        super().__init__()
        self.update(*args, **kw)

    def __repr__(self):
        items = ', '.join(f'{k}={repr(v)}' for k, v in self.items())
        return f'{type(self).__name__}({items})'

    def update(self, *args, **kw):
        for arg in args:
            if not isinstance(arg, self.__class__):
                raise TypeError(
                    f"Invalid argument {arg!r}, arguments"
                    f" must be of type {type(self).__name__},"
                    f" not {type(arg).__name__!r}"
                    )
            super().update(arg)
        super().update(kw)

    def __setitem__(self, name, value):
        if not name.isidentifier():
            raise ValueError(
                f"parameter names must be identifiers, {name!r} is invalid",
                )
        if name in self:
            raise ValueError(
                f"cannot add {name}={value!r}, a callspec with that name"
                f" already exists"
                )
        if not isinstance(value, C):
            value = C(value)
        super().__setitem__(name, value)


debug = False
_tupify = lambda x: x.items() if isinstance(x, Params) else [('', x)]
def _params_map(func, *, with_name=False, with_namelist=False, debug=False):
    if not callable(func):
        raise TypeError(
            f"argument must be callable, not {type(func).__name__!r}"
            )
    if with_name and with_namelist:
        raise ValueError("with_name and with_namelist cannot both be True")
    @wraps(func)
    def params_mapper(*args, **kw):
        if __debug__ and debug: print(f'flattening using {func.__name__}')
        if __debug__ and debug > 1: print(f'{args=} {kw=}')
        param_set = Params()
        for an, av in [t for a in [*args, Params(**kw)] for t in _tupify(a)]:
            if debug: print(f'{an=} {av=}')
            cs = C(*av.args, **av.kw) if isinstance(av, C) else C(av)
            if with_name:
                cs.args = (an, *cs.args)
            elif with_namelist:
                cs.args = (NameList(an), *cs.args)
            try:
                for n, v in cs(func):
                    try:
                        name = '__'.join(filter(None, (an, n)))
                    except Exception:
                        raise ValueError(f"Invalid label: {n!r}") from None
                    if __debug__ and debug: print(f'{n=} {v=} {name=}')
                    if not name:
                        raise ValueError('missing test label')
                    param_set[name] = v
            except Exception as ex:
                val = f'{an}={av!r}' if an else repr(av)
                raise ValueError(
                    f'{func.__name__} failed on {val}',
                    ) from ex
        return param_set
    return params_mapper

def params_map(*args, **kw):
    if not kw:
        return _params_map(*args)
    else:
        def _(func):
            return _params_map(func, *args, **kw)
        return _


def only(*args):
    yield ('', args[0]) if len(args) == 1 else args


@params_map
def as_value(name):
    yield name, C(name)


@params_map(with_name=True)
def with_names(name, *args, **kw):
    yield '', C(name, *args, **kw)


def for_each_name(*names):
    @params_map
    def for_each_name(*args, **kw):
        for name in names:
            yield name, C(name, *args, **kw)
    return for_each_name


def add_label(label):
    @params_map
    def add_label(*args, **kw):
        yield label, C(*args, **kw)
    return add_label


def for_each_function(*functions):
    @params_map
    def for_each_function(*args, **kw):
        for function in functions:
            yield function.__name__, C(function, *args, **kw)
    return for_each_function


# We could factor a common core out of these next two, but the error
# messages when the selection function fails would be more confusing.

def include_if(include, *, label=''):
    @params_map(with_name=True)
    def include_if(name, *args, **kw):
        if include(NameList(name), *args, **kw):
            yield label, C(*args, **kw)
    return include_if

def include_unless(omit, *, label=''):
    @params_map(with_name=True)
    def include_unless(name, *args, **kw):
        if omit(NameList(name), *args, **kw):
            return
        yield label, C(*args, **kw)
    return include_unless


class NameList(list):

    def __new__(cls, name):
        """Return a specialized list facilitating operations on test names.

        Split the test name into a list at '__' characters, so that it is at
        base a list of the name components that were used to construct the name
        by one or more param_maps, assuming that '__' has only been used in
        names via param_maps concatenation.  Calling string on the returned
        object should yield the original name.

        """
        return super().__new__(cls)

    def __init__(self, name):
        super().__init__(name.split('__') if name else [])

    def __str__(self):
        return '__'.join(self)

    def has_any(self, *names):
        """Return True if any name is an element of the name list

        names may be passed as a single tuple or a list of arguments.
        """
        if len(names) == 1 and not hasattr(names[0], 'encode'):
            names = names[0]
        return any(name and name in self for name in names)


    def has_all(self, *names):
        """Return True if all of the names are elements of the name list"""
        names = [n for n in names if n]
        if not names:
            return False
        return all(name in self for name in names)


def params(*args, **kw):
    """Mark decorated func so that it is called using specified C instances.

    If one or more dictionaries, and/or any keyword arguments are supplied,
    raise an error if the dictionary keys are not unique across all arguments.
    Combine these dictionaries, adding the name of the test followed by '__' as
    a prefix to each of the dictionary keys.  If called with the function as
    the only argument, create an empty dictionary instead.  If the class
    contains attributes whose names starts with 'params_' plus the name of the
    decorated function, add the attribute name with 'params_' removed from the
    front and '__' added to the end as a prefix to each key in the parameter's
    value.  If the resultant names duplicate the names in the existing combined
    dictionary, raise an error.  Otherwise add them to the combined dictionary.

    For each element of the combined dictionary, create a function whose name
    is the key, where the result of calling the function is to call the wrapped
    function with the arguments specified by the C instance (or equivalent)
    that must be the value of each dictionary entry.

    """
    if len(args) == 1 and not kw and callable( (func:= args[0]) ):
        func._params_ = False
        return func
    def params_decorator(func):
        func._params_ = Params(*args, **kw)
        return func
    return params_decorator


class ParamsMixin:

    """XXX docstring goes here once I write the docs."""

    paramsAttributePrefix = 'params_'
    paramsDebug = False
    paramsRequired = True

    @classmethod
    def __init_subclass__(cls, *args, **kwargs):
        """Turn each test decorated with @params into a series of tests.

        """
        super().__init_subclass__(*args, **kwargs)
        params_func_attrs = {}
        params_attrs = {}
        for name, attr in cls.__dict__.items():
            if hasattr(attr, '_params_'):
                params_func_attrs[name] = attr
                if __debug__ and cls.paramsDebug:
                    print(f'@params method {name!r}')
            elif name.startswith(cls.paramsAttributePrefix):
                if __debug__ and cls.paramsDebug:
                    print(f'{cls.paramsAttributePrefix} attribute {name!r}')
                params_attrs[name] = attr
        # Associate the params_ with the function with the matching name.
        params = collections.defaultdict(list)
        for pname, paramset in params_attrs.items():
            if not isinstance(paramset, Params):
                raise ValueError(
                    f'value of params constant {pname} must be a Params'
                    f' dictionary, not {type(paramset)}'
                    )
            n = pname.removeprefix(cls.paramsAttributePrefix)
            # Loop, in case the test name has one or more '__' in it...
            tn = []
            while n:
                if n in params_func_attrs:
                    break
                if '__' not in n:
                    raise ValueError(f'No @params test found for {pname!r}')
                n, t = n.rsplit('__', 1)
                if cls.paramsDebug:
                    print(f'{n=} {t=}')
                tn.insert(0, t)
            params[n].append(('__'.join(tn), paramset))
        for fname, func in params_func_attrs.items():
            if __debug__ and cls.paramsDebug:
                print(
                    f"{fname!r} has{'' if func._params_ else ' no'} decorator"
                    f" params and {(n := len(params[fname]))}"
                    f" {cls.paramsAttributePrefix}"
                    f" attribute{'' if n == 1 else 's'}",
                    )
            all_params = func._params_
            if all_params is False:
                if not params[fname]:
                    raise ValueError(f'No params found for {fname!r}')
                all_params = Params()
            for pn, ps in params[fname]:
                try:
                    pr = pn + '__' if pn else pn
                    all_params.update(
                        **{f'{pr}{n}' if pn else n: v for n, v in ps.items()}
                        )
                except Exception as ex:
                    raise ValueError(
                        f"error combining '{cls.paramsAttributePrefix}"
                        f"{fname}{'__' if pn else ''}{pn}'"
                        f" with existing params"
                        ) from ex
            if cls.paramsRequired and not all_params:
                raise ValueError(
                    f"paramsRequired is set and {fname!r} has no params",
                    )
            impl_name = '__' + fname
            delattr(cls, fname)
            setattr(cls, impl_name, func)
            for (test_name, callspec) in all_params.items():
                test = (
                    lambda self, impl_name=impl_name, callspec=callspec:
                           callspec(getattr(self, impl_name))
                    )
                test.__name__ = fname + '__' + test_name
                setattr(cls, test.__name__, test)
                if __debug__ and cls.paramsDebug:
                    print(f'generated {callspec.repr_call(test)}')
