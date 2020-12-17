"""
Internal hook annotation, representation and calling machinery.
"""
import inspect
import sys
import warnings
from .callers import _legacymulticall, _multicall


class HookspecMarker(object):
    """ Decorator helper class for marking functions as hook specifications.

    You can instantiate it with a project_name to get a decorator.
    Calling :py:meth:`.PluginManager.add_hookspecs` later will discover all marked functions
    if the :py:class:`.PluginManager` uses the same project_name.
    """

    def __init__(self, project_name):
        self.project_name = project_name

    def __call__(
        self, function=None, firstresult=False, historic=False, warn_on_impl=None
    ):
        """ if passed a function, directly sets attributes on the function
        which will make it discoverable to :py:meth:`.PluginManager.add_hookspecs`.
        If passed no function, returns a decorator which can be applied to a function
        later using the attributes supplied.

        If ``firstresult`` is ``True`` the 1:N hook call (N being the number of registered
        hook implementation functions) will stop at I<=N when the I'th function
        returns a non-``None`` result.

        If ``historic`` is ``True`` calls to a hook will be memorized and replayed
        on later registered plugins.

        """

        def setattr_hookspec_opts(func):
            if historic and firstresult:
                raise ValueError("cannot have a historic firstresult hook")
            setattr(
                func,
                self.project_name + "_spec",
                dict(
                    firstresult=firstresult,
                    historic=historic,
                    warn_on_impl=warn_on_impl,
                ),
            )
            return func

        if function is not None:
            return setattr_hookspec_opts(function)
        else:
            return setattr_hookspec_opts


class HookimplMarker(object):
    """ Decorator helper class for marking functions as hook implementations.

    You can instantiate with a ``project_name`` to get a decorator.
    Calling :py:meth:`.PluginManager.register` later will discover all marked functions
    if the :py:class:`.PluginManager` uses the same project_name.
    """

    def __init__(self, project_name):
        self.project_name = project_name

    def __call__(
        self,
        function=None,
        hookwrapper=False,
        optionalhook=False,
        tryfirst=False,
        trylast=False,
    ):

        """ if passed a function, directly sets attributes on the function
        which will make it discoverable to :py:meth:`.PluginManager.register`.
        If passed no function, returns a decorator which can be applied to a
        function later using the attributes supplied.

        If ``optionalhook`` is ``True`` a missing matching hook specification will not result
        in an error (by default it is an error if no matching spec is found).

        If ``tryfirst`` is ``True`` this hook implementation will run as early as possible
        in the chain of N hook implementations for a specification.

        If ``trylast`` is ``True`` this hook implementation will run as late as possible
        in the chain of N hook implementations.

        If ``hookwrapper`` is ``True`` the hook implementations needs to execute exactly
        one ``yield``.  The code before the ``yield`` is run early before any non-hookwrapper
        function is run.  The code after the ``yield`` is run after all non-hookwrapper
        function have run.  The ``yield`` receives a :py:class:`.callers._Result` object
        representing the exception or result outcome of the inner calls (including other
        hookwrapper calls).

        """

        def setattr_hookimpl_opts(func):
            setattr(
                func,
                self.project_name + "_impl",
                dict(
                    hookwrapper=hookwrapper,
                    optionalhook=optionalhook,
                    tryfirst=tryfirst,
                    trylast=trylast,
                ),
            )
            return func

        if function is None:
            return setattr_hookimpl_opts
        else:
            return setattr_hookimpl_opts(function)


def normalize_hookimpl_opts(opts):
    opts.setdefault("tryfirst", False)
    opts.setdefault("trylast", False)
    opts.setdefault("hookwrapper", False)
    opts.setdefault("optionalhook", False)


if hasattr(inspect, "getfullargspec"):

    def _getargspec(func):
        return inspect.getfullargspec(func)


else:

    def _getargspec(func):
        return inspect.getargspec(func)


_PYPY3 = hasattr(sys, "pypy_version_info") and sys.version_info.major == 3


def varnames(func):
    """Return tuple of positional and keywrord argument names for a function,
    method, class or callable.

    In case of a class, its ``__init__`` method is considered.
    For methods the ``self`` parameter is not included.
    """
    cache = getattr(func, "__dict__", {})
    try:
        return cache["_varnames"]
    except KeyError:
        pass

    if inspect.isclass(func):
        try:
            func = func.__init__
        except AttributeError:
            return (), ()
    elif not inspect.isroutine(func):  # callable object?
        try:
            func = getattr(func, "__call__", func)
        except Exception:
            return (), ()

    try:  # func MUST be a function or method here or we won't parse any args
        spec = _getargspec(func)
    except TypeError:
        return (), ()

    args, defaults = tuple(spec.args), spec.defaults
    if defaults:
        index = -len(defaults)
        args, kwargs = args[:index], tuple(args[index:])
    else:
        kwargs = ()

    # strip any implicit instance arg
    # pypy3 uses "obj" instead of "self" for default dunder methods
    implicit_names = ("self",) if not _PYPY3 else ("self", "obj")
    if args:
        if inspect.ismethod(func) or (
            "." in getattr(func, "__qualname__", ()) and args[0] in implicit_names
        ):
            args = args[1:]

    try:
        cache["_varnames"] = args, kwargs
    except TypeError:
        pass
    return args, kwargs


