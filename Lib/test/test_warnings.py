from contextlib import contextmanager
import linecache
import os
import StringIO
import sys
import unittest
import subprocess
from test import test_support
from test.script_helper import assert_python_ok

import warning_tests

import warnings as original_warnings

py_warnings = test_support.import_fresh_module('warnings', blocked=['_warnings'])
c_warnings = test_support.import_fresh_module('warnings', fresh=['_warnings'])

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


class BaseTest(unittest.TestCase):

    """Basic bookkeeping required for testing."""

    def setUp(self):
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
        super(BaseTest, self).setUp()

    def tearDown(self):
        sys.modules['warnings'] = original_warnings
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

class FilterTests(object):

    """Testing the filtering functionality."""

    def test_error(self):
        with original_warnings.catch_warnings(module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("error", category=UserWarning)
            self.assertRaises(UserWarning, self.module.warn,
                                "FilterTests.test_error")

    def test_ignore(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("ignore", category=UserWarning)
            self.module.warn("FilterTests.test_ignore", UserWarning)
            self.assertEqual(len(w), 0)

    def test_always(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("always", category=UserWarning)
            message = "FilterTests.test_always"
            self.module.warn(message, UserWarning)
            self.assertTrue(message, w[-1].message)
            self.module.warn(message, UserWarning)
            self.assertTrue(w[-1].message, message)

    def test_default(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.resetwarnings()
            self.module.filterwarnings("default", category=UserWarning)
            message = UserWarning("FilterTests.test_default")
            for x in xrange(2):
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
            self.module.warn_explicit(message, UserWarning, "test_warnings.py",
                                    42)
            self.assertEqual(w[-1].message, message)
            del w[:]
            self.module.warn_explicit(message, UserWarning, "test_warnings.py",
                                    13)
            self.assertEqual(len(w), 0)
            self.module.warn_explicit(message, UserWarning, "test_warnings2.py",
                                    42)
            self.assertEqual(len(w), 0)

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
            self.assertTrue(w[-1].category is UserWarning)

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
            self.assertTrue(w[-1].category is UserWarning)

class CFilterTests(BaseTest, FilterTests):
    module = c_warnings

class PyFilterTests(BaseTest, FilterTests):
    module = py_warnings


class WarnTests(unittest.TestCase):

    """Test warnings.warn() and warnings.warn_explicit()."""

    def test_message(self):
        with original_warnings.catch_warnings(record=True,
                module=self.module) as w:
            self.module.simplefilter("once")
            for i in range(4):
                text = 'multi %d' %i  # Different text on each call.
                self.module.warn(text)
                self.assertEqual(str(w[-1].message), text)
                self.assertTrue(w[-1].category is UserWarning)

    def test_filename(self):
        with warnings_state(self.module):
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                warning_tests.inner("spam1")
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "warning_tests.py")
                warning_tests.outer("spam2")
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "warning_tests.py")

    def test_stacklevel(self):
        # Test stacklevel argument
        # make sure all messages are different, so the warning won't be skipped
        with warnings_state(self.module):
            with original_warnings.catch_warnings(record=True,
                    module=self.module) as w:
                warning_tests.inner("spam3", stacklevel=1)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "warning_tests.py")
                warning_tests.outer("spam4", stacklevel=1)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "warning_tests.py")

                warning_tests.inner("spam5", stacklevel=2)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "test_warnings.py")
                warning_tests.outer("spam6", stacklevel=2)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "warning_tests.py")
                warning_tests.outer("spam6.5", stacklevel=3)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "test_warnings.py")

                warning_tests.inner("spam7", stacklevel=9999)
                self.assertEqual(os.path.basename(w[-1].filename),
                                    "sys")

    def test_missing_filename_not_main(self):
        # If __file__ is not specified and __main__ is not the module name,
        # then __file__ should be set to the module name.
        filename = warning_tests.__file__
        try:
            del warning_tests.__file__
            with warnings_state(self.module):
                with original_warnings.catch_warnings(record=True,
                        module=self.module) as w:
                    warning_tests.inner("spam8", stacklevel=1)
                    self.assertEqual(w[-1].filename, warning_tests.__name__)
        finally:
            warning_tests.__file__ = filename

    @unittest.skipUnless(hasattr(sys, 'argv'), 'test needs sys.argv')
    def test_missing_filename_main_with_argv(self):
        # If __file__ is not specified and the caller is __main__ and sys.argv
        # exists, then use sys.argv[0] as the file.
        filename = warning_tests.__file__
        module_name = warning_tests.__name__
        try:
            del warning_tests.__file__
            warning_tests.__name__ = '__main__'
            with warnings_state(self.module):
                with original_warnings.catch_warnings(record=True,
                        module=self.module) as w:
                    warning_tests.inner('spam9', stacklevel=1)
                    self.assertEqual(w[-1].filename, sys.argv[0])
        finally:
            warning_tests.__file__ = filename
            warning_tests.__name__ = module_name

    def test_missing_filename_main_without_argv(self):
        # If __file__ is not specified, the caller is __main__, and sys.argv
        # is not set, then '__main__' is the file name.
        filename = warning_tests.__file__
        module_name = warning_tests.__name__
        argv = sys.argv
        try:
            del warning_tests.__file__
            warning_tests.__name__ = '__main__'
            del sys.argv
            with warnings_state(self.module):
                with original_warnings.catch_warnings(record=True,
                        module=self.module) as w:
                    warning_tests.inner('spam10', stacklevel=1)
                    self.assertEqual(w[-1].filename, '__main__')
        finally:
            warning_tests.__file__ = filename
            warning_tests.__name__ = module_name
            sys.argv = argv

    def test_missing_filename_main_with_argv_empty_string(self):
        # If __file__ is not specified, the caller is __main__, and sys.argv[0]
        # is the empty string, then '__main__ is the file name.
        # Tests issue 2743.
        file_name = warning_tests.__file__
        module_name = warning_tests.__name__
        argv = sys.argv
        try:
            del warning_tests.__file__
            warning_tests.__name__ = '__main__'
            sys.argv = ['']
            with warnings_state(self.module):
                with original_warnings.catch_warnings(record=True,
                        module=self.module) as w:
                    warning_tests.inner('spam11', stacklevel=1)
                    self.assertEqual(w[-1].filename, '__main__')
        finally:
            warning_tests.__file__ = file_name
            warning_tests.__name__ = module_name
            sys.argv = argv

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


