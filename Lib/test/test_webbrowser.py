import io
import os
import re
import shlex
import subprocess
import sys
import unittest
import webbrowser
from functools import partial
from test import support
from test.support import import_helper
from test.support import is_apple_mobile
from test.support import os_helper
from test.support import requires_subprocess
from test.support import threading_helper
from unittest import mock

# The webbrowser module uses threading locks
threading_helper.requires_working_threading(module=True)

URL = 'https://www.example.com'
CMD_NAME = 'test'


class PopenMock(mock.MagicMock):

    def poll(self):
        return 0

    def wait(self, seconds=None):
        return 0


@requires_subprocess()
class CommandTestMixin:

    def _test(self, meth, *, args=[URL], kw={}, options, arguments):
        """Given a web browser instance method name along with arguments and
        keywords for same (which defaults to the single argument URL), creates
        a browser instance from the class pointed to by self.browser, calls the
        indicated instance method with the indicated arguments, and compares
        the resulting options and arguments passed to Popen by the browser
        instance against the 'options' and 'args' lists.  Options are compared
        in a position independent fashion, and the arguments are compared in
        sequence order to whatever is left over after removing the options.

        """
        popen = PopenMock()
        support.patch(self, subprocess, 'Popen', popen)
        browser = self.browser_class(name=CMD_NAME)
        getattr(browser, meth)(*args, **kw)
        popen_args = subprocess.Popen.call_args[0][0]
        self.assertEqual(popen_args[0], CMD_NAME)
        popen_args.pop(0)
        for option in options:
            self.assertIn(option, popen_args)
            popen_args.pop(popen_args.index(option))
        self.assertEqual(popen_args, arguments)


class GenericBrowserCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.GenericBrowser

    def test_open(self):
        self._test('open',
                   options=[],
                   arguments=[URL])


class BackgroundBrowserCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.BackgroundBrowser

    def test_open(self):
        self._test('open',
                   options=[],
                   arguments=[URL])


class ChromeCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.Chrome

    def test_open(self):
        self._test('open',
                   options=[],
                   arguments=[URL])

    def test_open_with_autoraise_false(self):
        self._test('open', kw=dict(autoraise=False),
                   options=[],
                   arguments=[URL])

    def test_open_new(self):
        self._test('open_new',
                   options=['--new-window'],
                   arguments=[URL])

    def test_open_new_tab(self):
        self._test('open_new_tab',
                   options=[],
                   arguments=[URL])

    def test_open_bad_new_parameter(self):
        with self.assertRaisesRegex(webbrowser.Error,
                                    re.escape("Bad 'new' parameter to open(); "
                                              "expected 0, 1, or 2, got 999")):
            self._test('open',
                       options=[],
                       arguments=[URL],
                       kw=dict(new=999))


class EdgeCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.Edge

    def test_open(self):
        self._test('open',
                   options=[],
                   arguments=[URL])

    def test_open_with_autoraise_false(self):
        self._test('open', kw=dict(autoraise=False),
                   options=[],
                   arguments=[URL])

    def test_open_new(self):
        self._test('open_new',
                   options=['--new-window'],
                   arguments=[URL])

    def test_open_new_tab(self):
        self._test('open_new_tab',
                   options=[],
                   arguments=[URL])


class MozillaCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.Mozilla

    def test_open(self):
        self._test('open',
                   options=[],
                   arguments=[URL])

    def test_open_with_autoraise_false(self):
        self._test('open', kw=dict(autoraise=False),
                   options=[],
                   arguments=[URL])

    def test_open_new(self):
        self._test('open_new',
                   options=[],
                   arguments=['-new-window', URL])

    def test_open_new_tab(self):
        self._test('open_new_tab',
                   options=[],
                   arguments=['-new-tab', URL])


class EpiphanyCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.Epiphany

    def test_open(self):
        self._test('open',
                   options=['-n'],
                   arguments=[URL])

    def test_open_with_autoraise_false(self):
        self._test('open', kw=dict(autoraise=False),
                   options=['-noraise', '-n'],
                   arguments=[URL])

    def test_open_new(self):
        self._test('open_new',
                   options=['-w'],
                   arguments=[URL])

    def test_open_new_tab(self):
        self._test('open_new_tab',
                   options=['-w'],
                   arguments=[URL])


class OperaCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.Opera

    def test_open(self):
        self._test('open',
                   options=[],
                   arguments=[URL])

    def test_open_with_autoraise_false(self):
        self._test('open', kw=dict(autoraise=False),
                   options=[],
                   arguments=[URL])

    def test_open_new(self):
        self._test('open_new',
                   options=['--new-window'],
                   arguments=[URL])

    def test_open_new_tab(self):
        self._test('open_new_tab',
                   options=[],
                   arguments=[URL])


class ELinksCommandTest(CommandTestMixin, unittest.TestCase):

    browser_class = webbrowser.Elinks

    def test_open(self):
        self._test('open', options=['-remote'],
                   arguments=[f'openURL({URL})'])

    def test_open_with_autoraise_false(self):
        self._test('open',
                   options=['-remote'],
                   arguments=[f'openURL({URL})'])

    def test_open_new(self):
        self._test('open_new',
                   options=['-remote'],
                   arguments=[f'openURL({URL},new-window)'])

    def test_open_new_tab(self):
        self._test('open_new_tab',
                   options=['-remote'],
                   arguments=[f'openURL({URL},new-tab)'])


@unittest.skipUnless(sys.platform == "ios", "Test only applicable to iOS")
class IOSBrowserTest(unittest.TestCase):
    def _obj_ref(self, *args):
        # Construct a string representation of the arguments that can be used
        # as a proxy for object instance references
        return "|".join(str(a) for a in args)

    @unittest.skipIf(getattr(webbrowser, "objc", None) is None,
                     "iOS Webbrowser tests require ctypes")
    def setUp(self):
        # Intercept the objc library. Wrap the calls to get the
        # references to classes and selectors to return strings, and
        # wrap msgSend to return stringified object references
        self.orig_objc = webbrowser.objc

        webbrowser.objc = mock.Mock()
        webbrowser.objc.objc_getClass = lambda cls: f"C#{cls.decode()}"
        webbrowser.objc.sel_registerName = lambda sel: f"S#{sel.decode()}"
        webbrowser.objc.objc_msgSend.side_effect = self._obj_ref

    def tearDown(self):
        webbrowser.objc = self.orig_objc

    def _test(self, meth, **kwargs):
        # The browser always gets focus, there's no concept of separate browser
        # windows, and there's no API-level control over creating a new tab.
        # Therefore, all calls to webbrowser are effectively the same.
        getattr(webbrowser, meth)(URL, **kwargs)

        # The ObjC String version of the URL is created with UTF-8 encoding
        url_string_args = [
            "C#NSString",
            "S#stringWithCString:encoding:",
            b'https://www.example.com',
            4,
        ]
        # The NSURL version of the URL is created from that string
        url_obj_args = [
            "C#NSURL",
            "S#URLWithString:",
            self._obj_ref(*url_string_args),
        ]
        # The openURL call is invoked on the shared application
        shared_app_args = ["C#UIApplication", "S#sharedApplication"]

        # Verify that the last call is the one that opens the URL.
        webbrowser.objc.objc_msgSend.assert_called_with(
            self._obj_ref(*shared_app_args),
            "S#openURL:options:completionHandler:",
            self._obj_ref(*url_obj_args),
            None,
            None
        )

    def test_open(self):
        self._test('open')

    def test_open_with_autoraise_false(self):
        self._test('open', autoraise=False)

    def test_open_new(self):
        self._test('open_new')

    def test_open_new_tab(self):
        self._test('open_new_tab')


class MockPopenPipe:
    def __init__(self, cmd, mode):
        self.cmd = cmd
        self.mode = mode
        self.pipe = io.StringIO()
        self._closed = False

    def write(self, buf):
        self.pipe.write(buf)

    def close(self):
        self._closed = True
        return None


