import array
import io
import threading
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently

threading_helper.requires_working_threading(module=True)


# The workers below take the shared array as their first argument.  In a
# debug build freed memory is filled with 0xdd, so that byte must never show
# up in a copy of an array of b'0'.

def pop1(a, *args):
    try:
        a.pop()
    except IndexError:
        pass

def append1(a, *args):
    a.append(2)

def insert1(a, *args):
    a.insert(0, 2)

def extend(a, *args):
    a.extend(array.array('i', [2]))

def inplace_concat(a, *args):
    a += array.array('i', [2])

def inplace_repeat(a, *args):
    a *= 2

def clear(a, *args):
    a.clear()

def remove1(a, *args):
    try:
        a.remove(1)
    except ValueError:
        pass

def fromunicode(a, *args):
    a.fromunicode('test')

def frombytes(a, *args):
    a.frombytes(b'0000')

def fromlist(a, *args):
    a.fromlist([2] * 50)

def setitem0(a, *args):
    try:
        a[0] = 0
    except IndexError:
        pass

def byteswap(a, *args):
    a.byteswap()

def reverse(a, *args):
    a.reverse()

def tounicode(a, *args):
    a.tounicode()

def tobytes(a, *args):
    a.tobytes()

def tolist(a, *args):
    a.tolist()

def tofile(a, *args):
    a.tofile(io.BytesIO())

def reduce_ex2(a, *args):
    a.__reduce_ex__(2)

def reduce_ex3(a, *args):
    c = a.__reduce_ex__(3)
    assert not c[1] or 0xdd not in c[1][3]

def copy(a, *args):
    c = a.__copy__()
    assert not c or 0xdd not in c

def repr1(a, *args):
    repr(a)

def repeat2(a, *args):
    a * 2

def count1(a, *args):
    a.count(1)

def index1(a, *args):
    try:
        a.index(1)
    except ValueError:
        pass

def contains1(a, *args):
    1 in a

def getitem0(a, *args):
    try:
        a[0]
    except IndexError:
        pass

def getslice(a, *args):
    a[:]
    a[::2]

def concat(a, *args):
    a + a

def richcmplhs(a, *args):
    a == a[:]

def richcmprhs(a, *args):
    a[:] == a

def new(a, *args):
    array.array(a.typecode, a)

# Workers that take a second object.

def clear2(a, c):
    try:
        c.clear()
    except BufferError:
        pass

def concat2(a, c):
    a + c

def inplace_concat2(a, c):
    a += c

def extend2(a, c):
    a.extend(c)

def setslice2(a, c):
    a[:] = c

def frombytes2(a, c):
    a.frombytes(c)

def new2(a, c):
    array.array('i', c)

def fromlist2(a, c):
    a.fromlist(c)

def iter_next(a, it):
    list(it)

def iter_reduce(a, it):
    c = it.__reduce__()
    assert not c[1] or 0xdd not in c[1][0]


class TestArray(unittest.TestCase):

    def check(self, funcs, a=None, *args):
        if a is None:
            a = array.array('i', [1])
        run_concurrently(funcs, args=(a, *args))

    def test_mutate(self):
        self.check([pop1] * 10)
        self.check([append1] * 10)
        self.check([insert1] * 10)
        self.check([remove1] * 10)
        self.check([inplace_repeat] * 10)
        self.check([byteswap] * 10)
        self.check([reverse] * 10, array.array('i', range(1000)))
        self.check([frombytes] * 10)
        self.check([fromlist] * 10)
        self.check([fromunicode] * 10, array.array('w', 'a'))
        self.check([insert1] + [clear] * 10)
        self.check([clear] + [extend] * 10)
        self.check([clear] + [inplace_concat] * 10)
        self.check([clear, setitem0] * 10, array.array('i', [1] * 10000))
        self.check([clear] + [inplace_repeat] * 10,
                   array.array('B', b'0' * 0x400))

    def test_read_and_mutate(self):
        self.check([pop1] + [getitem0] * 10)
        self.check([pop1] + [index1] * 10)
        self.check([pop1] + [contains1] * 10)
        self.check([pop1] + [count1] * 10)
        self.check([pop1] + [repr1] * 10)
        self.check([pop1] + [reduce_ex2] * 10)
        self.check([pop1] + [tobytes] * 10)
        self.check([pop1] + [tolist] * 10)
        self.check([insert1] + [repeat2] * 10)
        self.check([clear] + [copy] * 10, array.array('B', b'0' * 0x10000))
        self.check([clear] + [reduce_ex3] * 10,
                   array.array('B', b'0' * 0x10000))
        self.check([clear] + [repr1] * 10, array.array('B', b'0' * 0x1000))
        self.check([clear] + [getslice] * 10, array.array('i', [1] * 10000))
        self.check([clear] + [richcmplhs] * 10, array.array('i', [1] * 10000))
        self.check([clear] + [richcmprhs] * 10, array.array('i', [1] * 10000))
        self.check([clear, tounicode] * 10, array.array('w', 'a' * 10000))
        self.check([clear, tofile] * 10, array.array('w', 'a' * 10000))
        self.check([clear] + [concat] * 10, array.array('w', 'a' * 10000))
        self.check([clear] + [new] * 10, array.array('w', 'a' * 10000))

    def test_two_objects(self):
        def unicode_array():
            return array.array('w', 'a' * 10000)

        self.check([clear2] + [concat2] * 10,
                   unicode_array(), unicode_array())
        self.check([clear2] + [inplace_concat2] * 10,
                   unicode_array(), unicode_array())
        self.check([clear2] + [extend2] * 10,
                   unicode_array(), unicode_array())
        self.check([clear2] + [setslice2] * 10,
                   unicode_array(), unicode_array())
        self.check([clear2] + [frombytes2] * 10,
                   unicode_array(), array.array('B', b'a' * 10000))
        self.check([clear2] + [new2] * 10, None, [2] * 100)
        self.check([clear2] + [fromlist2] * 10, None, [2] * 100)

    def test_iterator(self):
        a = array.array('i', [1] * 10)
        self.check([clear] + [iter_next] * 10, a, iter(a))
        a = array.array('B', b'0' * 0x400)
        self.check([clear] + [iter_reduce] * 10, a, iter(a))

    def test_item_access_while_resizing(self):
        # Items are read and written without the lock.  A reader must only
        # see values that were stored, also if the buffer is replaced.
        n = 100
        for typecode, value in [('b', 1), ('H', 1), ('i', 1), ('q', 1),
                                ('d', 1.0), ('Zd', 1+2j), ('w', 'x')]:
            with self.subTest(typecode=typecode):
                a = array.array(typecode, [value] * n)
                tail = a[1:]
                done = threading.Event()

                def resize():
                    try:
                        for _ in range(200):
                            del a[1:]
                            a.extend(tail)
                    finally:
                        done.set()

                def read():
                    while not done.is_set():
                        try:
                            item = a[n - 1]
                        except IndexError:
                            pass
                        else:
                            self.assertEqual(item, value)
                        for item in a:
                            self.assertEqual(item, value)

                def write():
                    while not done.is_set():
                        try:
                            a[n - 1] = value
                        except IndexError:
                            pass

                run_concurrently([resize] + [read] * 4 + [write] * 2)
                self.assertEqual(a, array.array(typecode, [value] * n))


if __name__ == "__main__":
    unittest.main()