class _HookRelay(object):
    """ hook holder object for performing 1:N hook calls where N is the number
    of registered plugins.

    """


class _HookCaller(object):
    def __init__(self, name, hook_execute, specmodule_or_class=None, spec_opts=None):
        self.name = name
        self._wrappers = []
        self._nonwrappers = []
        self._hookexec = hook_execute
        self.argnames = None
        self.kwargnames = None
        self.multicall = _multicall
        self.spec = None
        if specmodule_or_class is not None:
            assert spec_opts is not None
            self.set_specification(specmodule_or_class, spec_opts)

    def has_spec(self):
        return self.spec is not None

    def set_specification(self, specmodule_or_class, spec_opts):
        assert not self.has_spec()
        self.spec = HookSpec(specmodule_or_class, self.name, spec_opts)
        if spec_opts.get("historic"):
            self._call_history = []

    def is_historic(self):
        return hasattr(self, "_call_history")

    def _remove_plugin(self, plugin):
        def remove(wrappers):
            for i, method in enumerate(wrappers):
                if method.plugin == plugin:
                    del wrappers[i]
                    return True

        if remove(self._wrappers) is None:
            if remove(self._nonwrappers) is None:
                raise ValueError("plugin %r not found" % (plugin,))

    def get_hookimpls(self):
        # Order is important for _hookexec
        return self._nonwrappers + self._wrappers

    def _add_hookimpl(self, hookimpl):
        """Add an implementation to the callback chain.
        """
        if hookimpl.hookwrapper:
            methods = self._wrappers
        else:
            methods = self._nonwrappers

        if hookimpl.trylast:
            methods.insert(0, hookimpl)
        elif hookimpl.tryfirst:
            methods.append(hookimpl)
        else:
            # find last non-tryfirst method
            i = len(methods) - 1
            while i >= 0 and methods[i].tryfirst:
                i -= 1
            methods.insert(i + 1, hookimpl)

        if "__multicall__" in hookimpl.argnames:
            warnings.warn(
                "Support for __multicall__ is now deprecated and will be"
                "removed in an upcoming release.",
                DeprecationWarning,
            )
            self.multicall = _legacymulticall

    def __repr__(self):
        return "<_HookCaller %r>" % (self.name,)

    def __call__(self, *args, **kwargs):
        if args:
            raise TypeError("hook calling supports only keyword arguments")
        assert not self.is_historic()
        if self.spec and self.spec.argnames:
            notincall = (
                set(self.spec.argnames) - set(["__multicall__"]) - set(kwargs.keys())
            )
            if notincall:
                warnings.warn(
                    "Argument(s) {} which are declared in the hookspec "
                    "can not be found in this hook call".format(tuple(notincall)),
                    stacklevel=2,
                )
        return self._hookexec(self, self.get_hookimpls(), kwargs)

    def call_historic(self, result_callback=None, kwargs=None, proc=None):
        """Call the hook with given ``kwargs`` for all registered plugins and
        for all plugins which will be registered afterwards.

        If ``result_callback`` is not ``None`` it will be called for for each
        non-``None`` result obtained from a hook implementation.

        .. note::
            The ``proc`` argument is now deprecated.
        """
        if proc is not None:
            warnings.warn(
                "Support for `proc` argument is now deprecated and will be"
                "removed in an upcoming release.",
                DeprecationWarning,
            )
            result_callback = proc

        self._call_history.append((kwargs or {}, result_callback))
        # historizing hooks don't return results
        res = self._hookexec(self, self.get_hookimpls(), kwargs)
        if result_callback is None:
            return
        # XXX: remember firstresult isn't compat with historic
        for x in res or []:
            result_callback(x)

    def call_extra(self, methods, kwargs):
        """ Call the hook with some additional temporarily participating
        methods using the specified ``kwargs`` as call parameters. """
        old = list(self._nonwrappers), list(self._wrappers)
        for method in methods:
            opts = dict(hookwrapper=False, trylast=False, tryfirst=False)
            hookimpl = HookImpl(None, "<temp>", method, opts)
            self._add_hookimpl(hookimpl)
        try:
            return self(**kwargs)
        finally:
            self._nonwrappers, self._wrappers = old

    def _maybe_apply_history(self, method):
        """Apply call history to a new hookimpl if it is marked as historic.
        """
        if self.is_historic():
            for kwargs, result_callback in self._call_history:
                res = self._hookexec(self, [method], kwargs)
                if res and result_callback is not None:
                    result_callback(res[0])


class HookImpl(object):
    def __init__(self, plugin, plugin_name, function, hook_impl_opts):
        self.function = function
        self.argnames, self.kwargnames = varnames(self.function)
        self.plugin = plugin
        self.opts = hook_impl_opts
        self.plugin_name = plugin_name
        self.__dict__.update(hook_impl_opts)

    def __repr__(self):
        return "<HookImpl plugin_name=%r, plugin=%r>" % (self.plugin_name, self.plugin)


class HookSpec(object):
    def __init__(self, namespace, name, opts):
        self.namespace = namespace
        self.function = function = getattr(namespace, name)
        self.name = name
        self.argnames, self.kwargnames = varnames(function)
        self.opts = opts
        self.argnames = ["__multicall__"] + list(self.argnames)
        self.warn_on_impl = opts.get("warn_on_impl")