@unittest.skipUnless(sys.platform == "darwin", "macOS specific test")
@requires_subprocess()
class MacOSXOSAScriptTest(unittest.TestCase):

    def setUp(self):
        # Ensure that 'BROWSER' is not set to 'open' or something else.
        # See: https://github.com/python/cpython/issues/131254.
        env = self.enterContext(os_helper.EnvironmentVarGuard())
        env.unset("BROWSER")

        support.patch(self, os, "popen", self.mock_popen)
        self.browser = webbrowser.MacOSXOSAScript("default")

    def mock_popen(self, cmd, mode):
        self.popen_pipe = MockPopenPipe(cmd, mode)
        return self.popen_pipe

    def test_default(self):
        browser = webbrowser.get()
        assert isinstance(browser, webbrowser.MacOSXOSAScript)
        self.assertEqual(browser.name, "default")

    def test_default_open(self):
        url = "https://python.org"
        self.browser.open(url)
        self.assertTrue(self.popen_pipe._closed)
        self.assertEqual(self.popen_pipe.cmd, "osascript")
        script = self.popen_pipe.pipe.getvalue()
        self.assertEqual(script.strip(), f'open location "{url}"')

    def test_url_quote(self):
        self.browser.open('https://python.org/"quote"')
        script = self.popen_pipe.pipe.getvalue()
        self.assertEqual(
            script.strip(), 'open location "https://python.org/%22quote%22"'
        )

    def test_default_browser_lookup(self):
        url = "file:///tmp/some-file.html"
        self.browser.open(url)
        script = self.popen_pipe.pipe.getvalue()
        # doesn't actually test the browser lookup works,
        # just that the branch is taken
        self.assertIn("URLForApplicationToOpenURL", script)
        self.assertIn(f'open location "{url}"', script)

    def test_explicit_browser(self):
        browser = webbrowser.MacOSXOSAScript("safari")
        browser.open("https://python.org")
        script = self.popen_pipe.pipe.getvalue()
        self.assertIn('tell application "safari"', script)
        self.assertIn('open location "https://python.org"', script)


class BrowserRegistrationTest(unittest.TestCase):

    def setUp(self):
        # Ensure we don't alter the real registered browser details
        self._saved_tryorder = webbrowser._tryorder
        webbrowser._tryorder = []
        self._saved_browsers = webbrowser._browsers
        webbrowser._browsers = {}

    def tearDown(self):
        webbrowser._tryorder = self._saved_tryorder
        webbrowser._browsers = self._saved_browsers

    def _check_registration(self, preferred):
        class ExampleBrowser:
            pass

        expected_tryorder = []
        expected_browsers = {}

        self.assertEqual(webbrowser._tryorder, expected_tryorder)
        self.assertEqual(webbrowser._browsers, expected_browsers)

        webbrowser.register('Example1', ExampleBrowser)
        expected_tryorder = ['Example1']
        expected_browsers['example1'] = [ExampleBrowser, None]
        self.assertEqual(webbrowser._tryorder, expected_tryorder)
        self.assertEqual(webbrowser._browsers, expected_browsers)

        instance = ExampleBrowser()
        if preferred is not None:
            webbrowser.register('example2', ExampleBrowser, instance,
                                preferred=preferred)
        else:
            webbrowser.register('example2', ExampleBrowser, instance)
        if preferred:
            expected_tryorder = ['example2', 'Example1']
        else:
            expected_tryorder = ['Example1', 'example2']
        expected_browsers['example2'] = [ExampleBrowser, instance]
        self.assertEqual(webbrowser._tryorder, expected_tryorder)
        self.assertEqual(webbrowser._browsers, expected_browsers)

    def test_register(self):
        self._check_registration(preferred=False)

    def test_register_default(self):
        self._check_registration(preferred=None)

    def test_register_preferred(self):
        self._check_registration(preferred=True)

    @unittest.skipUnless(sys.platform == "darwin", "macOS specific test")
    def test_no_xdg_settings_on_macOS(self):
        # On macOS webbrowser should not use xdg-settings to
        # look for X11 based browsers (for those users with
        # XQuartz installed)
        with mock.patch("subprocess.check_output") as ck_o:
            webbrowser.register_standard_browsers()

        ck_o.assert_not_called()


