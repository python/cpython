import copy_reg
import test_support
import unittest


class C:
    pass


class CopyRegTestCase(unittest.TestCase):

    def test_class(self):
        self.assertRaises(TypeError, copy_reg.pickle,
                          C, None, None)

    def test_noncallable_reduce(self):
        self.assertRaises(TypeError, copy_reg.pickle,
                          type(1), "not a callable")

    def test_noncallable_constructor(self):
        self.assertRaises(TypeError, copy_reg.pickle,
                          type(1), int, "not a callable")


def test_main():
    test_support.run_unittest(CopyRegTestCase)


if __name__ == "__main__":
    test_main()
