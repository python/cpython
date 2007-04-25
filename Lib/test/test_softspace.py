from test.test_support import run_unittest
import unittest
import StringIO

class SoftspaceTests(unittest.TestCase):
    def test_bug_480215(self):
        # SF bug 480215:  softspace confused in nested print
        f = StringIO.StringIO()
        class C:
            def __str__(self):
                print >> f, 'a'
                return 'b'

        print >> f, C(), 'c ', 'd\t', 'e'
        print >> f, 'f', 'g'
        # In 2.2 & earlier, this printed ' a\nbc  d\te\nf g\n'
        self.assertEqual(f.getvalue(), 'a\nb c  d\te\nf g\n')

def test_main():
    run_unittest(SoftspaceTests)

if __name__ == '__main__':
    test_main()