class ImportTest(unittest.TestCase):
    def test_register(self):
        webbrowser = import_helper.import_fresh_module('webbrowser')
        self.assertIsNone(webbrowser._tryorder)
        self.assertFalse(webbrowser._browsers)

        class ExampleBrowser:
            pass
        webbrowser.register('Example1', ExampleBrowser)
        self.assertTrue(webbrowser._tryorder)
        self.assertEqual(webbrowser._tryorder[-1], 'Example1')
        self.assertTrue(webbrowser._browsers)
        self.assertIn('example1', webbrowser._browsers)
        self.assertEqual(webbrowser._browsers['example1'], [ExampleBrowser, None])

    def test_get(self):
        webbrowser = import_helper.import_fresh_module('webbrowser')
        self.assertIsNone(webbrowser._tryorder)
        self.assertFalse(webbrowser._browsers)

        with self.assertRaises(webbrowser.Error):
            webbrowser.get('fakebrowser')
        self.assertIsNotNone(webbrowser._tryorder)

    @unittest.skipIf(" " in sys.executable, "test assumes no space in path (GH-114452)")
    def test_synthesize(self):
        webbrowser = import_helper.import_fresh_module('webbrowser')
        name = os.path.basename(sys.executable).lower()
        webbrowser.register(name, None, webbrowser.GenericBrowser(name))
        webbrowser.get(sys.executable)

    @unittest.skipIf(
        is_apple_mobile,
        "Apple mobile doesn't allow modifying browser with environment"
    )
    def test_environment(self):
        webbrowser = import_helper.import_fresh_module('webbrowser')
        try:
            browser = webbrowser.get().name
        except webbrowser.Error as err:
            self.skipTest(str(err))
        with os_helper.EnvironmentVarGuard() as env:
            env["BROWSER"] = browser
            webbrowser = import_helper.import_fresh_module('webbrowser')
            webbrowser.get()

    @unittest.skipIf(
        is_apple_mobile,
        "Apple mobile doesn't allow modifying browser with environment"
    )
    def test_environment_preferred(self):
        webbrowser = import_helper.import_fresh_module('webbrowser')
        try:
            webbrowser.get()
            least_preferred_browser = webbrowser.get(webbrowser._tryorder[-1]).name
        except (webbrowser.Error, IndexError) as err:
            self.skipTest(str(err))

        with os_helper.EnvironmentVarGuard() as env:
            env["BROWSER"] = least_preferred_browser
            webbrowser = import_helper.import_fresh_module('webbrowser')
            self.assertEqual(webbrowser.get().name, least_preferred_browser)

        with os_helper.EnvironmentVarGuard() as env:
            env["BROWSER"] = sys.executable
            webbrowser = import_helper.import_fresh_module('webbrowser')
            self.assertEqual(webbrowser.get().name, sys.executable)


class CliTest(unittest.TestCase):
    def test_parse_args(self):
        for command, url, new_win in [
            # No optional arguments
            ("https://example.com", "https://example.com", 0),
            # Each optional argument
            ("https://example.com -n", "https://example.com", 1),
            ("-n https://example.com", "https://example.com", 1),
            ("https://example.com -t", "https://example.com", 2),
            ("-t https://example.com", "https://example.com", 2),
            # Long form
            ("https://example.com --new-window", "https://example.com", 1),
            ("--new-window https://example.com", "https://example.com", 1),
            ("https://example.com --new-tab", "https://example.com", 2),
            ("--new-tab https://example.com", "https://example.com", 2),
        ]:
            args = webbrowser.parse_args(shlex.split(command))

            self.assertEqual(args.url, url)
            self.assertEqual(args.new_win, new_win)

    def test_parse_args_error(self):
        for command in [
            # Arguments must not both be given
            "https://example.com -n -t",
            "https://example.com --new-window --new-tab",
            "https://example.com -n --new-tab",
            "https://example.com --new-window -t",
        ]:
            with support.captured_stderr() as stderr:
                with self.assertRaises(SystemExit):
                    webbrowser.parse_args(shlex.split(command))
                self.assertIn(
                    'error: argument -t/--new-tab: not allowed with argument -n/--new-window',
                    stderr.getvalue(),
                )

        # Ensure ambiguous shortening fails
        with support.captured_stderr() as stderr:
            with self.assertRaises(SystemExit):
                webbrowser.parse_args(shlex.split("https://example.com --new"))
            self.assertIn(
                'error: ambiguous option: --new could match --new-window, --new-tab',
                stderr.getvalue()
            )

    def test_main(self):
        for command, expected_url, expected_new_win in [
            # No optional arguments
            ("https://example.com", "https://example.com", 0),
            # Each optional argument
            ("https://example.com -n", "https://example.com", 1),
            ("-n https://example.com", "https://example.com", 1),
            ("https://example.com -t", "https://example.com", 2),
            ("-t https://example.com", "https://example.com", 2),
            # Long form
            ("https://example.com --new-window", "https://example.com", 1),
            ("--new-window https://example.com", "https://example.com", 1),
            ("https://example.com --new-tab", "https://example.com", 2),
            ("--new-tab https://example.com", "https://example.com", 2),
        ]:
            with (
                mock.patch("webbrowser.open", return_value=None) as mock_open,
                mock.patch("builtins.print", return_value=None),
            ):
                webbrowser.main(shlex.split(command))
                mock_open.assert_called_once_with(expected_url, expected_new_win)


if __name__ == '__main__':
    unittest.main()