class CWarnTests(BaseTest, WarnTests):
    module = c_warnings

    # As an early adopter, we sanity check the
    # test_support.import_fresh_module utility function
    def test_accelerated(self):
        self.assertFalse(original_warnings is self.module)
        self.assertFalse(hasattr(self.module.warn, 'func_code'))

class PyWarnTests(BaseTest, WarnTests):
    module = py_warnings

    # As an early adopter, we sanity check the
    # test_support.import_fresh_module utility function
    def test_pure_python(self):
        self.assertFalse(original_warnings is self.module)
        self.assertTrue(hasattr(self.module.warn, 'func_code'))


class WCmdLineTests(unittest.TestCase):

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
            self.module._setoption('error::Warning::0')
            self.assertRaises(UserWarning, self.module.warn, 'convert to error')

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

class CWCmdLineTests(BaseTest, WCmdLineTests):
    module = c_warnings

class PyWCmdLineTests(BaseTest, WCmdLineTests):
    module = py_warnings


class _WarningsTests(BaseTest):

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
                self.assertEqual(len(registry), 1)
                del w[:]
                # Test removal.
                del self.module.defaultaction
                __warningregistry__ = {}
                registry = {}
                self.module.warn_explicit(message, UserWarning, "<test>", 43,
                                            registry=registry)
                self.assertEqual(w[-1].message, message)
                self.assertEqual(len(w), 1)
                self.assertEqual(len(registry), 1)
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
            with test_support.captured_output('stderr') as stream:
                self.module.warn(text)
                result = stream.getvalue()
        self.assertIn(text, result)

    def test_showwarning_not_callable(self):
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings("always", category=UserWarning)
            old_showwarning = self.module.showwarning
            self.module.showwarning = 23
            try:
                self.assertRaises(TypeError, self.module.warn, "Warning!")
            finally:
                self.module.showwarning = old_showwarning

    def test_show_warning_output(self):
        # With showarning() missing, make sure that output is okay.
        text = 'test show_warning'
        with original_warnings.catch_warnings(module=self.module):
            self.module.filterwarnings("always", category=UserWarning)
            del self.module.showwarning
            with test_support.captured_output('stderr') as stream:
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
            with original_warnings.catch_warnings(module=self.module, record=True) as w:
                self.module.filterwarnings("always", category=UserWarning)
                globals_dict['__file__'] = None
                self.module.warn('test', UserWarning)
            self.assertEqual(len(w), 1)
            self.assertEqual(w[0].category, UserWarning)
            self.assertEqual(str(w[0].message), 'test')
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


