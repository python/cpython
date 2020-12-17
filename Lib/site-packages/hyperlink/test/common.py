from typing import Any, Callable, Optional, Type
from unittest import TestCase


class HyperlinkTestCase(TestCase):
    """This type mostly exists to provide a backwards-compatible
    assertRaises method for Python 2.6 testing.
    """

    def assertRaises(  # type: ignore[override]
        self,
        expected_exception,  # type: Type[BaseException]
        callableObj=None,  # type: Optional[Callable[..., Any]]
        *args,  # type: Any
        **kwargs  # type: Any
    ):
        # type: (...) -> Any
        """Fail unless an exception of class expected_exception is raised
           by callableObj when invoked with arguments args and keyword
           arguments kwargs. If a different type of exception is
           raised, it will not be caught, and the test case will be
           deemed to have suffered an error, exactly as for an
           unexpected exception.

           If called with callableObj omitted or None, will return a
           context object used like this::

                with self.assertRaises(SomeException):
                    do_something()

           The context manager keeps a reference to the exception as
           the 'exception' attribute. This allows you to inspect the
           exception after the assertion::

               with self.assertRaises(SomeException) as cm:
                   do_something()
               the_exception = cm.exception
               self.assertEqual(the_exception.error_code, 3)
        """
        context = _AssertRaisesContext(expected_exception, self)
        if callableObj is None:
            return context
        with context:
            callableObj(*args, **kwargs)


class _AssertRaisesContext(object):
    "A context manager used to implement HyperlinkTestCase.assertRaises."

    def __init__(self, expected, test_case):
        # type: (Type[BaseException], TestCase) -> None
        self.expected = expected
        self.failureException = test_case.failureException

    def __enter__(self):
        # type: () -> "_AssertRaisesContext"
        return self

    def __exit__(self, exc_type, exc_value, tb):
        # type: (Optional[Type[BaseException]], Any, Any) -> bool
        if exc_type is None:
            exc_name = self.expected.__name__
            raise self.failureException("%s not raised" % (exc_name,))
        if not issubclass(exc_type, self.expected):
            # let unexpected exceptions pass through
            return False
        self.exception = exc_value  # store for later retrieval
        return True
