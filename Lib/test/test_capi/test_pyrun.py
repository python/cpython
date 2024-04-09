import unittest, os
import _testcapi

NULL = None
Py_single_input = _testcapi.Py_single_input
Py_file_input = _testcapi.Py_file_input
Py_eval_input = _testcapi.Py_eval_input


class PyRunTest(unittest.TestCase):

    def setUp(self):
        self.filename = "pyrun_fileexflags_sample.py"
        with open(self.filename, 'w') as fp:
            fp.write("import sysconfig\n\nconfig = sysconfig.get_config_vars()\n")

    def tearDown(self):
        os.remove(self.filename)

    # test function args:
    # filename -- filename to run script from
    # start -- Py_single_input, Py_file_input, Py_eval_input
    # globals -- dict
    # locals -- mapping (dict)
    # closeit -- whether close file or not true/false
    # cf_flags -- compile flags bitmap
    # cf_feature_version -- compile flags feature version
    def test_pyrun_fileexflags_with_differents_args(self):

        def check(*args):
            res = _testcapi.run_fileexflags(self.filename, Py_file_input, *args)
            self.assertIsNone(res)

        check({}, NULL, 1, 0, 0)
        check({}, {}, 1, 0, 0)
        check({}, {}, 0, 0, 0)
        check(globals(), NULL, 1, 0, 0)
        check(globals(), locals(), 1, 0, 0)
        check({}, {}, 1, 0, 0)

    def test_pyrun_fileexflags_globals_is_null(self):
        def check(*args):
            with self.assertRaises(SystemError):
                _testcapi.run_fileexflags(self.filename, Py_file_input, *args)

        check(NULL, NULL, 1, 0, 0)
        check(NULL, {}, 1, 0, 0)
        check(NULL, locals(), 1, 0, 0)

if __name__ == "__main__":
    unittest.main()