class WarningsDisplayTests(unittest.TestCase):

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

    @test_support.requires_unicode
    def test_formatwarning_unicode_msg(self):
        message = u"msg"
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

    @test_support.requires_unicode
    @unittest.skipUnless(test_support.FS_NONASCII, 'need test_support.FS_NONASCII')
    def test_formatwarning_unicode_msg_nonascii_filename(self):
        message = u"msg"
        category = Warning
        unicode_file_name = test_support.FS_NONASCII + u'.py'
        file_name = unicode_file_name.encode(sys.getfilesystemencoding())
        line_num = 3
        file_line = 'spam'
        format = "%s:%s: %s: %s\n  %s\n"
        expect = format % (file_name, line_num, category.__name__, str(message),
                            file_line)
        self.assertEqual(expect, self.module.formatwarning(message,
                                    category, file_name, line_num, file_line))
        message = u"\xb5sg"
        expect = format % (unicode_file_name, line_num, category.__name__, message,
                            file_line)
        self.assertEqual(expect, self.module.formatwarning(message,
                                    category, file_name, line_num, file_line))

    @test_support.requires_unicode
    def test_formatwarning_unicode_msg_nonascii_fileline(self):
        message = u"msg"
        category = Warning
        file_name = 'file.py'
        line_num = 3
        file_line = 'sp\xe4m'
        format = "%s:%s: %s: %s\n  %s\n"
        expect = format % (file_name, line_num, category.__name__, str(message),
                            file_line)
        self.assertEqual(expect, self.module.formatwarning(message,
                                    category, file_name, line_num, file_line))
        message = u"\xb5sg"
        expect = format % (file_name, line_num, category.__name__, message,
                            unicode(file_line, 'latin1'))
        self.assertEqual(expect, self.module.formatwarning(message,
                                    category, file_name, line_num, file_line))

    def test_showwarning(self):
        file_name = os.path.splitext(warning_tests.__file__)[0] + '.py'
        line_num = 3
        expected_file_line = linecache.getline(file_name, line_num).strip()
        message = 'msg'
        category = Warning
        file_object = StringIO.StringIO()
        expect = self.module.formatwarning(message, category, file_name,
                                            line_num)
        self.module.showwarning(message, category, file_name, line_num,
                                file_object)
        self.assertEqual(file_object.getvalue(), expect)
        # Test 'line' argument.
        expected_file_line += "for the win!"
        expect = self.module.formatwarning(message, category, file_name,
                                            line_num, expected_file_line)
        file_object = StringIO.StringIO()
        self.module.showwarning(message, category, file_name, line_num,
                                file_object, expected_file_line)
        self.assertEqual(expect, file_object.getvalue())

class CWarningsDisplayTests(BaseTest, WarningsDisplayTests):
    module = c_warnings

class PyWarningsDisplayTests(BaseTest, WarningsDisplayTests):
    module = py_warnings


