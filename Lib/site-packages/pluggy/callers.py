"""
Call loop machinery
"""
import sys
import warnings

_py3 = sys.version_info > (3, 0)


if not _py3:
    exec(
        """
def _reraise(cls, val, tb):
    raise cls, val, tb
"""
    )


def _raise_wrapfail(wrap_controller, msg):
    co = wrap_controller.gi_code
    raise RuntimeError(
        "wrap_controller at %r %s:%d %s"
        % (co.co_name, co.co_filename, co.co_firstlineno, msg)
    )


class HookCallError(Exception):
    """ Hook was called wrongly. """


class _Result(object):
    def __init__(self, result, excinfo):
        self._result = result
        self._excinfo = excinfo

    @property
    def excinfo(self):
        return self._excinfo

    @property
    def result(self):
        """Get the result(s) for this hook call (DEPRECATED in favor of ``get_result()``)."""
        msg = "Use get_result() which forces correct exception handling"
        warnings.warn(DeprecationWarning(msg), stacklevel=2)
        return self._result

    @classmethod
    def from_call(cls, func):
        __tracebackhide__ = True
        result = excinfo = None
        try:
            result = func()
        except BaseException:
            excinfo = sys.exc_info()

        return cls(result, excinfo)

    def force_result(self, result):
        """Force the result(s) to ``result``.

        If the hook was marked as a ``firstresult`` a single value should
        be set otherwise set a (modified) list of results. Any exceptions
        found during invocation will be deleted.
        """
        self._result = result
        self._excinfo = None

    def get_result(self):
        """Get the result(s) for this hook call.

        If the hook was marked as a ``firstresult`` only a single value
        will be returned otherwise a list of results.
        """
        __tracebackhide__ = True
        if self._excinfo is None:
            return self._result
        else:
            ex = self._excinfo
            if _py3:
                raise ex[1].with_traceback(ex[2])
            _reraise(*ex)  # noqa


def _wrapped_call(wrap_controller, func):
    """ Wrap calling to a function with a generator which needs to yield
    exactly once.  The yield point will trigger calling the wrapped function
    and return its ``_Result`` to the yield point.  The generator then needs
    to finish (raise StopIteration) in order for the wrapped call to complete.
    """
    try:
        next(wrap_controller)  # first yield
    except StopIteration:
        _raise_wrapfail(wrap_controller, "did not yield")
    call_outcome = _Result.from_call(func)
    try:
        wrap_controller.send(call_outcome)
        _raise_wrapfail(wrap_controller, "has second yield")
    except StopIteration:
        pass
    return call_outcome.get_result()


class _LegacyMultiCall(object):
    """ execute a call into multiple python functions/methods. """

    # XXX note that the __multicall__ argument is supported only
    # for pytest compatibility reasons.  It was never officially
    # supported there and is explicitely deprecated since 2.8
    # so we can remove it soon, allowing to avoid the below recursion
    # in execute() and simplify/speed up the execute loop.

    def __init__(self, hook_impls, kwargs, firstresult=False):
        self.hook_impls = hook_impls
        self.caller_kwargs = kwargs  # come from _HookCaller.__call__()
        self.caller_kwargs["__multicall__"] = self
        self.firstresult = firstresult

    def execute(self):
        caller_kwargs = self.caller_kwargs
        self.results = results = []
        firstresult = self.firstresult

        while self.hook_impls:
            hook_impl = self.hook_impls.pop()
            try:
                args = [caller_kwargs[argname] for argname in hook_impl.argnames]
            except KeyError:
                for argname in hook_impl.argnames:
                    if argname not in caller_kwargs:
                        raise HookCallError(
                            "hook call must provide argument %r" % (argname,)
                        )
            if hook_impl.hookwrapper:
                return _wrapped_call(hook_impl.function(*args), self.execute)
            res = hook_impl.function(*args)
            if res is not None:
                if firstresult:
                    return res
                results.append(res)

        if not firstresult:
            return results

    def __repr__(self):
        status = "%d meths" % (len(self.hook_impls),)
        if hasattr(self, "results"):
            status = ("%d results, " % len(self.results)) + status
        return "<_MultiCall %s, kwargs=%r>" % (status, self.caller_kwargs)


def _legacymulticall(hook_impls, caller_kwargs, firstresult=False):
    return _LegacyMultiCall(
        hook_impls, caller_kwargs, firstresult=firstresult
    ).execute()


def _multicall(hook_impls, caller_kwargs, firstresult=False):
    """Execute a call into multiple python functions/methods and return the
    result(s).

    ``caller_kwargs`` comes from _HookCaller.__call__().
    """
    __tracebackhide__ = True
    results = []
    excinfo = None
    try:  # run impl and wrapper setup functions in a loop
        teardowns = []
        try:
            for hook_impl in reversed(hook_impls):
                try:
                    args = [caller_kwargs[argname] for argname in hook_impl.argnames]
                except KeyError:
                    for argname in hook_impl.argnames:
                        if argname not in caller_kwargs:
                            raise HookCallError(
                                "hook call must provide argument %r" % (argname,)
                            )

                if hook_impl.hookwrapper:
                    try:
                        gen = hook_impl.function(*args)
                        next(gen)  # first yield
                        teardowns.append(gen)
                    except StopIteration:
                        _raise_wrapfail(gen, "did not yield")
                else:
                    res = hook_impl.function(*args)
                    if res is not None:
                        results.append(res)
                        if firstresult:  # halt further impl calls
                            break
        except BaseException:
            excinfo = sys.exc_info()
    finally:
        if firstresult:  # first result hooks return a single value
            outcome = _Result(results[0] if results else None, excinfo)
        else:
            outcome = _Result(results, excinfo)

        # run all wrapper post-yield blocks
        for gen in reversed(teardowns):
            try:
                gen.send(outcome)
                _raise_wrapfail(gen, "has second yield")
            except StopIteration:
                pass

        return outcome.get_result()
