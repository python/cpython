# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

from __future__ import with_statement
import string
import sys
import time
import random
import unittest
from test import test_support as support
try:
    import thread
    import threading
except ImportError:
    thread = None
    threading = None
# Skip this test if the _testcapi module isn't available.
_testcapi = support.import_module('_testcapi')

class CAPITest(unittest.TestCase):

    def test_buildvalue_N(self):
        _testcapi.test_buildvalue_N()


@unittest.skipUnless(threading, 'Threading required for this test.')
class TestPendingCalls(unittest.TestCase):

    def pendingcalls_submit(self, l, n):
        def callback():
            #this function can be interrupted by thread switching so let's
            #use an atomic operation
            l.append(None)

        for i in range(n):
            time.sleep(random.random()*0.02) #0.01 secs on average
            #try submitting callback until successful.
            #rely on regular interrupt to flush queue if we are
            #unsuccessful.
            while True:
                if _testcapi._pending_threadfunc(callback):
                    break;

    def pendingcalls_wait(self, l, n, context = None):
        #now, stick around until l[0] has grown to 10
        count = 0;
        while len(l) != n:
            #this busy loop is where we expect to be interrupted to
            #run our callbacks.  Note that callbacks are only run on the
            #main thread
            if False and support.verbose:
                print "(%i)"%(len(l),),
            for i in xrange(1000):
                a = i*i
            if context and not context.event.is_set():
                continue
            count += 1
            self.assertTrue(count < 10000,
                "timeout waiting for %i callbacks, got %i"%(n, len(l)))
        if False and support.verbose:
            print "(%i)"%(len(l),)

    def test_pendingcalls_threaded(self):
        #do every callback on a separate thread
        n = 32 #total callbacks
        threads = []
        class foo(object):pass
        context = foo()
        context.l = []
        context.n = 2 #submits per thread
        context.nThreads = n // context.n
        context.nFinished = 0
        context.lock = threading.Lock()
        context.event = threading.Event()

        threads = [threading.Thread(target=self.pendingcalls_thread,
                                    args=(context,))
                   for i in range(context.nThreads)]
        with support.start_threads(threads):
            self.pendingcalls_wait(context.l, n, context)

    def pendingcalls_thread(self, context):
        try:
            self.pendingcalls_submit(context.l, context.n)
        finally:
            with context.lock:
                context.nFinished += 1
                nFinished = context.nFinished
                if False and support.verbose:
                    print "finished threads: ", nFinished
            if nFinished == context.nThreads:
                context.event.set()

    def test_pendingcalls_non_threaded(self):
        #again, just using the main thread, likely they will all be dispatched at
        #once.  It is ok to ask for too many, because we loop until we find a slot.
        #the loop can be interrupted to dispatch.
        #there are only 32 dispatch slots, so we go for twice that!
        l = []
        n = 64
        self.pendingcalls_submit(l, n)
        self.pendingcalls_wait(l, n)


# Bug #6012
class Test6012(unittest.TestCase):
    def test(self):
        self.assertEqual(_testcapi.argparsing("Hello", "World"), 1)


