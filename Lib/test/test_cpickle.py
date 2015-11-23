import cPickle
import cStringIO
import io
import functools
import unittest
from test.pickletester import (AbstractUnpickleTests,
                               AbstractPickleTests,
                               AbstractPickleModuleTests,
                               AbstractPicklerUnpicklerObjectTests,
                               BigmemPickleTests)
from test import test_support

class cStringIOMixin:
    output = input = cStringIO.StringIO

    def close(self, f):
        pass

class BytesIOMixin:
    output = input = io.BytesIO

    def close(self, f):
        pass

class FileIOMixin:

    def output(self):
        return open(test_support.TESTFN, 'wb+')

    def input(self, data):
        f = open(test_support.TESTFN, 'wb+')
        try:
            f.write(data)
            f.seek(0)
            return f
        except:
            f.close()
            raise

    def close(self, f):
        f.close()
        test_support.unlink(test_support.TESTFN)


class cPickleTests(AbstractUnpickleTests, AbstractPickleTests,
                   AbstractPickleModuleTests):

    def setUp(self):
        self.dumps = cPickle.dumps
        self.loads = cPickle.loads

    error = cPickle.BadPickleGet
    module = cPickle
    bad_stack_errors = (cPickle.UnpicklingError,)

class cPickleUnpicklerTests(AbstractUnpickleTests):

    def loads(self, buf):
        f = self.input(buf)
        try:
            p = cPickle.Unpickler(f)
            return p.load()
        finally:
            self.close(f)

    error = cPickle.BadPickleGet
    bad_stack_errors = (cPickle.UnpicklingError,)

class cStringIOCUnpicklerTests(cStringIOMixin, cPickleUnpicklerTests):
    pass

class BytesIOCUnpicklerTests(BytesIOMixin, cPickleUnpicklerTests):
    pass

class FileIOCUnpicklerTests(FileIOMixin, cPickleUnpicklerTests):
    pass


class cPicklePicklerTests(AbstractPickleTests):

    def dumps(self, arg, proto=0):
        f = self.output()
        try:
            p = cPickle.Pickler(f, proto)
            p.dump(arg)
            f.seek(0)
            return f.read()
        finally:
            self.close(f)

    def loads(self, buf):
        f = self.input(buf)
        try:
            p = cPickle.Unpickler(f)
            return p.load()
        finally:
            self.close(f)

class cStringIOCPicklerTests(cStringIOMixin, cPicklePicklerTests):
    pass

class BytesIOCPicklerTests(BytesIOMixin, cPicklePicklerTests):
    pass

class FileIOCPicklerTests(FileIOMixin, cPicklePicklerTests):
    pass


class cPickleListPicklerTests(AbstractPickleTests):

    def dumps(self, arg, proto=0):
        p = cPickle.Pickler(proto)
        p.dump(arg)
        return p.getvalue()

    def loads(self, *args):
        f = self.input(args[0])
        try:
            p = cPickle.Unpickler(f)
            return p.load()
        finally:
            self.close(f)

    error = cPickle.BadPickleGet

class cStringIOCPicklerListTests(cStringIOMixin, cPickleListPicklerTests):
    pass

class BytesIOCPicklerListTests(BytesIOMixin, cPickleListPicklerTests):
    pass

class FileIOCPicklerListTests(FileIOMixin, cPickleListPicklerTests):
    pass


class cPickleFastPicklerTests(AbstractPickleTests):

    def dumps(self, arg, proto=0):
        f = self.output()
        try:
            p = cPickle.Pickler(f, proto)
            p.fast = 1
            p.dump(arg)
            f.seek(0)
            return f.read()
        finally:
            self.close(f)

    def loads(self, *args):
        f = self.input(args[0])
        try:
            p = cPickle.Unpickler(f)
            return p.load()
        finally:
            self.close(f)

    def test_nonrecursive_deep(self):
        # If it's not cyclic, it should pickle OK even if the nesting
        # depth exceeds PY_CPICKLE_FAST_LIMIT.  That happens to be
        # 50 today.  Jack Jansen reported stack overflow on Mac OS 9
        # at 64.
        a = []
        for i in range(60):
            a = [a]
        b = self.loads(self.dumps(a))
        self.assertEqual(a, b)

for name in dir(AbstractPickleTests):
    if name.startswith('test_recursive_'):
        func = getattr(AbstractPickleTests, name)
        if '_subclass' in name and '_and_inst' not in name:
            assert_args = RuntimeError, 'maximum recursion depth exceeded'
        else:
            assert_args = ValueError, "can't pickle cyclic objects"
        def wrapper(self, func=func, assert_args=assert_args):
            with self.assertRaisesRegexp(*assert_args):
                func(self)
        functools.update_wrapper(wrapper, func)
        setattr(cPickleFastPicklerTests, name, wrapper)

class cStringIOCPicklerFastTests(cStringIOMixin, cPickleFastPicklerTests):
    pass

class BytesIOCPicklerFastTests(BytesIOMixin, cPickleFastPicklerTests):
    pass

class FileIOCPicklerFastTests(FileIOMixin, cPickleFastPicklerTests):
    pass


class cPicklePicklerUnpicklerObjectTests(AbstractPicklerUnpicklerObjectTests):

    pickler_class = cPickle.Pickler
    unpickler_class = cPickle.Unpickler

class cPickleBigmemPickleTests(BigmemPickleTests):

    def dumps(self, arg, proto=0, fast=0):
        # Ignore fast
        return cPickle.dumps(arg, proto)

    def loads(self, buf):
        # Ignore fast
        return cPickle.loads(buf)


class Node(object):
    pass

class cPickleDeepRecursive(unittest.TestCase):
    def test_issue2702(self):
        # This should raise a RecursionLimit but in some
        # platforms (FreeBSD, win32) sometimes raises KeyError instead,
        # or just silently terminates the interpreter (=crashes).
        nodes = [Node() for i in range(500)]
        for n in nodes:
            n.connections = list(nodes)
            n.connections.remove(n)
        self.assertRaises((AttributeError, RuntimeError), cPickle.dumps, n)

    def test_issue3179(self):
        # Safe test, because I broke this case when fixing the
        # behaviour for the previous test.
        res=[]
        for x in range(1,2000):
            res.append(dict(doc=x, similar=[]))
        cPickle.dumps(res)


def test_main():
    test_support.run_unittest(
        cPickleTests,
        cStringIOCUnpicklerTests,
        BytesIOCUnpicklerTests,
        FileIOCUnpicklerTests,
        cStringIOCPicklerTests,
        BytesIOCPicklerTests,
        FileIOCPicklerTests,
        cStringIOCPicklerListTests,
        BytesIOCPicklerListTests,
        FileIOCPicklerListTests,
        cStringIOCPicklerFastTests,
        BytesIOCPicklerFastTests,
        FileIOCPicklerFastTests,
        cPickleDeepRecursive,
        cPicklePicklerUnpicklerObjectTests,
        cPickleBigmemPickleTests,
    )

if __name__ == "__main__":
    test_main()
