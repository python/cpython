from test.support import temp_dir
from test.support.script_helper import assert_python_failure
import unittest
import sys
import cgitb
from contextlib import ExitStack
from unittest import mock

class TestCgitb(unittest.TestCase):

    def mock_sys(self):
        "Mock system environment for cgitb"
        # use exit stack to match patch context managers to addCleanup
        stack = ExitStack()
        self.addCleanup(stack.close)
        self.stdout = stack.enter_context(mock.patch('cgitb.sys.stdout'))
        self.stderr = stack.enter_context(mock.patch('cgitb.sys.stderr'))
        prepatch = mock.patch('cgitb.sys', wraps=cgitb.sys, spec=cgitb.sys)
        self.sysmod = stack.enter_context(prepatch)
        self.sysmod.excepthook = self.sysmod.__excepthook__

    def test_fonts(self):
        text = "Hello Robbie!"
        self.assertEqual(cgitb.small(text), "<small>{}</small>".format(text))
        self.assertEqual(cgitb.strong(text), "<strong>{}</strong>".format(text))
        self.assertEqual(cgitb.grey(text),
                         '<font color="#909090">{}</font>'.format(text))

    def test_blanks(self):
        self.assertEqual(cgitb.small(""), "")
        self.assertEqual(cgitb.strong(""), "")
        self.assertEqual(cgitb.grey(""), "")

    def test_html(self):
        try:
            raise ValueError("Hello World")
        except ValueError as err:
            # If the html was templated we could do a bit more here.
            # At least check that we get details on what we just raised.
            html = cgitb.html(sys.exc_info())
            self.assertIn("ValueError", html)
            self.assertIn(str(err), html)

    def test_text(self):
        try:
            raise ValueError("Hello World")
        except ValueError as err:
            text = cgitb.text(sys.exc_info())
            self.assertIn("ValueError", text)
            self.assertIn("Hello World", text)

    def test_syshook_no_logdir_default_format(self):
        with temp_dir() as tracedir:
            rc, out, err = assert_python_failure(
                  '-c',
                  ('import cgitb; cgitb.enable(logdir=%s); '
                   'raise ValueError("Hello World")') % repr(tracedir))
        out = out.decode(sys.getfilesystemencoding())
        self.assertIn("ValueError", out)
        self.assertIn("Hello World", out)
        self.assertIn("<strong>&lt;module&gt;</strong>", out)
        # By default we emit HTML markup.
        self.assertIn('<p>', out)
        self.assertIn('</p>', out)

    def test_syshook_no_logdir_text_format(self):
        # Issue 12890: we were emitting the <p> tag in text mode.
        with temp_dir() as tracedir:
            rc, out, err = assert_python_failure(
                  '-c',
                  ('import cgitb; cgitb.enable(format="text", logdir=%s); '
                   'raise ValueError("Hello World")') % repr(tracedir))
        out = out.decode(sys.getfilesystemencoding())
        self.assertIn("ValueError", out)
        self.assertIn("Hello World", out)
        self.assertNotIn('<p>', out)
        self.assertNotIn('</p>', out)

    def test_text_displayed(self):
        self.mock_sys()
        cgitb.enable(display=1, format="text")
        try:
            raise ValueError("Just a little scratch!")
        except:
            cgitb.sys.excepthook(*self.sysmod.exc_info())
        self.assertEqual(self.stdout.write.call_count, 1)
        written = self.stdout.write.call_args[0][0]
        self.assertIn("ValueError", written)
        self.assertNotRegex(written, "^<p>")

    def test_text_suppressed(self):
        self.mock_sys()
        cgitb.enable(display=0, format="text")
        try:
            raise ValueError("Just a little scratch!")
        except:
            cgitb.sys.excepthook(*self.sysmod.exc_info())
        self.assertEqual(self.stdout.write.call_count, 1)
        written = self.stdout.write.call_args[0][0]
        self.assertNotIn("ValueError", written)
        self.assertNotRegex(written, "^<p>")

    def test_html_displayed(self):
        self.mock_sys()
        cgitb.enable(display=1, format="html")
        try:
            raise ValueError("Just a little scratch!")
        except:
            cgitb.sys.excepthook(*self.sysmod.exc_info())
        self.assertGreater(self.stdout.write.call_count, 0)
        written = self.stdout.write.call_args[0][0]
        self.assertIn("ValueError", written)
        self.assertRegex(written, "^<body")

    def test_html_suppressed(self):
        self.mock_sys()
        cgitb.enable(display=0, format="html")
        try:
            raise ValueError("Just a little scratch!")
        except:
            cgitb.sys.excepthook(*self.sysmod.exc_info())
        self.assertGreater(self.stdout.write.call_count, 0)
        written = self.stdout.write.call_args[0][0]
        self.assertNotIn("ValueError", written)
        self.assertRegex(written, "^<p>")



if __name__ == "__main__":
    unittest.main()
