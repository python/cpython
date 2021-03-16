from contextlib import contextmanager
import linecache
import os
from io import StringIO
import re
import sys
import textwrap
import unittest
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import warnings_helper
from test.support.script_helper import assert_python_ok, assert_python_failure

from test.test_warnings.data import stacklevel as warning_tests

import warnings as original_warnings


py_warnings = import_helper.import_fresh_module('warnings',
                                                blocked=['_warnings'])
c_warnings = import_helper.import_fresh_module('warnings',
                                               fresh=['_warnings'])

Py_DEBUG = hasattr(sys, 'gettotalrefcount')

@contextmanager
def warnings_state(module):
    """Use a specific warnings implementation in warning_tests."""
    global __warningregistry__
    for to_clear in (sys, warning_tests):
        try:
            to_clear.__warningregistry__.clear()
        except AttributeError:
            pass
    try:
        __warningregistry__.clear()
    except NameError:
        pass
    original_warnings = warning_tests.warnings
    original_filters = module.filters
    try:
        module.filters = original_filters[:]
        module.simplefilter("once")
        warning_tests.warnings = module
        yield
    finally:
        warning_tests.warnings = original_warnings
        module.filters = original_filters


class TestWarning(Warning):
    pass


class BaseTest:

    """Basic bookkeeping required for testing."""

    def setUp(self):
        self.old_unittest_module = unittest.case.warnings
        # The __warningregistry__ needs to be in a pristine state for tests
        # to work properly.
        if '__warningregistry__' in globals():
            del globals()['__warningregistry__']
        if hasattr(warning_tests, '__warningregistry__'):
            del warning_tests.__warningregistry__
        if hasattr(sys, '__warningregistry__'):
            del sys.__warningregistry__
        # The 'warnings' module must be explicitly set so that the proper
        # interaction between _warnings and 'warnings' can be controlled.
        sys.modules['warnings'] = self.module
        # Ensure that unittest.TestCase.assertWarns() uses the same warnings
        # module than warnings.catch_warnings(). Otherwise,
        # warnings.catch_warnings() will be unable to remove the added filter.
        unittest.case.warnings = self.module
        super(BaseTest, self).setUp()

    def tearDown(self):
        sys.modules['warnings'] = original_warnings
        unittest.case.warnings = self.old_unittest_module
        super(BaseTest, self).tearDown()

class PublicAPITests(BaseTest):

    """Ensures that the correct values are exposed in the
    public API.
    """

    def test_module_all_attribute(self):
        self.assertTrue(hasattr(self.module, '__all__'))
        target_api = ["warn", "warn_explicit", "showwarning",
                      "formatwarning", "filterwarnings", "simplefilter",
                      "resetwarnings", "catch_warnings"]
        self.assertSetEqual(set(self.module.__all__),
                            set(target_api))

class CPublicAPITests(PublicAPITests, unittest.TestCase):
    module = c_warnings

class PyPublicAPITests(PublicAPITests, unittest.TestCase):
    module = py_warnings

