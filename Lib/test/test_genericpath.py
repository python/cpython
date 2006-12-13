import unittest
from test import test_support
import os
import genericpath

class AllCommonTest(unittest.TestCase):

    def assertIs(self, a, b):
        self.assert_(a is b)

    def test_commonprefix(self):
        self.assertEqual(
            genericpath.commonprefix([]),
            ""
        )
        self.assertEqual(
            genericpath.commonprefix(["/home/swenson/spam", "/home/swen/spam"]),
            "/home/swen"
        )
        self.assertEqual(
            genericpath.commonprefix(["/home/swen/spam", "/home/swen/eggs"]),
            "/home/swen/"
        )
        self.assertEqual(
            genericpath.commonprefix(["/home/swen/spam", "/home/swen/spam"]),
            "/home/swen/spam"
        )

    def test_getsize(self):
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertEqual(genericpath.getsize(test_support.TESTFN), 3)
        finally:
            if not f.closed:
                f.close()
            os.remove(test_support.TESTFN)

    def test_time(self):
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            f = open(test_support.TESTFN, "ab")
            f.write("bar")
            f.close()
            f = open(test_support.TESTFN, "rb")
            d = f.read()
            f.close()
            self.assertEqual(d, "foobar")

            self.assert_(
                genericpath.getctime(test_support.TESTFN) <=
                genericpath.getmtime(test_support.TESTFN)
            )
        finally:
            if not f.closed:
                f.close()
            os.remove(test_support.TESTFN)

    def test_exists(self):
        self.assertIs(genericpath.exists(test_support.TESTFN), False)
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertIs(genericpath.exists(test_support.TESTFN), True)
        finally:
            if not f.close():
                f.close()
            try:
                os.remove(test_support.TESTFN)
            except os.error:
                pass

        self.assertRaises(TypeError, genericpath.exists)

    def test_isdir(self):
        self.assertIs(genericpath.isdir(test_support.TESTFN), False)
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertIs(genericpath.isdir(test_support.TESTFN), False)
            os.remove(test_support.TESTFN)
            os.mkdir(test_support.TESTFN)
            self.assertIs(genericpath.isdir(test_support.TESTFN), True)
            os.rmdir(test_support.TESTFN)
        finally:
            if not f.close():
                f.close()
            try:
                os.remove(test_support.TESTFN)
            except os.error:
                pass
            try:
                os.rmdir(test_support.TESTFN)
            except os.error:
                pass

        self.assertRaises(TypeError, genericpath.isdir)

    def test_isfile(self):
        self.assertIs(genericpath.isfile(test_support.TESTFN), False)
        f = open(test_support.TESTFN, "wb")
        try:
            f.write("foo")
            f.close()
            self.assertIs(genericpath.isfile(test_support.TESTFN), True)
            os.remove(test_support.TESTFN)
            os.mkdir(test_support.TESTFN)
            self.assertIs(genericpath.isfile(test_support.TESTFN), False)
            os.rmdir(test_support.TESTFN)
        finally:
            if not f.close():
                f.close()
            try:
                os.remove(test_support.TESTFN)
            except os.error:
                pass
            try:
                os.rmdir(test_support.TESTFN)
            except os.error:
                pass

        self.assertRaises(TypeError, genericpath.isdir)

        def test_samefile(self):
            f = open(test_support.TESTFN + "1", "wb")
            try:
                f.write("foo")
                f.close()
                self.assertIs(
                    genericpath.samefile(
                        test_support.TESTFN + "1",
                        test_support.TESTFN + "1"
                    ),
                    True
                )
                # If we don't have links, assume that os.stat doesn't return resonable
                # inode information and thus, that samefile() doesn't work
                if hasattr(os, "symlink"):
                    os.symlink(
                        test_support.TESTFN + "1",
                        test_support.TESTFN + "2"
                    )
                    self.assertIs(
                        genericpath.samefile(
                            test_support.TESTFN + "1",
                            test_support.TESTFN + "2"
                        ),
                        True
                    )
                    os.remove(test_support.TESTFN + "2")
                    f = open(test_support.TESTFN + "2", "wb")
                    f.write("bar")
                    f.close()
                    self.assertIs(
                        genericpath.samefile(
                            test_support.TESTFN + "1",
                            test_support.TESTFN + "2"
                        ),
                        False
                    )
            finally:
                if not f.close():
                    f.close()
                try:
                    os.remove(test_support.TESTFN + "1")
                except os.error:
                    pass
                try:
                    os.remove(test_support.TESTFN + "2")
                except os.error:
                    pass

            self.assertRaises(TypeError, genericpath.samefile)

def test_main():
    test_support.run_unittest(AllCommonTest)

if __name__=="__main__":
    test_main()