class CatchWarningTests(BaseTest):

    """Test catch_warnings()."""

    def test_catch_warnings_restore(self):
        wmod = self.module
        orig_filters = wmod.filters
        orig_showwarning = wmod.showwarning
        # Ensure both showwarning and filters are restored when recording
        with wmod.catch_warnings(module=wmod, record=True):
            wmod.filters = wmod.showwarning = object()
        self.assertTrue(wmod.filters is orig_filters)
        self.assertTrue(wmod.showwarning is orig_showwarning)
        # Same test, but with recording disabled
        with wmod.catch_warnings(module=wmod, record=False):
            wmod.filters = wmod.showwarning = object()
        self.assertTrue(wmod.filters is orig_filters)
        self.assertTrue(wmod.showwarning is orig_showwarning)

    def test_catch_warnings_recording(self):
        wmod = self.module
        # Ensure warnings are recorded when requested
        with wmod.catch_warnings(module=wmod, record=True) as w:
            self.assertEqual(w, [])
            self.assertTrue(type(w) is list)
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
            self.assertTrue(w is None)
            self.assertTrue(wmod.showwarning is orig_showwarning)

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
            self.assertTrue(w is None)
            self.assertTrue(wmod.showwarning is orig_showwarning)
            self.assertTrue(wmod.filters is not orig_filters)
        self.assertTrue(wmod.filters is orig_filters)
        if wmod is sys.modules['warnings']:
            # Ensure the default module is this one
            with wmod.catch_warnings() as w:
                self.assertTrue(w is None)
                self.assertTrue(wmod.showwarning is orig_showwarning)
                self.assertTrue(wmod.filters is not orig_filters)
            self.assertTrue(wmod.filters is orig_filters)

    def test_check_warnings(self):
        # Explicit tests for the test_support convenience wrapper
        wmod = self.module
        if wmod is not sys.modules['warnings']:
            self.skipTest('module to test is not loaded warnings module')
        with test_support.check_warnings(quiet=False) as w:
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

        with test_support.check_warnings():
            # defaults to quiet=True without argument
            pass
        with test_support.check_warnings(('foo', UserWarning)):
            wmod.warn("foo")

        with self.assertRaises(AssertionError):
            with test_support.check_warnings(('', RuntimeWarning)):
                # defaults to quiet=False with argument
                pass
        with self.assertRaises(AssertionError):
            with test_support.check_warnings(('foo', RuntimeWarning)):
                wmod.warn("foo")


class CCatchWarningTests(CatchWarningTests):
    module = c_warnings

class PyCatchWarningTests(CatchWarningTests):
    module = py_warnings


class EnvironmentVariableTests(BaseTest):

    def test_single_warning(self):
        newenv = os.environ.copy()
        newenv["PYTHONWARNINGS"] = "ignore::DeprecationWarning"
        p = subprocess.Popen([sys.executable,
                "-c", "import sys; sys.stdout.write(str(sys.warnoptions))"],
                stdout=subprocess.PIPE, env=newenv)
        self.assertEqual(p.communicate()[0], "['ignore::DeprecationWarning']")
        self.assertEqual(p.wait(), 0)

    def test_comma_separated_warnings(self):
        newenv = os.environ.copy()
        newenv["PYTHONWARNINGS"] = ("ignore::DeprecationWarning,"
                                    "ignore::UnicodeWarning")
        p = subprocess.Popen([sys.executable,
                "-c", "import sys; sys.stdout.write(str(sys.warnoptions))"],
                stdout=subprocess.PIPE, env=newenv)
        self.assertEqual(p.communicate()[0],
                "['ignore::DeprecationWarning', 'ignore::UnicodeWarning']")
        self.assertEqual(p.wait(), 0)

    def test_envvar_and_command_line(self):
        newenv = os.environ.copy()
        newenv["PYTHONWARNINGS"] = "ignore::DeprecationWarning"
        p = subprocess.Popen([sys.executable, "-W" "ignore::UnicodeWarning",
                "-c", "import sys; sys.stdout.write(str(sys.warnoptions))"],
                stdout=subprocess.PIPE, env=newenv)
        self.assertEqual(p.communicate()[0],
                "['ignore::UnicodeWarning', 'ignore::DeprecationWarning']")
        self.assertEqual(p.wait(), 0)

class CEnvironmentVariableTests(EnvironmentVariableTests):
    module = c_warnings

class PyEnvironmentVariableTests(EnvironmentVariableTests):
    module = py_warnings


def test_main():
    py_warnings.onceregistry.clear()
    c_warnings.onceregistry.clear()
    test_support.run_unittest(CFilterTests, PyFilterTests,
                                CWarnTests, PyWarnTests,
                                CWCmdLineTests, PyWCmdLineTests,
                                _WarningsTests,
                                CWarningsDisplayTests, PyWarningsDisplayTests,
                                CCatchWarningTests, PyCatchWarningTests,
                                CEnvironmentVariableTests,
                                PyEnvironmentVariableTests
                             )


if __name__ == "__main__":
    test_main()
