import unittest
from test_support import run_unittest

# The test cases here cover several paths through the function calling
# code.  They depend on the METH_XXX flag that is used to define a C
# function, which can't be verified from Python.  If the METH_XXX decl
# for a C function changes, these tests may not cover the right paths.

class CFunctionCalls(unittest.TestCase):

    def test_varargs0(self):
        self.assertRaises(TypeError, {}.has_key)

    def test_varargs1(self):
        {}.has_key(0)

    def test_varargs2(self):
        self.assertRaises(TypeError, {}.has_key, 0, 1)

    def test_varargs0_ext(self):
        try:
            {}.has_key(*())
        except TypeError:
            pass

    def test_varargs1_ext(self):
        {}.has_key(*(0,))

    def test_varargs2_ext(self):
        try:
            {}.has_key(*(1, 2))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_varargs0_kw(self):
        self.assertRaises(TypeError, {}.has_key, x=2)

    def test_varargs1_kw(self):
        self.assertRaises(TypeError, {}.has_key, x=2)

    def test_varargs2_kw(self):
        self.assertRaises(TypeError, {}.has_key, x=2, y=2)

    def test_oldargs0_0(self):
        {}.keys()

    def test_oldargs0_1(self):
        self.assertRaises(TypeError, {}.keys, 0)

    def test_oldargs0_2(self):
        self.assertRaises(TypeError, {}.keys, 0, 1)

    def test_oldargs0_0_ext(self):
        {}.keys(*())

    def test_oldargs0_1_ext(self):
        try:
            {}.keys(*(0,))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs0_2_ext(self):
        try:
            {}.keys(*(1, 2))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs0_0_kw(self):
        try:
            {}.keys(x=2)
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs0_1_kw(self):
        self.assertRaises(TypeError, {}.keys, x=2)

    def test_oldargs0_2_kw(self):
        self.assertRaises(TypeError, {}.keys, x=2, y=2)

    def test_oldargs1_0(self):
        self.assertRaises(TypeError, {}.update)

    def test_oldargs1_1(self):
        {}.update({})

    def test_oldargs1_2(self):
        self.assertRaises(TypeError, {}.update, {}, 1)

    def test_oldargs1_0_ext(self):
        try:
            {}.update(*())
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs1_1_ext(self):
        {}.update(*({},))

    def test_oldargs1_2_ext(self):
        try:
            {}.update(*({}, 2))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs1_0_kw(self):
        self.assertRaises(TypeError, {}.update, x=2)

    def test_oldargs1_1_kw(self):
        self.assertRaises(TypeError, {}.update, {}, x=2)

    def test_oldargs1_2_kw(self):
        self.assertRaises(TypeError, {}.update, x=2, y=2)


def test_main():
    run_unittest(CFunctionCalls)


if __name__ == "__main__":
    test_main()
