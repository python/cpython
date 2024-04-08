import unittest, os
from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

NULL = None
Py_single_input = _testcapi.Py_single_input
Py_file_input = _testcapi.Py_file_input
Py_eval_input = _testcapi.Py_eval_input

class PyRunTest(unittest.TestCase):

    # create file
    def __init__(self, *args, **kwargs):
        self.filename = "pyrun_fileexflags_sample.py"
        with open(self.filename, 'w') as fp:
            fp.write("import sysconfig\n\nconfig = sysconfig.get_config_vars()\n")
            fp.close()
            unittest.TestCase.__init__(self, *args, **kwargs)

    # delete file
    def __del__(self):
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

        self.assertIsNone(_testcapi.eval_pyrun_fileexflags(self.filename, Py_file_input, {}, NULL, 1, 0, 0))
        self.assertIsNone(_testcapi.eval_pyrun_fileexflags(self.filename, Py_file_input, {}, {}, 1, 0, 0))
        self.assertIsNone(_testcapi.eval_pyrun_fileexflags(self.filename, Py_file_input, {}, {}, 0, 0, 0))
        self.assertIsNone(_testcapi.eval_pyrun_fileexflags(self.filename, Py_file_input, globals(), NULL, 1, 0, 0))
        self.assertIsNone(_testcapi.eval_pyrun_fileexflags(self.filename, Py_file_input, globals(), locals(), 1, 0, 0))
        self.assertIsNone(_testcapi.eval_pyrun_fileexflags(self.filename, Py_file_input, {}, {}, 1, 0, 0))
        self.assertRaises(SystemError, _testcapi.eval_pyrun_fileexflags, self.filename, Py_file_input, NULL, NULL, 1, 0, 0)
        self.assertRaises(SystemError, _testcapi.eval_pyrun_fileexflags, self.filename, Py_file_input, NULL, {}, 1, 0, 0)
        self.assertRaises(SystemError, _testcapi.eval_pyrun_fileexflags, self.filename, Py_file_input, NULL, locals(), 1, 0, 0)

if __name__ == "__main__":
    unittest.main()
