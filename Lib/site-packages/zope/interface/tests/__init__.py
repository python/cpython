from zope.interface._compat import _should_attempt_c_optimizations


class OptimizationTestMixin(object):
    """
    Helper for testing that C optimizations are used
    when appropriate.
    """

    def _getTargetClass(self):
        """
        Define this to return the implementation in use,
        without the 'Py' or 'Fallback' suffix.
        """
        raise NotImplementedError

    def _getFallbackClass(self):
        """
        Define this to return the fallback Python implementation.
        """
        # Is there an algorithmic way to do this? The C
        # objects all come from the same module so I don't see how we can
        # get the Python object from that.
        raise NotImplementedError

    def test_optimizations(self):
        used = self._getTargetClass()
        fallback = self._getFallbackClass()

        if _should_attempt_c_optimizations():
            self.assertIsNot(used, fallback)
        else:
            self.assertIs(used, fallback)


class MissingSomeAttrs(object):
    """
    Helper for tests that raises a specific exception
    for attributes that are missing. This is usually not
    an AttributeError, and this object is used to test that
    those errors are not improperly caught and treated like
    an AttributeError.
    """

    def __init__(self, exc_kind, **other_attrs):
        self.__exc_kind = exc_kind
        d = object.__getattribute__(self, '__dict__')
        d.update(other_attrs)

    def __getattribute__(self, name):
        # Note that we ignore objects found in the class dictionary.
        d = object.__getattribute__(self, '__dict__')
        try:
            return d[name]
        except KeyError:
            raise d['_MissingSomeAttrs__exc_kind'](name)

    EXCEPTION_CLASSES = (
        TypeError,
        RuntimeError,
        BaseException,
        ValueError,
    )

    @classmethod
    def test_raises(cls, unittest, test_func, expected_missing, **other_attrs):
        """
        Loop through various exceptions, calling *test_func* inside a ``assertRaises`` block.

        :param test_func: A callable of one argument, the instance of this
            class.
        :param str expected_missing: The attribute that should fail with the exception.
           This is used to ensure that we're testing the path we think we are.
        :param other_attrs: Attributes that should be provided on the test object.
           Must not contain *expected_missing*.
        """
        assert isinstance(expected_missing, str)
        assert expected_missing not in other_attrs
        for exc in cls.EXCEPTION_CLASSES:
            ob = cls(exc, **other_attrs)
            with unittest.assertRaises(exc) as ex:
                test_func(ob)

            unittest.assertEqual(ex.exception.args[0], expected_missing)

        # Now test that the AttributeError for that expected_missing is *not* raised.
        ob = cls(AttributeError, **other_attrs)
        try:
            test_func(ob)
        except AttributeError as e:
            unittest.assertNotIn(expected_missing, str(e))
        except Exception: # pylint:disable=broad-except
            pass

# Be sure cleanup functionality is available; classes that use the adapter hook
# need to be sure to subclass ``CleanUp``.
#
# If zope.component is installed and imported when we run our tests
# (import chain:
# zope.testrunner->zope.security->zope.location->zope.component.api)
# it adds an adapter hook that uses its global site manager. That can cause
# leakage from one test to another unless its cleanup hooks are run. The symptoms can
# be odd, especially if one test used C objects and the next used the Python
# implementation. (For example, you can get strange TypeErrors or find inexplicable
# comparisons being done.)
try:
    from zope.testing import cleanup
except ImportError:
    class CleanUp(object):
        def cleanUp(self):
            pass

        setUp = tearDown = cleanUp
else:
    CleanUp = cleanup.CleanUp
