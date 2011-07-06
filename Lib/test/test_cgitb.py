from test.support import run_unittest
import unittest
import sys
import subprocess
import cgitb

class TestCgitb(unittest.TestCase):
    
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
            
    def test_hook(self):
        proc = subprocess.Popen([sys.executable, '-c',
                                 ('import cgitb;' 
                                  'cgitb.enable();' 
                                  'raise ValueError("Hello World")')],
                                stdout=subprocess.PIPE)
        out = proc.stdout.read().decode(sys.getfilesystemencoding())
        self.addCleanup(proc.stdout.close)
        self.assertIn("ValueError", out)
        self.assertIn("Hello World", out)


def test_main():
    run_unittest(TestCgitb)
    
if __name__ == "__main__":
    test_main()
