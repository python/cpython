import pipes
import os
import string
import unittest
from test.test_support import TESTFN, run_unittest, unlink, TestSkipped

if os.name != 'posix':
    raise TestSkipped('pipes module only works on posix')

TESTFN2 = TESTFN + "2"

class SimplePipeTests(unittest.TestCase):
    def tearDown(self):
        for f in (TESTFN, TESTFN2):
            unlink(f)

    def testSimplePipe1(self):
        t = pipes.Template()
        t.append('tr a-z A-Z', pipes.STDIN_STDOUT)
        f = t.open(TESTFN, 'w')
        f.write('hello world #1')
        f.close()
        self.assertEqual(open(TESTFN).read(), 'HELLO WORLD #1')

    def testSimplePipe2(self):
        file(TESTFN, 'w').write('hello world #2')
        t = pipes.Template()
        t.append('tr a-z A-Z < $IN > $OUT', pipes.FILEIN_FILEOUT)
        t.copy(TESTFN, TESTFN2)
        self.assertEqual(open(TESTFN2).read(), 'HELLO WORLD #2')

    def testSimplePipe3(self):
        file(TESTFN, 'w').write('hello world #2')
        t = pipes.Template()
        t.append('tr a-z A-Z < $IN', pipes.FILEIN_STDOUT)
        self.assertEqual(t.open(TESTFN, 'r').read(), 'HELLO WORLD #2')

    def testEmptyPipeline1(self):
        # copy through empty pipe
        d = 'empty pipeline test COPY'
        file(TESTFN, 'w').write(d)
        file(TESTFN2, 'w').write('')
        t=pipes.Template()
        t.copy(TESTFN, TESTFN2)
        self.assertEqual(open(TESTFN2).read(), d)

    def testEmptyPipeline2(self):
        # read through empty pipe
        d = 'empty pipeline test READ'
        file(TESTFN, 'w').write(d)
        t=pipes.Template()
        self.assertEqual(t.open(TESTFN, 'r').read(), d)

    def testEmptyPipeline3(self):
        # write through empty pipe
        d = 'empty pipeline test WRITE'
        t = pipes.Template()
        t.open(TESTFN, 'w').write(d)
        self.assertEqual(open(TESTFN).read(), d)

    def testQuoting(self):
        safeunquoted = string.ascii_letters + string.digits + '!@%_-+=:,./'
        unsafe = '"`$\\'

        self.assertEqual(pipes.quote(safeunquoted), safeunquoted)
        self.assertEqual(pipes.quote('test file name'), "'test file name'")
        for u in unsafe:
            self.assertEqual(pipes.quote('test%sname' % u),
                              "'test%sname'" % u)
        for u in unsafe:
            self.assertEqual(pipes.quote("test%s'name'" % u),
                              '"test\\%s\'name\'"' % u)

    def testRepr(self):
        t = pipes.Template()
        self.assertEqual(repr(t), "<Template instance, steps=[]>")
        t.append('tr a-z A-Z', pipes.STDIN_STDOUT)
        self.assertEqual(repr(t),
                    "<Template instance, steps=[('tr a-z A-Z', '--')]>")

    def testSetDebug(self):
        t = pipes.Template()
        t.debug(False)
        self.assertEqual(t.debugging, False)
        t.debug(True)
        self.assertEqual(t.debugging, True)

    def testReadOpenSink(self):
        # check calling open('r') on a pipe ending with
        # a sink raises ValueError
        t = pipes.Template()
        t.append('boguscmd', pipes.SINK)
        self.assertRaises(ValueError, t.open, 'bogusfile', 'r')

    def testWriteOpenSource(self):
        # check calling open('w') on a pipe ending with
        # a source raises ValueError
        t = pipes.Template()
        t.prepend('boguscmd', pipes.SOURCE)
        self.assertRaises(ValueError, t.open, 'bogusfile', 'w')

    def testBadAppendOptions(self):
        t = pipes.Template()

        # try a non-string command
        self.assertRaises(TypeError, t.append, 7, pipes.STDIN_STDOUT)

        # try a type that isn't recognized
        self.assertRaises(ValueError, t.append, 'boguscmd', 'xx')

        # shouldn't be able to append a source
        self.assertRaises(ValueError, t.append, 'boguscmd', pipes.SOURCE)

        # check appending two sinks
        t = pipes.Template()
        t.append('boguscmd', pipes.SINK)
        self.assertRaises(ValueError, t.append, 'boguscmd', pipes.SINK)

        # command needing file input but with no $IN
        t = pipes.Template()
        self.assertRaises(ValueError, t.append, 'boguscmd $OUT',
                           pipes.FILEIN_FILEOUT)
        t = pipes.Template()
        self.assertRaises(ValueError, t.append, 'boguscmd',
                           pipes.FILEIN_STDOUT)

        # command needing file output but with no $OUT
        t = pipes.Template()
        self.assertRaises(ValueError, t.append, 'boguscmd $IN',
                           pipes.FILEIN_FILEOUT)
        t = pipes.Template()
        self.assertRaises(ValueError, t.append, 'boguscmd',
                           pipes.STDIN_FILEOUT)


    def testBadPrependOptions(self):
        t = pipes.Template()

        # try a non-string command
        self.assertRaises(TypeError, t.prepend, 7, pipes.STDIN_STDOUT)

        # try a type that isn't recognized
        self.assertRaises(ValueError, t.prepend, 'tr a-z A-Z', 'xx')

        # shouldn't be able to prepend a sink
        self.assertRaises(ValueError, t.prepend, 'boguscmd', pipes.SINK)

        # check prepending two sources
        t = pipes.Template()
        t.prepend('boguscmd', pipes.SOURCE)
        self.assertRaises(ValueError, t.prepend, 'boguscmd', pipes.SOURCE)

        # command needing file input but with no $IN
        t = pipes.Template()
        self.assertRaises(ValueError, t.prepend, 'boguscmd $OUT',
                           pipes.FILEIN_FILEOUT)
        t = pipes.Template()
        self.assertRaises(ValueError, t.prepend, 'boguscmd',
                           pipes.FILEIN_STDOUT)

        # command needing file output but with no $OUT
        t = pipes.Template()
        self.assertRaises(ValueError, t.prepend, 'boguscmd $IN',
                           pipes.FILEIN_FILEOUT)
        t = pipes.Template()
        self.assertRaises(ValueError, t.prepend, 'boguscmd',
                           pipes.STDIN_FILEOUT)

    def testBadOpenMode(self):
        t = pipes.Template()
        self.assertRaises(ValueError, t.open, 'bogusfile', 'x')

    def testClone(self):
        t = pipes.Template()
        t.append('tr a-z A-Z', pipes.STDIN_STDOUT)

        u = t.clone()
        self.assertNotEqual(id(t), id(u))
        self.assertEqual(t.steps, u.steps)
        self.assertNotEqual(id(t.steps), id(u.steps))
        self.assertEqual(t.debugging, u.debugging)

def test_main():
    run_unittest(SimplePipeTests)

if __name__ == "__main__":
    test_main()