class FilterTests(BaseTest):

    """Testing the filtering functionality."""

    def test_error(self):
        with original_warnings.catch_warnings(module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("error", category=UserWarning)
            self.assertRaises(UserWarning, self.module.warn,
                                "FilterTests.test_error")

    def test_error_after_default(self):
        with original_warnings.catch_warnings(module=self.module) as w:
            self.module.resetwarnings()
            message = "FilterTests.test_ignore_after_default"
            def f():
                self.module.warn(message, UserWarning)

            with support.captured_stderr() as stderr:
                f()
            stderr = stderr.getvalue()
            self.assertIn("UserWarning: FilterTests.test_ignore_after_default",
                          stderr)
            self.assertIn("self.module.warn(message, UserWarning)",
                          stderr)

            self.module.filterwarnings("error", category=UserWarning)
            self.assertRaises(UserWarning, f)

    def test_ignore(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("ignore", category=UserWarning)
            self.module.warn("FilterTests.test_ignore", UserWarning)
            self.assertEqual(len(w), 0)
            self.assertEqual(list(__warningregistry__), ['version'])

    def test_ignore_after_default(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            message = "FilterTests.test_ignore_after_default"
            def f():
                self.module.warn(message, UserWarning)
            f()
            self.module.filterwarnings("ignore", category=UserWarning)
            f()
            f()
            self.assertEqual(len(w), 1)

    def test_always(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("always", category=UserWarning)
            message = "FilterTests.test_always"
            def f():
                self.module.warn(message, UserWarning)
            f()
            self.assertEqual(len(w), 1)
            self.assertEqual(w[-1].message.args[0], message)
            f()
            self.assertEqual(len(w), 2)
            self.assertEqual(w[-1].message.args[0], message)

    def test_always_after_default(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            message = "FilterTests.test_always_after_ignore"
            def f():
                self.module.warn(message, UserWarning)
            f()
            self.assertEqual(len(w), 1)
            self.assertEqual(w[-1].message.args[0], message)
            f()
            self.assertEqual(len(w), 1)
            self.module.filterwarnings("always", category=UserWarning)
            f()
            self.assertEqual(len(w), 2)
            self.assertEqual(w[-1].message.args[0], message)
            f()
            self.assertEqual(len(w), 3)
            self.assertEqual(w[-1].message.args[0], message)

    def test_default(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("default", category=UserWarning)
            message = UserWarning("FilterTests.test_default")
            for x in range(2):
                self.module.warn(message, UserWarning)
                if x == 0:
                    self.assertEqual(w[-1].message, message)
                    del w[:]
                elif x == 1:
                    self.assertEqual(len(w), 0)
                else:
                    raise ValueError("loop variant unhandled")

    def test_module(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("module", category=UserWarning)
            message = UserWarning("FilterTests.test_module")
            self.module.warn(message, UserWarning)
            self.assertEqual(w[-1].message, message)
            del w[:]
            self.module.warn(message, UserWarning)
            self.assertEqual(len(w), 0)

    def test_once(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("once", category=UserWarning)
            message = UserWarning("FilterTests.test_once")
            self.module.warn_explicit(message, UserWarning, "__init__.py",
                                    42)
            self.assertEqual(w[-1].message, message)
            del w[:]
            self.module.warn_explicit(message, UserWarning, "__init__.py",
                                    13)
            self.assertEqual(len(w), 0)
            self.module.warn_explicit(message, UserWarning, "test_warnings2.py",
                                    42)
            self.assertEqual(len(w), 0)

    def test_module_globals(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.simplefilter("always", UserWarning)

            # bpo-33509: module_globals=None must not crash
            self.module.warn_explicit('msg', UserWarning, "filename", 42,
                                      module_globals=None)
            self.assertEqual(len(w), 1)

            # Invalid module_globals type
            with self.assertRaises(TypeError):
                self.module.warn_explicit('msg', UserWarning, "filename", 42,
                                          module_globals=True)
            self.assertEqual(len(w), 1)

            # Empty module_globals
            self.module.warn_explicit('msg', UserWarning, "filename", 42,
                                      module_globals={})
            self.assertEqual(len(w), 2)

    def test_inheritance(self):
        with original_warnings.catch_warnings(module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("error", category=Warning)
            self.assertRaises(UserWarning, self.module.warn,
                                "FilterTests.test_inheritance", UserWarning)

    def test_ordering(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("ignore", category=UserWarning)
            self.module.filterwarnings("error", category=UserWarning,
                                        append=True)
            del w[:]
            try:
                self.module.warn("FilterTests.test_ordering", UserWarning)
            except UserWarning:
                self.fail("order handling for actions failed")
            self.assertEqual(len(w), 0)

    def test_filterwarnings(self):
        # Test filterwarnings().
        # Implicitly also tests resetwarnings().
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.filterwarnings("error", "", Warning, "", 0)
            self.assertRaises(UserWarning, self.module.warn, 'convert to error')

            self.module.resetwarnings()
            text = 'handle normally'
            self.module.warn(text)
            self.assertEqual(str(w[-1].message), text)
            self.assertIs(w[-1].category, UserWarning)

            self.module.filterwarnings("ignore", "", Warning, "", 0)
            text = 'filtered out'
            self.module.warn(text)
            self.assertNotEqual(str(w[-1].message), text)

            self.module.resetwarnings()
            self.module.filterwarnings("error", "hex*", Warning, "", 0)
            self.assertRaises(UserWarning, self.module.warn, 'hex/oct')
            text = 'nonmatching text'
            self.module.warn(text)
            self.assertEqual(str(w[-1].message), text)
            self.assertIs(w[-1].category, UserWarning)

    def test_message_matching(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.simplefilter("ignore", UserWarning)
            self.module.filterwarnings("error", "match", UserWarning)
            self.assertRaises(UserWarning, self.module.warn, "match")
            self.assertRaises(UserWarning, self.module.warn, "match prefix")
            self.module.warn("suffix match")
            self.assertEqual(w, [])
            self.module.warn("something completely different")
            self.assertEqual(w, [])

    def test_mutate_filter_list(self):
        class X:
            def match(self, a):
                L[:] = []

        L = [("default",X(),UserWarning,X(),0) for i in range(2)]
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.filters = L
            self.module.warn_explicit(UserWarning("b"), None, "f.py", 42)
            self.assertEqual(str(w[-1].message), "b")

    def test_filterwarnings_duplicate_filters(self):
        with original_warnings.catch_warnings(module=self.module):
            self.module.resetwarnings()
            self.module.filterwarnings("error", category=UserWarning)
            self.assertEqual(len(self.module.filters), 1)
            self.module.filterwarnings("ignore", category=UserWarning)
            self.module.filterwarnings("error", category=UserWarning)
            self.assertEqual(
                len(self.module.filters), 2,
                "filterwarnings inserted duplicate filter"
            )
            self.assertEqual(
                self.module.filters[0][0], "error",
                "filterwarnings did not promote filter to "
                "the beginning of list"
            )

    def test_simplefilter_duplicate_filters(self):
        with original_warnings.catch_warnings(module=self.module):
            self.module.resetwarnings()
            self.module.simplefilter("error", category=UserWarning)
            self.assertEqual(len(self.module.filters), 1)
            self.module.simplefilter("ignore", category=UserWarning)
            self.module.simplefilter("error", category=UserWarning)
            self.assertEqual(
                len(self.module.filters), 2,
                "simplefilter inserted duplicate filter"
            )
            self.assertEqual(
                self.module.filters[0][0], "error",
                "simplefilter did not promote filter to the beginning of list"
            )

    def test_append_duplicate(self):
        with original_warnings.catch_warnings(module=self.module,
                record=True) as w:
            self.module.resetwarnings()
            self.module.simplefilter("ignore")
            self.module.simplefilter("error", append=True)
            self.module.simplefilter("ignore", append=True)
            self.module.warn("test_append_duplicate", category=UserWarning)
            self.assertEqual(len(self.module.filters), 2,
                "simplefilter inserted duplicate filter"
            )
            self.assertEqual(len(w), 0,
                "appended duplicate changed order of filters"
            )

class CFilterTests(FilterTests, unittest.TestCase):
    module = c_warnings

class PyFilterTests(FilterTests, unittest.TestCase):
    module = py_warnings


class WarnTests(BaseTest):

    """Test warnings.warn() and warnings.warn_explicit()."""

    def test_message(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.simplefilter("once")
            for i in range(4):
                text = 'multi %d' %i  # Different text on each call.
                self.module.warn(text)
                self.assertEqual(str(w[-1].message), text)
                self.assertIs(w[-1].category, UserWarning)

    # Issue 3639
    def test_warn_nonstandard_types(self):
        # warn() should handle non-standard types without issue.
        for ob in (Warning, None, 42):
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                self.module.simplefilter("once")
                self.module.warn(ob)
                # Don't directly compare objects since
                # ``Warning() != Warning()``.
                self.assertEqual(str(w[-1].message), str(UserWarning(ob)))

    def test_filename(self):
        with warnings_state(self.module):
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                warning_tests.inner("spam1")
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "stacklevel.py")
                warning_tests.outer("spam2")
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "stacklevel.py")

    def test_stacklevel(self):
        # Test stacklevel argument
        # make sure all messages are different, so the warning won't be skipped
        with warnings_state(self.module):
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                warning_tests.inner("spam3", stacklevel=1)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "stacklevel.py")
                warning_tests.outer("spam4", stacklevel=1)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "stacklevel.py")

                warning_tests.inner("spam5", stacklevel=2)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "__init__.py")
                warning_tests.outer("spam6", stacklevel=2)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "stacklevel.py")
                warning_tests.outer("spam6.5", stacklevel=3)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "__init__.py")

                warning_tests.inner("spam7", stacklevel=9999)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "sys")

    def test_stacklevel_import(self):
        # Issue #24305: With stacklevel=2, module-level warnings should work.
        import_helper.unload('test.test_warnings.data.import_warning')
        with warnings_state(self.module):
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                self.module.simplefilter('always')
                import test.test_warnings.data.import_warning
                self.assertEqual(len(w), 1)
                self.assertEqual(w[0].filename, __file__)

    def test_exec_filename(self):
        filename = "<warnings-test>"
        codeobj = compile(("import warnings\n"
                           "warnings.warn('hello', UserWarning)"),
                          filename, "exec")
        with original_warnings.catch_warnings(record=True) as w:
            self.module.simplefilter("always", category=UserWarning)
            exec(codeobj)
        self.assertEqual(w[0].filename, filename)

    def test_warn_explicit_non_ascii_filename(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("always", category=UserWarning)
            for filename in ("nonascii\xe9\u20ac", "surrogate\udc80"):
                try:
                    os.fsencode(filename)
                except UnicodeEncodeError:
                    continue
                self.module.warn_explicit("text", UserWarning, filename, 1)
                self.assertEqual(w[-1].filename, filename)

    def test_warn_explicit_type_errors(self):
        # warn_explicit() should error out gracefully if it is given objects
        # of the wrong types.
        # lineno is expected to be an integer.
        self.assertRaises(TypeError, self.module.warn_explicit,
                            None, UserWarning, None, None)
        # Either 'message' needs to be an instance of Warning or 'category'
        # needs to be a subclass.
        self.assertRaises(TypeError, self.module.warn_explicit,
                            None, None, None, 1)
        # 'registry' must be a dict or None.
        self.assertRaises((TypeError, AttributeError),
                            self.module.warn_explicit,
                            None, Warning, None, 1, registry=42)

    def test_bad_str(self):
        # issue 6415
        # Warnings instance with a bad format string for __str__ should not
        # trigger a bus error.
        class BadStrWarning(Warning):
            """Warning with a bad format string for __str__."""
            def __str__(self):
                return ("A bad formatted string %(err)" %
                        {"err" : "there is no %(err)s"})

        with self.assertRaises(ValueError):
            self.module.warn(BadStrWarning())

    def test_warning_classes(self):
        class MyWarningClass(Warning):
            pass

        class NonWarningSubclass:
            pass

        # passing a non-subclass of Warning should raise a TypeError
        with self.assertRaises(TypeError) as cm:
            self.module.warn('bad warning category', '')
        self.assertIn('category must be a Warning subclass, not ',
                      str(cm.exception))

        with self.assertRaises(TypeError) as cm:
            self.module.warn('bad warning category', NonWarningSubclass)
        self.assertIn('category must be a Warning subclass, not ',
                      str(cm.exception))

        # check that warning instances also raise a TypeError
        with self.assertRaises(TypeError) as cm:
            self.module.warn('bad warning category', MyWarningClass())
        self.assertIn('category must be a Warning subclass, not ',
                      str(cm.exception))

        with original_warnings.catch_warnings(module=self.module):
            self.module.resetwarnings()
            self.module.filterwarnings('default')
            with self.assertWarns(MyWarningClass) as cm:
                self.module.warn('good warning category', MyWarningClass)
            self.assertEqual('good warning category', str(cm.warning))

            with self.assertWarns(UserWarning) as cm:
                self.module.warn('good warning category', None)
            self.assertEqual('good warning category', str(cm.warning))

            with self.assertWarns(MyWarningClass) as cm:
                self.module.warn('good warning category', MyWarningClass)
            self.assertIsInstance(cm.warning, Warning)

class CWarnTests(WarnTests, unittest.TestCase):
    module = c_warnings

    # As an early adopter, we sanity check the
    # test.import_helper.import_fresh_module utility function
    def test_accelerated(self):
        self.assertIsNot(original_warnings, self.module)
        self.assertFalse(hasattr(self.module.warn, '__code__'))

class PyWarnTests(WarnTests, unittest.TestCase):
    module = py_warnings

    # As an early adopter, we sanity check the
    # test.import_helper.import_fresh_module utility function
    def test_pure_python(self):
        self.assertIsNot(original_warnings, self.module)
        self.assertTrue(hasattr(self.module.warn, '__code__'))


class WCmdLineTests(BaseTest):

    def test_improper_input(self):
        # Uses the private _setoption() function to test the parsing
        # of command-line warning arguments
        with original_warnings.catch_warnings(module=self.module):
            self.assertRaises(self.module._OptionError,
                              self.module._setoption, '1:2:3:4:5:6')
            self.assertRaises(self.module._OptionError,
                              self.module._setoption, 'bogus::Warning')
            self.assertRaises(self.module._OptionError,
                              self.module._setoption, 'ignore:2::4:-5')
            with self.assertRaises(self.module._OptionError):
                self.module._setoption('ignore::123')
            with self.assertRaises(self.module._OptionError):
                self.module._setoption('ignore::123abc')
            with self.assertRaises(self.module._OptionError):
                self.module._setoption('ignore::===')
            with self.assertRaisesRegex(self.module._OptionError, 'Wärning'):
                self.module._setoption('ignore::Wärning')
            self.module._setoption('error::Warning::0')
            self.assertRaises(UserWarning, self.module.warn, 'convert to error')

    def test_import_from_module(self):
        with original_warnings.catch_warnings(module=self.module):
            self.module._setoption('ignore::Warning')
            with self.assertRaises(self.module._OptionError):
                self.module._setoption('ignore::TestWarning')
            with self.assertRaises(self.module._OptionError):
                self.module._setoption('ignore::test.test_warnings.bogus')
            self.module._setoption('error::test.test_warnings.TestWarning')
            with self.assertRaises(TestWarning):
                self.module.warn('test warning', TestWarning)


class CWCmdLineTests(WCmdLineTests, unittest.TestCase):
    module = c_warnings


class PyWCmdLineTests(WCmdLineTests, unittest.TestCase):
    module = py_warnings

    def test_improper_option(self):
        # Same as above, but check that the message is printed out when
        # the interpreter is executed. This also checks that options are
        # actually parsed at all.
        rc, out, err = assert_python_ok("-Wxxx", "-c", "pass")
        self.assertIn(b"Invalid -W option ignored: invalid action: 'xxx'", err)

    def test_warnings_bootstrap(self):
        # Check that the warnings module does get loaded when -W<some option>
        # is used (see issue #10372 for an example of silent bootstrap failure).
        rc, out, err = assert_python_ok("-Wi", "-c",
            "import sys; sys.modules['warnings'].warn('foo', RuntimeWarning)")
        # '-Wi' was observed
        self.assertFalse(out.strip())
        self.assertNotIn(b'RuntimeWarning', err)


class _WarningsTests(BaseTest, unittest.TestCase):

    """Tests specific to the _warnings module."""

    module = c_warnings

    def test_filter(self):
        # Everything should function even if 'filters' is not in warnings.
        with original_warnings.catch_warnings(module=self.module) as w:
            self.module.filterwarnings("error", "", Warning, "", 0)
            self.assertRaises(UserWarning, self.module.warn,
                                'convert to error')
            del self.module.filters
            self.assertRaises(UserWarning, self.module.warn,
                                'convert to error')

    def test_onceregistry(self):
        # Replacing or removing the onceregistry should be okay.
        global __warningregistry__
        message = UserWarning('onceregistry test')
        try:
            original_registry = self.module.onceregistry
            __warningregistry__ = {}
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                self.module.resetwarnings()
                self.module.filterwarnings("once", category=UserWarning)
                self.module.warn_explicit(message, UserWarning, "file", 42)
                self.assertEqual(w[-1].message, message)
                del w[:]
                self.module.warn_explicit(message, UserWarning, "file", 42)
                self.assertEqual(len(w), 0)
                # Test the resetting of onceregistry.
                self.module.onceregistry = {}
                __warningregistry__ = {}
                self.module.warn('onceregistry test')
                self.assertEqual(w[-1].message.args, message.args)
                # Removal of onceregistry is okay.
                del w[:]
                del self.module.onceregistry
                __warningregistry__ = {}
                self.module.warn_explicit(message, UserWarning, "file", 42)
                self.assertEqual(len(w), 0)
        finally:
            self.module.onceregistry = original_registry

    def test_default_action(self):
        # Replacing or removing defaultaction should be okay.
        message = UserWarning("defaultaction test")
        original = self.module.defaultaction
        try:
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                self.module.resetwarnings()
                registry = {}
                self.module.warn_explicit(message, UserWarning, "<test>", 42,
                                            registry=registry)
                self.assertEqual(w[-1].message, message)
                self.assertEqual(len(w), 1)
                # One actual registry key plus the "version" key
                self.assertEqual(len(registry), 2)
                self.assertIn("version", registry)
                del w[:]
                # Test removal.
                del self.module.defaultaction
                __warningregistry__ = {}
                registry = {}
                self.module.warn_explicit(message, UserWarning, "<test>", 43,
                                            registry=registry)
                self.assertEqual(w[-1].message, message)
                self.assertEqual(len(w), 1)
                self.assertEqual(len(registry), 2)
                del w[:]
                # Test setting.
                self.module.defaultaction = "ignore"
                __warningregistry__ = {}
                registry = {}
                self.module.warn_explicit(message, UserWarning, "<test>", 44,
                                            registry=registry)
                self.assertEqual(len(w), 0)
        finally:
            self.module.defaultaction = original

    def test_showwarning_missing(self):
        # Test that showwarning() missing is okay.
        text = 'del showwarning test'
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings("always", category=UserWarning)
            del self.module.showwarning
            with support.captured_output('stderr') as stream:
                self.module.warn(text)
                result = stream.getvalue()
        self.assertIn(text, result)

    def test_showwarnmsg_missing(self):
        # Test that _showwarnmsg() missing is okay.
        text = 'del _showwarnmsg test'
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings("always", category=UserWarning)

            show = self.module._showwarnmsg
            try:
                del self.module._showwarnmsg
                with support.captured_output('stderr') as stream:
                    self.module.warn(text)
                    result = stream.getvalue()
            finally:
                self.module._showwarnmsg = show
        self.assertIn(text, result)

    def test_showwarning_not_callable(self):
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings("always", category=UserWarning)
            self.module.showwarning = print
            with support.captured_output('stdout'):
                self.module.warn('Warning!')
            self.module.showwarning = 23
            self.assertRaises(TypeError, self.module.warn, "Warning!")

    def test_show_warning_output(self):
        # With showwarning() missing, make sure that output is okay.
        text = 'test show_warning'
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings("always", category=UserWarning)
            del self.module.showwarning
            with support.captured_output('stderr') as stream:
                warning_tests.inner(text)
                result = stream.getvalue()
        self.assertEqual(result.count('\n'), 2,
                             "Too many newlines in %r" % result)
        first_line, second_line = result.split('\n', 1)
        expected_file = os.path.splitext(warning_tests.__file__)[0] + '.py'
        first_line_parts = first_line.rsplit(':', 3)
        path, line, warning_class, message = first_line_parts
        line = int(line)
        self.assertEqual(expected_file, path)
        self.assertEqual(warning_class, ' ' + UserWarning.__name__)
        self.assertEqual(message, ' ' + text)
        expected_line = '  ' + linecache.getline(path, line).strip() + '\n'
        assert expected_line
        self.assertEqual(second_line, expected_line)

    def test_filename_none(self):
        # issue #12467: race condition if a warning is emitted at shutdown
        globals_dict = globals()
        oldfile = globals_dict['__file__']
        try:
            catch = original_warnings.catch_warnings(record=True,
                                                     module=self.module)
            with catch as w:
                self.module.filterwarnings("always", category=UserWarning)
                globals_dict['__file__'] = None
                original_warnings.warn('test', UserWarning)
                self.assertTrue(len(w))
        finally:
            globals_dict['__file__'] = oldfile

    def test_stderr_none(self):
        rc, stdout, stderr = assert_python_ok("-c",
            "import sys; sys.stderr = None; "
            "import warnings; warnings.simplefilter('always'); "
            "warnings.warn('Warning!')")
        self.assertEqual(stdout, b'')
        self.assertNotIn(b'Warning!', stderr)
        self.assertNotIn(b'Error', stderr)

    def test_issue31285(self):
        # warn_explicit() should neither raise a SystemError nor cause an
        # assertion failure, in case the return value of get_source() has a
        # bad splitlines() method.
        def get_bad_loader(splitlines_ret_val):
            class BadLoader:
                def get_source(self, fullname):
                    class BadSource(str):
                        def splitlines(self):
                            return splitlines_ret_val
                    return BadSource('spam')
            return BadLoader()

        wmod = self.module
        with original_warnings.catch_warnings(module=wmod):
            wmod.filterwarnings('default', category=UserWarning)

            with support.captured_stderr() as stderr:
                wmod.warn_explicit(
                    'foo', UserWarning, 'bar', 1,
                    module_globals={'__loader__': get_bad_loader(42),
                                    '__name__': 'foobar'})
            self.assertIn('UserWarning: foo', stderr.getvalue())

            show = wmod._showwarnmsg
            try:
                del wmod._showwarnmsg
                with support.captured_stderr() as stderr:
                    wmod.warn_explicit(
                        'eggs', UserWarning, 'bar', 1,
                        module_globals={'__loader__': get_bad_loader([42]),
                                        '__name__': 'foobar'})
                self.assertIn('UserWarning: eggs', stderr.getvalue())
            finally:
                wmod._showwarnmsg = show

    @support.cpython_only
    def test_issue31411(self):
        # warn_explicit() shouldn't raise a SystemError in case
        # warnings.onceregistry isn't a dictionary.
        wmod = self.module
        with original_warnings.catch_warnings(module=wmod):
            wmod.filterwarnings('once')
            with support.swap_attr(wmod, 'onceregistry', None):
                with self.assertRaises(TypeError):
                    wmod.warn_explicit('foo', Warning, 'bar', 1, registry=None)

    @support.cpython_only
    def test_issue31416(self):
        # warn_explicit() shouldn't cause an assertion failure in case of a
        # bad warnings.filters or warnings.defaultaction.
        wmod = self.module
        with original_warnings.catch_warnings(module=wmod):
            wmod.filters = [(None, None, Warning, None, 0)]
            with self.assertRaises(TypeError):
                wmod.warn_explicit('foo', Warning, 'bar', 1)

            wmod.filters = []
            with support.swap_attr(wmod, 'defaultaction', None), \
                 self.assertRaises(TypeError):
                wmod.warn_explicit('foo', Warning, 'bar', 1)

    @support.cpython_only
    def test_issue31566(self):
        # warn() shouldn't cause an assertion failure in case of a bad
        # __name__ global.
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings('error', category=UserWarning)
            with support.swap_item(globals(), '__name__', b'foo'), \
                 support.swap_item(globals(), '__file__', None):
                self.assertRaises(UserWarning, self.module.warn, 'bar')


class WarningsDisplayTests(BaseTest):

    """Test the displaying of warnings and the ability to overload functions
    related to displaying warnings."""

    def test_formatwarning(self):
        message = "msg"
        category = Warning
        file_name = os.path.splitext(warning_tests.__file__)[0] + '.py'
        line_num = 3
        file_line = linecache.getline(file_name, line_num).strip()
        format = "%s:%s: %s: %s\n  %s\n"
        expect = format % (file_name, line_num, category.__name__, message,
                            file_line)
        self.assertEqual(expect, self.module.formatwarning(message,
                                                category, file_name, line_num))
        # Test the 'line' argument.
        file_line += " for the win!"
        expect = format % (file_name, line_num, category.__name__, message,
                            file_line)
        self.assertEqual(expect, self.module.formatwarning(message,
                                    category, file_name, line_num, file_line))

    def test_showwarning(self):
        file_name = os.path.splitext(warning_tests.__file__)[0] + '.py'
        line_num = 3
        expected_file_line = linecache.getline(file_name, line_num).strip()
        message = 'msg'
        category = Warning
        file_object = StringIO()
        expect = self.module.formatwarning(message, category, file_name,
                                            line_num)
        self.module.showwarning(message, category, file_name, line_num,
                                file_object)
        self.assertEqual(file_object.getvalue(), expect)
        # Test 'line' argument.
        expected_file_line += "for the win!"
        expect = self.module.formatwarning(message, category, file_name,
                                            line_num, expected_file_line)
        file_object = StringIO()
        self.module.showwarning(message, category, file_name, line_num,
                                file_object, expected_file_line)
        self.assertEqual(expect, file_object.getvalue())

    def test_formatwarning_override(self):
        # bpo-35178: Test that a custom formatwarning function gets the 'line'
        # argument as a positional argument, and not only as a keyword argument
        def myformatwarning(message, category, filename, lineno, text):
            return f'm={message}:c={category}:f={filename}:l={lineno}:t={text}'

        file_name = os.path.splitext(warning_tests.__file__)[0] + '.py'
        line_num = 3
        file_line = linecache.getline(file_name, line_num).strip()
        message = 'msg'
        category = Warning
        file_object = StringIO()
        expected = f'm={message}:c={category}:f={file_name}:l={line_num}' + \
                   f':t={file_line}'
        with support.swap_attr(self.module, 'formatwarning', myformatwarning):
            self.module.showwarning(message, category, file_name, line_num,
                                    file_object, file_line)
            self.assertEqual(file_object.getvalue(), expected)


class CWarningsDisplayTests(WarningsDisplayTests, unittest.TestCase):
    module = c_warnings

class PyWarningsDisplayTests(WarningsDisplayTests, unittest.TestCase):
    module = py_warnings

    def test_tracemalloc(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)

        with open(os_helper.TESTFN, 'w') as fp:
            fp.write(textwrap.dedent("""
                def func():
                    f = open(__file__)
                    # Emit ResourceWarning
                    f = None

                func()
            """))

        def run(*args):
            res = assert_python_ok(*args, PYTHONIOENCODING='utf-8')
            stderr = res.err.decode('utf-8', 'replace')
            stderr = '\n'.join(stderr.splitlines())

            # normalize newlines
            stderr = re.sub('<.*>', '<...>', stderr)
            return stderr

        # tracemalloc disabled
        filename = os.path.abspath(os_helper.TESTFN)
        stderr = run('-Wd', os_helper.TESTFN)
        expected = textwrap.dedent(f'''
            {filename}:5: ResourceWarning: unclosed file <...>
              f = None
            ResourceWarning: Enable tracemalloc to get the object allocation traceback
        ''').strip()
        self.assertEqual(stderr, expected)

        # tracemalloc enabled
        stderr = run('-Wd', '-X', 'tracemalloc=2', os_helper.TESTFN)
        expected = textwrap.dedent(f'''
            {filename}:5: ResourceWarning: unclosed file <...>
              f = None
            Object allocated at (most recent call last):
              File "{filename}", lineno 7
                func()
              File "{filename}", lineno 3
                f = open(__file__)
        ''').strip()
        self.assertEqual(stderr, expected)


class CatchWarningTests(BaseTest):

    """Test catch_warnings()."""

    def test_catch_warnings_restore(self):
        wmod = self.module
        orig_filters = wmod.filters
        orig_showwarning = wmod.showwarning
        # Ensure both showwarning and filters are restored when recording
        with wmod.catch_warnings(module=wmod, record=True):
            wmod.filters = wmod.showwarning = object()
        self.assertIs(wmod.filters, orig_filters)
        self.assertIs(wmod.showwarning, orig_showwarning)
        # Same test, but with recording disabled
        with wmod.catch_warnings(module=wmod, record=False):
            wmod.filters = wmod.showwarning = object()
        self.assertIs(wmod.filters, orig_filters)
        self.assertIs(wmod.showwarning, orig_showwarning)

    def test_catch_warnings_recording(self):
        wmod = self.module
        # Ensure warnings are recorded when requested
        with wmod.catch_warnings(module=wmod, record=True) as w:
            self.assertEqual(w, [])
            self.assertIs(type(w), list)
            wmod.simplefilter("always")
            wmod.warn("foo")
            self.assertEqual(str(w[-1].message), "foo")
            wmod.warn("bar")
            self.assertEqual(str(w[-1].message), "bar")
            self.assertEqual(str(w[0].message), "foo")
            self.assertEqual(str(w[1].message), "bar")
            del w[:]
            self.assertEqual(w, [])
        # Ensure warnings are not recorded when not requested
        orig_showwarning = wmod.showwarning
        with wmod.catch_warnings(module=wmod, record=False) as w:
            self.assertIsNone(w)
            self.assertIs(wmod.showwarning, orig_showwarning)

    def test_catch_warnings_reentry_guard(self):
        wmod = self.module
        # Ensure catch_warnings is protected against incorrect usage
        x = wmod.catch_warnings(module=wmod, record=True)
        self.assertRaises(RuntimeError, x.__exit__)
        with x:
            self.assertRaises(RuntimeError, x.__enter__)
        # Same test, but with recording disabled
        x = wmod.catch_warnings(module=wmod, record=False)
        self.assertRaises(RuntimeError, x.__exit__)
        with x:
            self.assertRaises(RuntimeError, x.__enter__)

    def test_catch_warnings_defaults(self):
        wmod = self.module
        orig_filters = wmod.filters
        orig_showwarning = wmod.showwarning
        # Ensure default behaviour is not to record warnings
        with wmod.catch_warnings(module=wmod) as w:
            self.assertIsNone(w)
            self.assertIs(wmod.showwarning, orig_showwarning)
            self.assertIsNot(wmod.filters, orig_filters)
        self.assertIs(wmod.filters, orig_filters)
        if wmod is sys.modules['warnings']:
            # Ensure the default module is this one
            with wmod.catch_warnings() as w:
                self.assertIsNone(w)
                self.assertIs(wmod.showwarning, orig_showwarning)
                self.assertIsNot(wmod.filters, orig_filters)
            self.assertIs(wmod.filters, orig_filters)

    def test_record_override_showwarning_before(self):
        # Issue #28835: If warnings.showwarning() was overridden, make sure
        # that catch_warnings(record=True) overrides it again.
        text = "This is a warning"
        wmod = self.module
        my_log = []

        def my_logger(message, category, filename, lineno, file=None, line=None):
            nonlocal my_log
            my_log.append(message)

        # Override warnings.showwarning() before calling catch_warnings()
        with support.swap_attr(wmod, 'showwarning', my_logger):
            with wmod.catch_warnings(module=wmod, record=True) as log:
                self.assertIsNot(wmod.showwarning, my_logger)

                wmod.simplefilter("always")
                wmod.warn(text)

            self.assertIs(wmod.showwarning, my_logger)

        self.assertEqual(len(log), 1, log)
        self.assertEqual(log[0].message.args[0], text)
        self.assertEqual(my_log, [])

    def test_record_override_showwarning_inside(self):
        # Issue #28835: It is possible to override warnings.showwarning()
        # in the catch_warnings(record=True) context manager.
        text = "This is a warning"
        wmod = self.module
        my_log = []

        def my_logger(message, category, filename, lineno, file=None, line=None):
            nonlocal my_log
            my_log.append(message)

        with wmod.catch_warnings(module=wmod, record=True) as log:
            wmod.simplefilter("always")
            wmod.showwarning = my_logger
            wmod.warn(text)

        self.assertEqual(len(my_log), 1, my_log)
        self.assertEqual(my_log[0].args[0], text)
        self.assertEqual(log, [])

    def test_check_warnings(self):
        # Explicit tests for the test.support convenience wrapper
        wmod = self.module
        if wmod is not sys.modules['warnings']:
            self.skipTest('module to test is not loaded warnings module')
        with warnings_helper.check_warnings(quiet=False) as w:
            self.assertEqual(w.warnings, [])
            wmod.simplefilter("always")
            wmod.warn("foo")
            self.assertEqual(str(w.message), "foo")
            wmod.warn("bar")
            self.assertEqual(str(w.message), "bar")
            self.assertEqual(str(w.warnings[0].message), "foo")
            self.assertEqual(str(w.warnings[1].message), "bar")
            w.reset()
            self.assertEqual(w.warnings, [])

        with warnings_helper.check_warnings():
            # defaults to quiet=True without argument
            pass
        with warnings_helper.check_warnings(('foo', UserWarning)):
            wmod.warn("foo")

        with self.assertRaises(AssertionError):
            with warnings_helper.check_warnings(('', RuntimeWarning)):
                # defaults to quiet=False with argument
                pass
        with self.assertRaises(AssertionError):
            with warnings_helper.check_warnings(('foo', RuntimeWarning)):
                wmod.warn("foo")

class CCatchWarningTests(CatchWarningTests, unittest.TestCase):
    module = c_warnings

class PyCatchWarningTests(CatchWarningTests, unittest.TestCase):
    module = py_warnings


class EnvironmentVariableTests(BaseTest):

    def test_single_warning(self):
        rc, stdout, stderr = assert_python_ok("-c",
            "import sys; sys.stdout.write(str(sys.warnoptions))",
            PYTHONWARNINGS="ignore::DeprecationWarning",
            PYTHONDEVMODE="")
        self.assertEqual(stdout, b"['ignore::DeprecationWarning']")

    def test_comma_separated_warnings(self):
        rc, stdout, stderr = assert_python_ok("-c",
            "import sys; sys.stdout.write(str(sys.warnoptions))",
            PYTHONWARNINGS="ignore::DeprecationWarning,ignore::UnicodeWarning",
            PYTHONDEVMODE="")
        self.assertEqual(stdout,
            b"['ignore::DeprecationWarning', 'ignore::UnicodeWarning']")

    def test_envvar_and_command_line(self):
        rc, stdout, stderr = assert_python_ok("-Wignore::UnicodeWarning", "-c",
            "import sys; sys.stdout.write(str(sys.warnoptions))",
            PYTHONWARNINGS="ignore::DeprecationWarning",
            PYTHONDEVMODE="")
        self.assertEqual(stdout,
            b"['ignore::DeprecationWarning', 'ignore::UnicodeWarning']")

    def test_conflicting_envvar_and_command_line(self):
        rc, stdout, stderr = assert_python_failure("-Werror::DeprecationWarning", "-c",
            "import sys, warnings; sys.stdout.write(str(sys.warnoptions)); "
            "warnings.warn('Message', DeprecationWarning)",
            PYTHONWARNINGS="default::DeprecationWarning",
            PYTHONDEVMODE="")
        self.assertEqual(stdout,
            b"['default::DeprecationWarning', 'error::DeprecationWarning']")
        self.assertEqual(stderr.splitlines(),
            [b"Traceback (most recent call last):",
             b"  File \"<string>\", line 1, in <module>",
             b"DeprecationWarning: Message"])

    def test_default_filter_configuration(self):
        pure_python_api = self.module is py_warnings
        if Py_DEBUG:
            expected_default_filters = []
        else:
            if pure_python_api:
                main_module_filter = re.compile("__main__")
            else:
                main_module_filter = "__main__"
            expected_default_filters = [
                ('default', None, DeprecationWarning, main_module_filter, 0),
                ('ignore', None, DeprecationWarning, None, 0),
                ('ignore', None, PendingDeprecationWarning, None, 0),
                ('ignore', None, ImportWarning, None, 0),
                ('ignore', None, ResourceWarning, None, 0),
            ]
        expected_output = [str(f).encode() for f in expected_default_filters]

        if pure_python_api:
            # Disable the warnings acceleration module in the subprocess
            code = "import sys; sys.modules.pop('warnings', None); sys.modules['_warnings'] = None; "
        else:
            code = ""
        code += "import warnings; [print(f) for f in warnings.filters]"

        rc, stdout, stderr = assert_python_ok("-c", code, __isolated=True)
        stdout_lines = [line.strip() for line in stdout.splitlines()]
        self.maxDiff = None
        self.assertEqual(stdout_lines, expected_output)


    @unittest.skipUnless(sys.getfilesystemencoding() != 'ascii',
                         'requires non-ascii filesystemencoding')
    def test_nonascii(self):
        PYTHONWARNINGS="ignore:DeprecationWarning" + os_helper.FS_NONASCII
        rc, stdout, stderr = assert_python_ok("-c",
            "import sys; sys.stdout.write(str(sys.warnoptions))",
            PYTHONIOENCODING="utf-8",
            PYTHONWARNINGS=PYTHONWARNINGS,
            PYTHONDEVMODE="")
        self.assertEqual(stdout, str([PYTHONWARNINGS]).encode())

class CEnvironmentVariableTests(EnvironmentVariableTests, unittest.TestCase):
    module = c_warnings

class PyEnvironmentVariableTests(EnvironmentVariableTests, unittest.TestCase):
    module = py_warnings


class BootstrapTest(unittest.TestCase):
    def test_issue_8766(self):
        # "import encodings" emits a warning whereas the warnings is not loaded
        # or not completely loaded (warnings imports indirectly encodings by
        # importing linecache) yet
        with os_helper.temp_cwd() as cwd, os_helper.temp_cwd('encodings'):
            # encodings loaded by initfsencoding()
            assert_python_ok('-c', 'pass', PYTHONPATH=cwd)

            # Use -W to load warnings module at startup
            assert_python_ok('-c', 'pass', '-W', 'always', PYTHONPATH=cwd)


class FinalizationTest(unittest.TestCase):
    def test_finalization(self):
        # Issue #19421: warnings.warn() should not crash
        # during Python finalization
        code = """
import warnings
warn = warnings.warn

class A:
    def __del__(self):
        warn("test")

a=A()
        """
        rc, out, err = assert_python_ok("-c", code)
        self.assertEqual(err.decode().rstrip(),
                         '<string>:7: UserWarning: test')

    def test_late_resource_warning(self):
        # Issue #21925: Emitting a ResourceWarning late during the Python
        # shutdown must be logged.

        expected = b"sys:1: ResourceWarning: unclosed file "

        # don't import the warnings module
        # (_warnings will try to import it)
        code = "f = open(%a)" % __file__
        rc, out, err = assert_python_ok("-Wd", "-c", code)
        self.assertTrue(err.startswith(expected), ascii(err))

        # import the warnings module
        code = "import warnings; f = open(%a)" % __file__
        rc, out, err = assert_python_ok("-Wd", "-c", code)
        self.assertTrue(err.startswith(expected), ascii(err))


def setUpModule():
    py_warnings.onceregistry.clear()
    c_warnings.onceregistry.clear()

tearDownModule = setUpModule

if __name__ == "__main__":
    unittest.main()
