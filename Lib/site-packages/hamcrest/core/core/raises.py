import re
import sys
from typing import Any, Callable, Mapping, Optional, Tuple, Type, cast
from weakref import ref

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Per Fagrell"
__copyright__ = "Copyright 2013 hamcrest.org"
__license__ = "BSD, see License.txt"


class Raises(BaseMatcher[Callable[..., Any]]):
    def __init__(
        self,
        expected: Type[Exception],
        pattern: Optional[str] = None,
        matching: Optional[Matcher] = None,
    ) -> None:
        self.pattern = pattern
        self.matcher = matching
        self.expected = expected
        self.actual = None  # type: Optional[BaseException]
        self.function = None  # type: Optional[Callable[..., Any]]

    def _matches(self, function: Callable[..., Any]) -> bool:
        if not callable(function):
            return False

        self.function = cast(Callable[..., Any], ref(function))
        return self._call_function(function)

    def _call_function(self, function: Callable[..., Any]) -> bool:
        self.actual = None
        try:
            function()
        except BaseException:
            self.actual = sys.exc_info()[1]

            if isinstance(self.actual, self.expected):
                if self.pattern is not None:
                    if re.search(self.pattern, str(self.actual)) is None:
                        return False
                if self.matcher is not None:
                    if not self.matcher.matches(self.actual):
                        return False
                return True
        return False

    def describe_to(self, description: Description) -> None:
        description.append_text("Expected a callable raising %s" % self.expected)

    def describe_mismatch(self, item, description: Description) -> None:
        if not callable(item):
            description.append_text("%s is not callable" % item)
            return

        function = None if self.function is None else self.function()
        if function is None or function is not item:
            self.function = ref(item)
            if not self._call_function(item):
                return

        if self.actual is None:
            description.append_text("No exception raised.")
        elif isinstance(self.actual, self.expected):
            if self.pattern is not None or self.matcher is not None:
                description.append_text("Correct assertion type raised, but ")
                if self.pattern is not None:
                    description.append_text('the expected pattern ("%s") ' % self.pattern)
                if self.pattern is not None and self.matcher is not None:
                    description.append_text("and ")
                if self.matcher is not None:
                    description.append_description_of(self.matcher)
                    description.append_text(" ")
                description.append_text('not found. Exception message was: "%s"' % str(self.actual))
        else:
            description.append_text(
                "%r of type %s was raised instead" % (self.actual, type(self.actual))
            )

    def describe_match(self, item, match_description: Description) -> None:
        self._call_function(item)
        match_description.append_text(
            "%r of type %s was raised." % (self.actual, type(self.actual))
        )


def raises(exception: Type[Exception], pattern=None, matching=None) -> Matcher[Callable[..., Any]]:
    """Matches if the called function raised the expected exception.

    :param exception:  The class of the expected exception
    :param pattern:    Optional regular expression to match exception message.
    :param matching:   Optional Hamcrest matchers to apply to the exception.

    Expects the actual to be wrapped by using :py:func:`~hamcrest.core.core.raises.calling`,
    or a callable taking no arguments.
    Optional argument pattern should be a string containing a regular expression.  If provided,
    the string representation of the actual exception - e.g. `str(actual)` - must match pattern.

    Examples::

        assert_that(calling(int).with_args('q'), raises(TypeError))
        assert_that(calling(parse, broken_input), raises(ValueError))
        assert_that(
            calling(valid_user, bad_json),
            raises(HTTPError, matching=has_properties(status_code=500)
        )
    """
    return Raises(exception, pattern, matching)


class DeferredCallable(object):
    def __init__(self, func: Callable[..., Any]):
        self.func = func
        self.args = tuple()  # type: Tuple[Any, ...]
        self.kwargs = {}  # type: Mapping[str, Any]

    def __call__(self):
        self.func(*self.args, **self.kwargs)

    def with_args(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        return self


def calling(func: Callable[..., Any]) -> DeferredCallable:
    """Wrapper for function call that delays the actual execution so that
    :py:func:`~hamcrest.core.core.raises.raises` matcher can catch any thrown exception.

    :param func: The function or method to be called

    The arguments can be provided with a call to the `with_args` function on the returned
    object::

           calling(my_method).with_args(arguments, and_='keywords')
    """
    return DeferredCallable(func)
