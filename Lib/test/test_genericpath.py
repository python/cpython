import unittest
from test import support
import os
import genericpath

class AllCommonTest(unittest.TestCase):

    def assertIs(self, a, b):
        self.assertTrue(a is b)

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
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertEqual(genericpath.getsize(support.TESTFN), 3)
        finally:
            if not f.closed:
                f.close()
            os.remove(support.TESTFN)

    def test_time(self):
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            f = open(support.TESTFN, "ab")
            f.write(b"bar")
            f.close()
            f = open(support.TESTFN, "rb")
            d = f.read()
            f.close()
            self.assertEqual(d, b"foobar")

            self.assertLessEqual(
                genericpath.getctime(support.TESTFN),
                genericpath.getmtime(support.TESTFN)
            )
        finally:
            if not f.closed:
                f.close()
            os.remove(support.TESTFN)

    def test_exists(self):
        self.assertIs(genericpath.exists(support.TESTFN), False)
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(genericpath.exists(support.TESTFN), True)
        finally:
            if not f.close():
                f.close()
            try:
                os.remove(support.TESTFN)
            except os.error:
                pass

        self.assertRaises(TypeError, genericpath.exists)

    def test_isdir(self):
        self.assertIs(genericpath.isdir(support.TESTFN), False)
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(genericpath.isdir(support.TESTFN), False)
            os.remove(support.TESTFN)
            os.mkdir(support.TESTFN)
            self.assertIs(genericpath.isdir(support.TESTFN), True)
            os.rmdir(support.TESTFN)
        finally:
            if not f.close():
                f.close()
            try:
                os.remove(support.TESTFN)
            except os.error:
                pass
            try:
                os.rmdir(support.TESTFN)
            except os.error:
                pass

        self.assertRaises(TypeError, genericpath.isdir)

    def test_isfile(self):
        self.assertIs(genericpath.isfile(support.TESTFN), False)
        f = open(support.TESTFN, "wb")
        try:
            f.write(b"foo")
            f.close()
            self.assertIs(genericpath.isfile(support.TESTFN), True)
            os.remove(support.TESTFN)
            os.mkdir(support.TESTFN)
            self.assertIs(genericpath.isfile(support.TESTFN), False)
            os.rmdir(support.TESTFN)
        finally:
            if not f.close():
                f.close()
            try:
                os.remove(support.TESTFN)
            except os.error:
                pass
            try:
                os.rmdir(support.TESTFN)
            except os.error:
                pass

        self.assertRaises(TypeError, genericpath.isdir)

        def test_samefile(self):
            f = open(support.TESTFN + "1", "wb")
            try:
                f.write(b"foo")
                f.close()
                self.assertIs(
                    genericpath.samefile(
                        support.TESTFN + "1",
                        support.TESTFN + "1"
                    ),
                    True
                )
                # If we don't have links, assume that os.stat doesn't return resonable
                # inode information and thus, that samefile() doesn't work
                if hasattr(os, "symlink"):
                    os.symlink(
                        support.TESTFN + "1",
                        support.TESTFN + "2"
                    )
                    self.assertIs(
                        genericpath.samefile(
                            support.TESTFN + "1",
                            support.TESTFN + "2"
                        ),
                        True
                    )
                    os.remove(support.TESTFN + "2")
                    f = open(support.TESTFN + "2", "wb")
                    f.write(b"bar")
                    f.close()
                    self.assertIs(
                        genericpath.samefile(
                            support.TESTFN + "1",
                            support.TESTFN + "2"
                        ),
                        False
                    )
            finally:
                if not f.close():
                    f.close()
                try:
                    os.remove(support.TESTFN + "1")
                except os.error:
                    pass
                try:
                    os.remove(support.TESTFN + "2")
                except os.error:
                    pass

            self.assertRaises(TypeError, genericpath.samefile)

def test_main():
    support.run_unittest(AllCommonTest)

if __name__=="__main__":
    test_main()
