# Check every path through every method of UserList

from UserList import UserList
import unittest, test.test_support

class UserListTest(unittest.TestCase):

    def test_constructors(self):
        l0 = []
        l1 = [0]
        l2 = [0, 1]

        u = UserList()
        u0 = UserList(l0)
        u1 = UserList(l1)
        u2 = UserList(l2)

        uu = UserList(u)
        uu0 = UserList(u0)
        uu1 = UserList(u1)
        uu2 = UserList(u2)

        v = UserList(tuple(u))
        class OtherList:
            def __init__(self, initlist):
                self.__data = initlist
            def __len__(self):
                return len(self.__data)
            def __getitem__(self, i):
                return self.__data[i]
        v0 = UserList(OtherList(u0))
        vv = UserList("this is also a sequence")

    def test_repr(self):
        l0 = []
        l2 = [0, 1, 2]
        u0 = UserList(l0)
        u2 = UserList(l2)

        self.assertEqual(str(u0), str(l0))
        self.assertEqual(repr(u0), repr(l0))
        self.assertEqual(`u2`, `l2`)

    def test_cmplen(self):
        l0 = []
        l1 = [0]
        l2 = [0, 1]

        # Test constructors

        u = UserList()
        u0 = UserList(l0)
        u1 = UserList(l1)
        u2 = UserList(l2)

        uu = UserList(u)
        uu0 = UserList(u0)
        uu1 = UserList(u1)
        uu2 = UserList(u2)

        def mycmp(x, y):
            r = cmp(x, y)
            if r < 0: return -1
            if r > 0: return 1
            return r

        all = [l0, l1, l2, u, u0, u1, u2, uu, uu0, uu1, uu2]
        for a in all:
            for b in all:
                self.assertEqual(mycmp(a, b), mycmp(len(a), len(b)))

        self.assert_(u0 <= u2)
        self.assert_(u2 >= u0)

    def test_getitem(self):
        u = UserList([0, 1, 2])
        for i in xrange(len(u)):
            self.assertEqual(u[i], i)

    def test_setitem(self):
        u = UserList([0, 1])
        u[0] = 0
        u[1] = 100
        self.assertEqual(u, [0, 100])
        self.assertRaises(IndexError, u.__setitem__, 2, 200)

    def test_delitem(self):
        u = UserList([0, 1])
        del u[1]
        del u[0]
        self.assertRaises(IndexError, u.__delitem__, 0)

    def test_getslice(self):
        l = [0, 1]
        u = UserList(l)
        for i in xrange(-3, 4):
            self.assertEqual(u[:i], l[:i])
            self.assertEqual(u[i:], l[i:])
            for j in xrange(-3, 4):
                self.assertEqual(u[i:j], l[i:j])

    def test_setslice(self):
        l = [0, 1]
        u = UserList(l)

        # Test __setslice__
        for i in range(-3, 4):
            u[:i] = l[:i]
            self.assertEqual(u, l)
            u2 = u[:]
            u2[:i] = u[:i]
            self.assertEqual(u2, u)
            u[i:] = l[i:]
            self.assertEqual(u, l)
            u2 = u[:]
            u2[i:] = u[i:]
            self.assertEqual(u2, u)
            for j in range(-3, 4):
                u[i:j] = l[i:j]
                self.assertEqual(u, l)
                u2 = u[:]
                u2[i:j] = u[i:j]
                self.assertEqual(u2, u)

        uu2 = u2[:]
        uu2[:0] = [-2, -1]
        self.assertEqual(uu2, [-2, -1, 0, 1])
        uu2[0:] = []
        self.assertEqual(uu2, [])

    def test_contains(self):
        u = UserList([0, 1, 2])
        for i in u:
            self.assert_(i in u)
        for i in min(u)-1, max(u)+1:
            self.assert_(i not in u)

    def test_delslice(self):
        u = UserList([0, 1])
        del u[1:2]
        del u[0:1]
        self.assertEqual(u, [])

        u = UserList([0, 1])
        del u[1:]
        del u[:1]
        self.assertEqual(u, [])

    def test_addmul(self):
        u1 = UserList([0])
        u2 = UserList([0, 1])
        self.assertEqual(u1, u1 + [])
        self.assertEqual(u1, [] + u1)
        self.assertEqual(u1 + [1], u2)
        self.assertEqual([-1] + u1, [-1, 0])
        self.assertEqual(u2, u2*1)
        self.assertEqual(u2, 1*u2)
        self.assertEqual(u2+u2, u2*2)
        self.assertEqual(u2+u2, 2*u2)
        self.assertEqual(u2+u2+u2, u2*3)
        self.assertEqual(u2+u2+u2, 3*u2)

    def test_add_specials(self):
        u = UserList("spam")
        u2 = u + "eggs"
        self.assertEqual(u2, list("spameggs"))

    def test_radd_specials(self):
        u = UserList("eggs")
        u2 = "spam" + u
        self.assertEqual(u2, list("spameggs"))
        u2 = u.__radd__(UserList("spam"))
        self.assertEqual(u2, list("spameggs"))

    def test_append(self):
        u = UserList((0, ))
        u.append(1)
        self.assertEqual(u, [0, 1])

    def test_insert(self):
        u = UserList((0, 1))
        u.insert(0, -1)
        self.assertEqual(u, [-1, 0, 1])

    def test_pop(self):
        u = UserList((-1, 0, 1))
        u.pop()
        self.assertEqual(u, [-1, 0])
        u.pop(0)
        self.assertEqual(u, [0])

    def test_remove(self):
        u = UserList((0, 1))
        u.remove(1)
        self.assertEqual(u, [0])

    def test_count(self):
        u = UserList((0, 1))*3
        self.assertEqual(u.count(0), 3)
        self.assertEqual(u.count(1), 3)
        self.assertEqual(u.count(2), 0)

    def test_index(self):
        u = UserList((0, 1))
        self.assertEqual(u.index(0), 0)
        self.assertEqual(u.index(1), 1)
        self.assertRaises(ValueError, u.index, 2)

        u = UserList([-2,-1,0,0,1,2])
        self.assertEqual(u.count(0), 2)
        self.assertEqual(u.index(0), 2)
        self.assertEqual(u.index(0,2), 2)
        self.assertEqual(u.index(-2,-10), 0)
        self.assertEqual(u.index(0,3), 3)
        self.assertEqual(u.index(0,3,4), 3)
        self.assertRaises(ValueError, u.index, 2,0,-10)

    def test_reverse(self):
        u = UserList((0, 1))
        u2 = u[:]
        u.reverse()
        self.assertEqual(u, [1, 0])
        u.reverse()
        self.assertEqual(u, u2)

    def test_sort(self):
        u = UserList([1, 0])
        u.sort()
        self.assertEqual(u, [0, 1])

    def test_slice(self):
        u = UserList("spam")
        u[:2] = "h"
        self.assertEqual(u, list("ham"))

    def test_iadd(self):
        u = UserList((0, 1))
        u += [0, 1]
        self.assertEqual(u, [0, 1, 0, 1])
        u += UserList([0, 1])
        self.assertEqual(u, [0, 1, 0, 1, 0, 1])

        u = UserList("spam")
        u += "eggs"
        self.assertEqual(u, list("spameggs"))

    def test_extend(self):
        u1 = UserList((0, ))
        u2 = UserList((0, 1))
        u = u1[:]
        u.extend(u2)
        self.assertEqual(u, u1 + u2)

        u = UserList("spam")
        u.extend("eggs")
        self.assertEqual(u, list("spameggs"))

    def test_imul(self):
        u = UserList((0, 1))
        u *= 3
        self.assertEqual(u, [0, 1, 0, 1, 0, 1])

def test_main():
    test.test_support.run_unittest(UserListTest)

if __name__ == "__main__":
    test_main()
