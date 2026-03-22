# Check every path through every method of UserList

from collections import UserList
from test import list_tests
from test import support
import unittest


class UserListSubclass(UserList):
    pass

class UserListSubclass2(UserList):
    pass


class UserListTest(list_tests.CommonTest):
    type2test = UserList

    def test_data(self):
        u = UserList()
        self.assertEqual(u.data, [])
        self.assertIs(type(u.data), list)
        a = [1, 2]
        u = UserList(a)
        self.assertEqual(u.data, a)
        self.assertIsNot(u.data, a)
        self.assertIs(type(u.data), list)
        u = UserList(u)
        self.assertEqual(u.data, a)
        self.assertIs(type(u.data), list)
        u = UserList("spam")
        self.assertEqual(u.data, list("spam"))
        self.assertIs(type(u.data), list)

    def test_getslice(self):
        super().test_getslice()
        l = [0, 1, 2, 3, 4]
        u = self.type2test(l)
        for i in range(-3, 6):
            self.assertEqual(u[:i], l[:i])
            self.assertEqual(u[i:], l[i:])
            for j in range(-3, 6):
                self.assertEqual(u[i:j], l[i:j])

    def test_slice_type(self):
        l = [0, 1, 2, 3, 4]
        u = UserList(l)
        self.assertIsInstance(u[:], u.__class__)
        self.assertEqual(u[:],u)

    def test_mixed_add(self):
        for t in UserList, list, str, tuple, iter:
            with self.subTest(t.__name__):
                u = UserList("spam") + t("eggs")
                self.assertEqual(u, list("spameggs"))
                self.assertIs(type(u), UserList)

                u = t("spam") + UserList("eggs")
                self.assertEqual(u, list("spameggs"))
                self.assertIs(type(u), UserList)

        u = UserList("spam") + UserListSubclass("eggs")
        self.assertEqual(u, list("spameggs"))
        self.assertIs(type(u), UserList)

        u = UserListSubclass("spam") + UserList("eggs")
        self.assertEqual(u, list("spameggs"))
        self.assertIs(type(u), UserListSubclass)

        u = UserListSubclass("spam") + UserListSubclass2("eggs")
        self.assertEqual(u, list("spameggs"))
        self.assertIs(type(u), UserListSubclass)

        u2 = UserList("eggs").__radd__(UserList("spam"))
        self.assertEqual(u2, list("spameggs"))
        self.assertIs(type(u), UserListSubclass)

        u2 = UserListSubclass("eggs").__radd__(UserListSubclass2("spam"))
        self.assertEqual(u2, list("spameggs"))
        self.assertIs(type(u), UserListSubclass)

    def test_mixed_iadd(self):
        for t in UserList, list, str, tuple, iter:
            with self.subTest(t.__name__):
                u = u2 = UserList("spam")
                u += t("eggs")
                self.assertEqual(u, list("spameggs"))
                self.assertIs(type(u), UserList)
                self.assertIs(u, u2)

                u = t("spam")
                u += UserList("eggs")
                self.assertEqual(u, list("spameggs"))
                self.assertIs(type(u), UserList)

        u = u2 = UserList("spam")
        u += UserListSubclass("eggs")
        self.assertEqual(u, list("spameggs"))
        self.assertIs(type(u), UserList)
        self.assertIs(u, u2)

        u = u2 = UserListSubclass("spam")
        u += UserList("eggs")
        self.assertEqual(u, list("spameggs"))
        self.assertIs(type(u), UserListSubclass)
        self.assertIs(u, u2)

        u = u2 = UserListSubclass("spam")
        u += UserListSubclass2("eggs")
        self.assertEqual(u, list("spameggs"))
        self.assertIs(type(u), UserListSubclass)
        self.assertIs(u, u2)

    def test_mixed_cmp(self):
        u = self.type2test([0, 1])
        self._assert_cmp(u, [0, 1], 0)
        self._assert_cmp(u, [0], 1)
        self._assert_cmp(u, [0, 2], -1)

    def test_getitemoverwriteiter(self):
        # Verify that __getitem__ overrides *are* recognized by __iter__
        class T(self.type2test):
            def __getitem__(self, key):
                return str(key) + '!!!'
        self.assertEqual(next(iter(T((1,2)))), "0!!!")

    def test_implementation(self):
        u = UserList([1])
        with (support.swap_attr(UserList, '__len__', None),
              support.swap_attr(UserList, 'insert', None)):
            u.append(2)
        self.assertEqual(u, [1, 2])
        with support.swap_attr(UserList, 'append', None):
            u.extend([3, 4])
        self.assertEqual(u, [1, 2, 3, 4])
        with support.swap_attr(UserList, 'append', None):
            u.extend(UserList([3, 4]))
        self.assertEqual(u, [1, 2, 3, 4, 3, 4])
        with support.swap_attr(UserList, '__iter__', None):
            c = u.count(3)
        self.assertEqual(c, 2)
        with (support.swap_attr(UserList, '__iter__', None),
              support.swap_attr(UserList, '__getitem__', None)):
            i = u.index(4)
        self.assertEqual(i, 3)
        with (support.swap_attr(UserList, 'index', None),
              support.swap_attr(UserList, '__getitem__', None)):
            u.remove(3)
        self.assertEqual(u, [1, 2, 4, 3, 4])
        with (support.swap_attr(UserList, '__getitem__', None),
              support.swap_attr(UserList, '__delitem__', None)):
            u.pop()
        self.assertEqual(u, [1, 2, 4, 3])
        with (support.swap_attr(UserList, '__len__', None),
              support.swap_attr(UserList, '__getitem__', None),
              support.swap_attr(UserList, '__setitem__', None)):
            u.reverse()
        self.assertEqual(u, [3, 4, 2, 1])
        with (support.swap_attr(UserList, '__len__', None),
              support.swap_attr(UserList, 'pop', None)):
            u.clear()
        self.assertEqual(u, [])

    def test_userlist_copy(self):
        u = self.type2test([6, 8, 1, 9, 1])
        v = u.copy()
        self.assertEqual(u, v)
        self.assertEqual(type(u), type(v))

    # Decorate existing test with recursion limit, because
    # the test is for C structure, but `UserList` is a Python structure.
    test_repr_deep = support.infinite_recursion(25)(
        list_tests.CommonTest.test_repr_deep,
    )

if __name__ == "__main__":
    unittest.main()
