import unittest, os
from collections import UserDict
from test.support import import_helper
from test.support.os_helper import unlink, TESTFN, TESTFN_UNDECODABLE

NULL = None
_testcapi = import_helper.import_module('_testcapi')
Py_single_input = _testcapi.Py_single_input
Py_file_input = _testcapi.Py_file_input
Py_eval_input = _testcapi.Py_eval_input


class PyRunTest(unittest.TestCase):

    # run_fileexflags args:
    # filename -- filename to run script from
    # start -- Py_single_input, Py_file_input, Py_eval_input
    # globals -- dict
    # locals -- mapping (dict)
    # closeit -- whether close file or not true/false
    # cf_flags -- compile flags bitmap
    # cf_feature_version -- compile flags feature version
    def test_pyrun_fileexflags(self):
        def run(*args):
            return _testcapi.run_fileexflags(TESTFN, Py_file_input, *args)

        with open(TESTFN, 'w') as fp:
            fp.write("a\n")
        self.addCleanup(unlink, TESTFN)

        self.assertIsNone(run(dict(a=1)))
        self.assertIsNone(run(dict(a=1), {}))
        self.assertIsNone(run({}, dict(a=1)))
        self.assertIsNone(run({}, UserDict(a=1)))
        self.assertIsNone(run(dict(a=1), {}, 1))  # closeit = True

        self.assertRaises(NameError, run, {})
        self.assertRaises(NameError, run, {}, {})
        self.assertRaises(TypeError, run, dict(a=1), [])
        self.assertRaises(TypeError, run, dict(a=1), 1)

        self.assertRaises(SystemError, run, NULL)
        self.assertRaises(SystemError, run, NULL, {})
        self.assertRaises(SystemError, run, NULL, dict(a=1))
        self.assertRaises(SystemError, run, UserDict())
        self.assertRaises(SystemError, run, UserDict(), {})
        self.assertRaises(SystemError, run, UserDict(), dict(a=1))

    @unittest.skipUnless(TESTFN_UNDECODABLE, "only works if there are undecodable paths")
    def test_pyrun_fileexflags_with_undecodable_filename(self):
        run = _testcapi.run_fileexflags
        try:
            with open(TESTFN_UNDECODABLE, 'w') as fp:
                fp.write("b\n")
            self.addCleanup(unlink, TESTFN_UNDECODABLE)
        except OSError:
            self.skipTest("undecodable paths are not supported")
        self.assertIsNone(run(TESTFN_UNDECODABLE, Py_file_input, dict(b=1)))


if __name__ == "__main__":
    unittest.main()
