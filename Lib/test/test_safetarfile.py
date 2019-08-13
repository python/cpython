import sys
import os
import io
import unittest
import tarfile
from .test_tarfile import testtardir, setUpModule as setUpModuleTarFile, tearDownModule as tearDownModuleTarFile, GzipTest, Bz2Test, LzmaTest, UstarReadTestBase, ListTestBase

class SafeTarFileTestBase:
    tarfile_module = tarfile.SafeTarFile
    tarfile_open = tarfile.safe_open

class SafeTarFileTest(unittest.TestCase):
    ANALYZE_RESULTS = "analyzeresults"
    FILTER_RESULTS = "filterresults"
    IS_SAFE_RESULTS = "is_saferesults"

    TEST_RESULTS = {
        os.path.join(testtardir, "sly_absolute0.tar"):
        {
            ANALYZE_RESULTS:
            {
                "/tmp/moo": {tarfile.WARN_ABSOLUTE_NAME}
            },
            FILTER_RESULTS: [],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_absolute1.tar"):
        {
            ANALYZE_RESULTS:
            {
                "//tmp/moo": {tarfile.WARN_ABSOLUTE_NAME}
            },
            FILTER_RESULTS: [],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_dirsymlink0.tar"):
        {
            ANALYZE_RESULTS:
            {
                "tmp": {tarfile.WARN_ABSOLUTE_LINKNAME},
                "tmp/moo": {tarfile.WARN_ABSOLUTE_NAME}
            },
            FILTER_RESULTS: [],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_dirsymlink1.tar"):
        {
            ANALYZE_RESULTS:
            {
                "cur": set(),
                "par": {tarfile.WARN_RELATIVE_LINKNAME},
                "par/moo": {tarfile.WARN_RELATIVE_NAME}
            },
            FILTER_RESULTS: ["cur"],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_dirsymlink2.tar"):
        {
            ANALYZE_RESULTS:
            {
                "cur": set(),
                "cur/par": {tarfile.WARN_RELATIVE_LINKNAME},
                "par/moo": {tarfile.WARN_RELATIVE_NAME}
            },
            FILTER_RESULTS: ["cur"],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_dirsymlink3.tar"):
        {
            ANALYZE_RESULTS:
            {
                "dirsym": set(),
                "dirsym/sym": set(),
                "dirsym/symsym3": {tarfile.WARN_RELATIVE_LINKNAME}
            },
            FILTER_RESULTS: ["dirsym", "dirsym/sym"],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_relative0.tar"):
        {
            ANALYZE_RESULTS:
            {
                "../moo": {tarfile.WARN_RELATIVE_NAME}
            },
            FILTER_RESULTS: [],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_relative1.tar"):
        {
            ANALYZE_RESULTS:
            {
                "tmp/../../moo": {tarfile.WARN_RELATIVE_NAME}
            },
            FILTER_RESULTS: [],
            IS_SAFE_RESULTS: False
        },

        os.path.join(testtardir, "sly_symlink.tar"):
        {
            ANALYZE_RESULTS:
            {
                "moo": {tarfile.WARN_ABSOLUTE_LINKNAME},
                "moo": {tarfile.WARN_DUPLICATE_NAME}
            },
            FILTER_RESULTS: [],
            IS_SAFE_RESULTS: False
        }
    }

    def test_analyze(self):
        for entry in self.TEST_RESULTS:
            analyzeresults = self._get_analyze_results(entry)
            expectedvalues = self.TEST_RESULTS[entry][self.ANALYZE_RESULTS]
            self.assertEqual(analyzeresults, expectedvalues,
                    "SafeTarFile analyze() failed for " + entry)

    def test_filter(self):
        for entry in self.TEST_RESULTS:
            filterresults = self._get_filter_results(entry)
            expectedvalues = self.TEST_RESULTS[entry][self.FILTER_RESULTS]
            self.assertEqual(filterresults, expectedvalues,
                    "SafeTarFile filter() failed for " + entry)

    def test_is_safe(self):
        for entry in self.TEST_RESULTS:
            issaferesults = self._get_is_safe_results(entry)
            expectedvalues = self.TEST_RESULTS[entry][self.IS_SAFE_RESULTS]
            self.assertEqual(issaferesults, expectedvalues,
                    "SafeTarFile is_safe() failed for " + entry)

    def _get_analyze_results(self, tarballpath):
        with open(tarballpath, "r+b") as fileobj:
            results = {}
            tar = tarfile.safe_open(fileobj=fileobj)
            for result in tar.analyze():
                results[result[0].name] = result[1]

            return results

    def _get_filter_results(self, tarballpath):
        with open(tarballpath, "r+b") as fileobj:
            results = []
            tar = tarfile.safe_open(fileobj=fileobj)
            for result in tar.filter():
                results.append(result.name)

            return results

    def _get_is_safe_results(self, tarballpath):
        with open(tarballpath, "r+b") as fileobj:
            tar = tarfile.safe_open(fileobj=fileobj)
            return tar.is_safe()


class FileUstarReadTest(UstarReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class FileGzipUstarReadTest(GzipTest, UstarReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class FileBz2UstarReadTest(Bz2Test, UstarReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class FileLzmaUstarReadTest(LzmaTest, UstarReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass


class ListTest(ListTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

"""

class GzipListTest(GzipTest, ListTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class Bz2ListTest(Bz2Test, ListTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class LzmaListTest(LzmaTest, ListTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass



class MiscReadTest(MiscReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    test_fail_comp = None

class GzipMiscReadTest(GzipTest, MiscReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class Bz2MiscReadTest(Bz2Test, MiscReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    def requires_name_attribute(self):
        self.skipTest("BZ2File have no name attribute")

class LzmaMiscReadTest(LzmaTest, MiscReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    def requires_name_attribute(self):
        self.skipTest("LZMAFile have no name attribute")


class StreamReadTest(StreamReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class GzipStreamReadTest(GzipTest, StreamReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class Bz2StreamReadTest(Bz2Test, StreamReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class LzmaStreamReadTest(LzmaTest, StreamReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass


class DetectReadTest(DetectReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class GzipDetectReadTest(GzipTest, DetectReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class Bz2DetectReadTest(Bz2DetectReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class LzmaDetectReadTest(LzmaTest, DetectReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

class MemberReadTest(MemberReadTestBase, unittest.TestCase, SafeTarFileTestBase):
    pass

"""

def setUpModule():
    setUpModuleTarFile()

def tearDownModule():
    tearDownModuleTarFile()

if __name__ == "__main__":
    unittest.main()