class SkipitemTest(unittest.TestCase):

    def test_skipitem(self):
        """
        If this test failed, you probably added a new "format unit"
        in Python/getargs.c, but neglected to update our poor friend
        skipitem() in the same file.  (If so, shame on you!)

        With a few exceptions**, this function brute-force tests all
        printable ASCII*** characters (32 to 126 inclusive) as format units,
        checking to see that PyArg_ParseTupleAndKeywords() return consistent
        errors both when the unit is attempted to be used and when it is
        skipped.  If the format unit doesn't exist, we'll get one of two
        specific error messages (one for used, one for skipped); if it does
        exist we *won't* get that error--we'll get either no error or some
        other error.  If we get the specific "does not exist" error for one
        test and not for the other, there's a mismatch, and the test fails.

           ** Some format units have special funny semantics and it would
              be difficult to accommodate them here.  Since these are all
              well-established and properly skipped in skipitem() we can
              get away with not testing them--this test is really intended
              to catch *new* format units.

          *** Python C source files must be ASCII.  Therefore it's impossible
              to have non-ASCII format units.

        """
        empty_tuple = ()
        tuple_1 = (0,)
        dict_b = {'b':1}
        keywords = ["a", "b"]

        for i in range(32, 127):
            c = chr(i)

            # skip parentheses, the error reporting is inconsistent about them
            # skip 'e', it's always a two-character code
            # skip '|', it doesn't represent arguments anyway
            if c in '()e|':
                continue

            # test the format unit when not skipped
            format = c + "i"
            try:
                _testcapi.parse_tuple_and_keywords(tuple_1, dict_b,
                    format, keywords)
                when_not_skipped = False
            except TypeError as e:
                s = "argument 1 (impossible<bad format char>)"
                when_not_skipped = (str(e) == s)
            except RuntimeError:
                when_not_skipped = False

            # test the format unit when skipped
            optional_format = "|" + format
            try:
                _testcapi.parse_tuple_and_keywords(empty_tuple, dict_b,
                    optional_format, keywords)
                when_skipped = False
            except RuntimeError as e:
                s = "impossible<bad format char>: '{}'".format(format)
                when_skipped = (str(e) == s)

            message = ("test_skipitem_parity: "
                "detected mismatch between convertsimple and skipitem "
                "for format unit '{}' ({}), not skipped {}, skipped {}".format(
                    c, i, when_skipped, when_not_skipped))
            self.assertIs(when_skipped, when_not_skipped, message)

    def test_skipitem_with_suffix(self):
        parse = _testcapi.parse_tuple_and_keywords
        empty_tuple = ()
        tuple_1 = (0,)
        dict_b = {'b':1}
        keywords = ["a", "b"]

        supported = ('s#', 's*', 'z#', 'z*', 'u#', 't#', 'w#', 'w*')
        for c in string.ascii_letters:
            for c2 in '#*':
                f = c + c2
                optional_format = "|" + f + "i"
                if f in supported:
                    parse(empty_tuple, dict_b, optional_format, keywords)
                else:
                    with self.assertRaisesRegexp((RuntimeError, TypeError),
                                'impossible<bad format char>'):
                        parse(empty_tuple, dict_b, optional_format, keywords)

        for c in map(chr, range(32, 128)):
            f = 'e' + c
            optional_format = "|" + f + "i"
            if c in 'st':
                parse(empty_tuple, dict_b, optional_format, keywords)
            else:
                with self.assertRaisesRegexp(RuntimeError,
                            'impossible<bad format char>'):
                    parse(empty_tuple, dict_b, optional_format, keywords)

    def test_parse_tuple_and_keywords(self):
        # Test handling errors in the parse_tuple_and_keywords helper itself
        self.assertRaises(TypeError, _testcapi.parse_tuple_and_keywords,
                          (), {}, 42, [])
        self.assertRaises(ValueError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', 42)
        self.assertRaises(ValueError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', [''] * 42)
        self.assertRaises(TypeError, _testcapi.parse_tuple_and_keywords,
                          (), {}, '', [42])

    def test_bad_use(self):
        # Test handling invalid format and keywords in
        # PyArg_ParseTupleAndKeywords()
        self.assertRaises(TypeError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '||O', ['a'])
        self.assertRaises(RuntimeError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '|O', ['a', 'b'])
        self.assertRaises(RuntimeError, _testcapi.parse_tuple_and_keywords,
                          (1,), {}, '|OO', ['a'])


@unittest.skipUnless(threading and thread, 'Threading required for this test.')
class TestThreadState(unittest.TestCase):

    @support.reap_threads
    def test_thread_state(self):
        # some extra thread-state tests driven via _testcapi
        def target():
            idents = []

            def callback():
                idents.append(thread.get_ident())

            _testcapi._test_thread_state(callback)
            a = b = callback
            time.sleep(1)
            # Check our main thread is in the list exactly 3 times.
            self.assertEqual(idents.count(thread.get_ident()), 3,
                             "Couldn't find main thread correctly in the list")

        target()
        t = threading.Thread(target=target)
        t.start()
        t.join()


def test_main():
    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if support.verbose:
                print "internal", name
            try:
                test()
            except _testcapi.error:
                raise support.TestFailed, sys.exc_info()[1]

    support.run_unittest(CAPITest, TestPendingCalls, SkipitemTest,
                         TestThreadState)

if __name__ == "__main__":
    test_main()
