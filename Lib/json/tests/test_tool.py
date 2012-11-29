import os
import sys
import textwrap
import unittest
import subprocess
from test import test_support
from test.script_helper import assert_python_ok

class TestTool(unittest.TestCase):
    data = """

        [["blorpie"],[ "whoops" ] , [
                                 ],\t"d-shtaeou",\r"d-nthiouh",
        "i-vhbjkhnth", {"nifty":87}, {"morefield" :\tfalse,"field"
            :"yes"}  ]
           """

    expect = textwrap.dedent("""\
    [
        [
            "blorpie"
        ],
        [
            "whoops"
        ],
        [],
        "d-shtaeou",
        "d-nthiouh",
        "i-vhbjkhnth",
        {
            "nifty": 87
        },
        {
            "field": "yes",
            "morefield": false
        }
    ]
    """)

    def test_stdin_stdout(self):
        proc = subprocess.Popen(
                (sys.executable, '-m', 'json.tool'),
                stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        out, err = proc.communicate(self.data.encode())
        self.assertEqual(out.splitlines(), self.expect.encode().splitlines())
        self.assertEqual(err, None)

    def _create_infile(self):
        infile = test_support.TESTFN
        with open(infile, "w") as fp:
            self.addCleanup(os.remove, infile)
            fp.write(self.data)
        return infile

    def test_infile_stdout(self):
        infile = self._create_infile()
        rc, out, err = assert_python_ok('-m', 'json.tool', infile)
        self.assertEqual(out.splitlines(), self.expect.encode().splitlines())
        self.assertEqual(err, b'')

    def test_infile_outfile(self):
        infile = self._create_infile()
        outfile = test_support.TESTFN + '.out'
        rc, out, err = assert_python_ok('-m', 'json.tool', infile, outfile)
        self.addCleanup(os.remove, outfile)
        with open(outfile, "r") as fp:
            self.assertEqual(fp.read(), self.expect)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')
