import os
import sys
import textwrap
import unittest
from subprocess import Popen, PIPE
from test import support
from test.support.script_helper import assert_python_ok


class TestTool(unittest.TestCase):
    data = """

        [["blorpie"],[ "whoops" ] , [
                                 ],\t"d-shtaeou",\r"d-nthiouh",
        "i-vhbjkhnth", {"nifty":87}, {"morefield" :\tfalse,"field"
            :"yes"}  ]
           """

    expect_without_sort_keys = textwrap.dedent("""\
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
            "morefield": false,
            "field": "yes"
        }
    ]
    """)

    jsonlines_raw = textwrap.dedent("""\
    {"ingredients":["frog", "water", "chocolate", "glucose"]}
    {"ingredients":["chocolate","steel bolts"]}
    """)

    jsonlines_expect = textwrap.dedent("""\
    {
        "ingredients": [
            "frog",
            "water",
            "chocolate",
            "glucose"
        ]
    }
    {
        "ingredients": [
            "chocolate",
            "steel bolts"
        ]
    }
    """)

    def test_stdin_stdout(self):
        args = sys.executable, '-m', 'json.tool'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            out, err = proc.communicate(self.data.encode())
        self.assertEqual(out.splitlines(), self.expect.encode().splitlines())
        self.assertEqual(err, b'')

    def _create_infile(self, data=None):
        infile = support.TESTFN
        with open(infile, "w", encoding="utf-8") as fp:
            self.addCleanup(os.remove, infile)
            fp.write(data or self.data)
        return infile

    def test_infile_stdout(self):
        infile = self._create_infile()
        rc, out, err = assert_python_ok('-m', 'json.tool', infile)
        self.assertEqual(rc, 0)
        self.assertEqual(out.splitlines(), self.expect.encode().splitlines())
        self.assertEqual(err, b'')

    def test_non_ascii_infile(self):
        data = '{"msg": "\u3053\u3093\u306b\u3061\u306f"}'
        expect = textwrap.dedent('''\
        {
            "msg": "\\u3053\\u3093\\u306b\\u3061\\u306f"
        }
        ''').encode()

        infile = self._create_infile(data)
        rc, out, err = assert_python_ok('-m', 'json.tool', infile)

        self.assertEqual(rc, 0)
        self.assertEqual(out.splitlines(), expect.splitlines())
        self.assertEqual(err, b'')

    def test_infile_outfile(self):
        infile = self._create_infile()
        outfile = support.TESTFN + '.out'
        rc, out, err = assert_python_ok('-m', 'json.tool', infile, outfile)
        self.addCleanup(os.remove, outfile)
        with open(outfile, "r") as fp:
            self.assertEqual(fp.read(), self.expect)
        self.assertEqual(rc, 0)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    def test_jsonlines(self):
        args = sys.executable, '-m', 'json.tool', '--json-lines'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            out, err = proc.communicate(self.jsonlines_raw.encode())
        self.assertEqual(out.splitlines(), self.jsonlines_expect.encode().splitlines())
        self.assertEqual(err, b'')

    def test_help_flag(self):
        rc, out, err = assert_python_ok('-m', 'json.tool', '-h')
        self.assertEqual(rc, 0)
        self.assertTrue(out.startswith(b'usage: '))
        self.assertEqual(err, b'')

    def test_sort_keys_flag(self):
        infile = self._create_infile()
        rc, out, err = assert_python_ok('-m', 'json.tool', '--sort-keys', infile)
        self.assertEqual(rc, 0)
        self.assertEqual(out.splitlines(),
                         self.expect_without_sort_keys.encode().splitlines())
        self.assertEqual(err, b'')

    def test_indent(self):
        json_stdin = b'[1, 2]'
        expect = textwrap.dedent('''\
        [
          1,
          2
        ]
        ''').encode()
        args = sys.executable, '-m', 'json.tool', '--indent', '2'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            json_stdout, err = proc.communicate(json_stdin)
        self.assertEqual(expect.splitlines(), json_stdout.splitlines())
        self.assertEqual(err, b'')

    def test_no_indent(self):
        json_stdin = b'[1,\n2]'
        expect = b'[1, 2]'
        args = sys.executable, '-m', 'json.tool', '--no-indent'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            json_stdout, err = proc.communicate(json_stdin)
        self.assertEqual(expect.splitlines(), json_stdout.splitlines())
        self.assertEqual(err, b'')

    def test_tab(self):
        json_stdin = b'[1, 2]'
        expect = b'[\n\t1,\n\t2\n]\n'
        args = sys.executable, '-m', 'json.tool', '--tab'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            json_stdout, err = proc.communicate(json_stdin)
        self.assertEqual(expect.splitlines(), json_stdout.splitlines())
        self.assertEqual(err, b'')

    def test_compact(self):
        json_stdin = b'[ 1 ,\n 2]'
        expect = b'[1,2]'
        args = sys.executable, '-m', 'json.tool', '--compact'
        with Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE) as proc:
            json_stdout, err = proc.communicate(json_stdin)
        self.assertEqual(expect.splitlines(), json_stdout.splitlines())
        self.assertEqual(err, b'')

    def test_no_ensure_ascii_flag(self):
        infile = self._create_infile('{"key":"💩"}')
        outfile = support.TESTFN + '.out'
        self.addCleanup(os.remove, outfile)
        assert_python_ok('-m', 'json.tool', '--no-ensure-ascii', infile, outfile)
        with open(outfile, "rb") as f:
            lines = f.read().splitlines()
        # asserting utf-8 encoded output file
        expected = [b'{', b'    "key": "\xf0\x9f\x92\xa9"', b"}"]
        self.assertEqual(lines, expected)

    def test_ensure_ascii_default(self):
        infile = self._create_infile('{"key":"💩"}')
        outfile = support.TESTFN + '.out'
        self.addCleanup(os.remove, outfile)
        assert_python_ok('-m', 'json.tool', infile, outfile)
        with open(outfile, "rb") as f:
            lines = f.read().splitlines()
        # asserting an ascii encoded output file
        expected = [b'{', rb'    "key": "\ud83d\udca9"', b"}"]
        self.assertEqual(lines, expected)
