import errno
import os
import sys
import textwrap
import unittest
import subprocess

from test import support
from test.support import force_colorized, force_not_colorized, os_helper
from test.support.script_helper import assert_python_ok

from _colorize import get_theme


@support.requires_subprocess()
class TestMain(unittest.TestCase):
    data = """

        [["blorpie"],[ "whoops" ] , [
                                 ],\t"d-shtaeou",\r"d-nthiouh",
        "i-vhbjkhnth", {"nifty":87}, {"morefield" :\tfalse,"field"
            :"yes"}  ]
           """
    module = 'json'

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

    @force_not_colorized
    def test_stdin_stdout(self):
        args = sys.executable, '-m', self.module
        process = subprocess.run(args, input=self.data, capture_output=True, text=True, check=True)
        self.assertEqual(process.stdout, self.expect)
        self.assertEqual(process.stderr, '')

    def _create_infile(self, data=None):
        infile = os_helper.TESTFN
        with open(infile, "w", encoding="utf-8") as fp:
            self.addCleanup(os.remove, infile)
            fp.write(data or self.data)
        return infile

    def test_infile_stdout(self):
        infile = self._create_infile()
        rc, out, err = assert_python_ok('-m', self.module, infile,
                                        PYTHON_COLORS='0')
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
        rc, out, err = assert_python_ok('-m', self.module, infile,
                                        PYTHON_COLORS='0')

        self.assertEqual(rc, 0)
        self.assertEqual(out.splitlines(), expect.splitlines())
        self.assertEqual(err, b'')

    def test_infile_outfile(self):
        infile = self._create_infile()
        outfile = os_helper.TESTFN + '.out'
        rc, out, err = assert_python_ok('-m', self.module, infile, outfile,
                                        PYTHON_COLORS='0')
        self.addCleanup(os.remove, outfile)
        with open(outfile, "r", encoding="utf-8") as fp:
            self.assertEqual(fp.read(), self.expect)
        self.assertEqual(rc, 0)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    def test_writing_in_place(self):
        infile = self._create_infile()
        rc, out, err = assert_python_ok('-m', self.module, infile, infile,
                                        PYTHON_COLORS='0')
        with open(infile, "r", encoding="utf-8") as fp:
            self.assertEqual(fp.read(), self.expect)
        self.assertEqual(rc, 0)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

    @force_not_colorized
    def test_jsonlines(self):
        args = sys.executable, '-m', self.module, '--json-lines'
        process = subprocess.run(args, input=self.jsonlines_raw, capture_output=True, text=True, check=True)
        self.assertEqual(process.stdout, self.jsonlines_expect)
        self.assertEqual(process.stderr, '')

    def test_help_flag(self):
        rc, out, err = assert_python_ok('-m', self.module, '-h',
                                        PYTHON_COLORS='0')
        self.assertEqual(rc, 0)
        self.assertStartsWith(out, b'usage: ')
        self.assertEqual(err, b'')

    def test_sort_keys_flag(self):
        infile = self._create_infile()
        rc, out, err = assert_python_ok('-m', self.module, '--sort-keys', infile,
                                        PYTHON_COLORS='0')
        self.assertEqual(rc, 0)
        self.assertEqual(out.splitlines(),
                         self.expect_without_sort_keys.encode().splitlines())
        self.assertEqual(err, b'')

    @force_not_colorized
    def test_indent(self):
        input_ = '[1, 2]'
        expect = textwrap.dedent('''\
        [
          1,
          2
        ]
        ''')
        args = sys.executable, '-m', self.module, '--indent', '2'
        process = subprocess.run(args, input=input_, capture_output=True, text=True, check=True)
        self.assertEqual(process.stdout, expect)
        self.assertEqual(process.stderr, '')

    @force_not_colorized
    def test_no_indent(self):
        input_ = '[1,\n2]'
        expect = '[1, 2]\n'
        args = sys.executable, '-m', self.module, '--no-indent'
        process = subprocess.run(args, input=input_, capture_output=True, text=True, check=True)
        self.assertEqual(process.stdout, expect)
        self.assertEqual(process.stderr, '')

    @force_not_colorized
    def test_tab(self):
        input_ = '[1, 2]'
        expect = '[\n\t1,\n\t2\n]\n'
        args = sys.executable, '-m', self.module, '--tab'
        process = subprocess.run(args, input=input_, capture_output=True, text=True, check=True)
        self.assertEqual(process.stdout, expect)
        self.assertEqual(process.stderr, '')

    @force_not_colorized
    def test_compact(self):
        input_ = '[ 1 ,\n 2]'
        expect = '[1,2]\n'
        args = sys.executable, '-m', self.module, '--compact'
        process = subprocess.run(args, input=input_, capture_output=True, text=True, check=True)
        self.assertEqual(process.stdout, expect)
        self.assertEqual(process.stderr, '')

    def test_no_ensure_ascii_flag(self):
        infile = self._create_infile('{"key":"💩"}')
        outfile = os_helper.TESTFN + '.out'
        self.addCleanup(os.remove, outfile)
        assert_python_ok('-m', self.module, '--no-ensure-ascii', infile,
                         outfile, PYTHON_COLORS='0')
        with open(outfile, "rb") as f:
            lines = f.read().splitlines()
        # asserting utf-8 encoded output file
        expected = [b'{', b'    "key": "\xf0\x9f\x92\xa9"', b"}"]
        self.assertEqual(lines, expected)

    def test_ensure_ascii_default(self):
        infile = self._create_infile('{"key":"💩"}')
        outfile = os_helper.TESTFN + '.out'
        self.addCleanup(os.remove, outfile)
        assert_python_ok('-m', self.module, infile, outfile, PYTHON_COLORS='0')
        with open(outfile, "rb") as f:
            lines = f.read().splitlines()
        # asserting an ascii encoded output file
        expected = [b'{', rb'    "key": "\ud83d\udca9"', b"}"]
        self.assertEqual(lines, expected)

    @force_not_colorized
    @unittest.skipIf(sys.platform =="win32", "The test is failed with ValueError on Windows")
    def test_broken_pipe_error(self):
        cmd = [sys.executable, '-m', self.module]
        proc = subprocess.Popen(cmd,
                                stdout=subprocess.PIPE,
                                stdin=subprocess.PIPE)
        # bpo-39828: Closing before json attempts to write into stdout.
        proc.stdout.close()
        proc.communicate(b'"{}"')
        self.assertEqual(proc.returncode, errno.EPIPE)

    @force_colorized
    def test_colors(self):
        infile = os_helper.TESTFN
        self.addCleanup(os.remove, infile)

        t = get_theme().syntax
        ob = "{"
        cb = "}"

        cases = (
            ('{}', '{}'),
            ('[]', '[]'),
            ('null', f'{t.keyword}null{t.reset}'),
            ('true', f'{t.keyword}true{t.reset}'),
            ('false', f'{t.keyword}false{t.reset}'),
            ('NaN', f'{t.number}NaN{t.reset}'),
            ('Infinity', f'{t.number}Infinity{t.reset}'),
            ('-Infinity', f'{t.number}-Infinity{t.reset}'),
            ('"foo"', f'{t.string}"foo"{t.reset}'),
            (r'" \"foo\" "', f'{t.string}" \\"foo\\" "{t.reset}'),
            ('"α"', f'{t.string}"\\u03b1"{t.reset}'),
            ('123', f'{t.number}123{t.reset}'),
            ('-1.25e+23', f'{t.number}-1.25e+23{t.reset}'),
            (r'{"\\": ""}',
             f'''\
{ob}
    {t.definition}"\\\\"{t.reset}: {t.string}""{t.reset}
{cb}'''),
            (r'{"\\\\": ""}',
             f'''\
{ob}
    {t.definition}"\\\\\\\\"{t.reset}: {t.string}""{t.reset}
{cb}'''),
            ('''\
{
    "foo": "bar",
    "baz": 1234,
    "qux": [true, false, null],
    "xyz": [NaN, -Infinity, Infinity]
}''',
             f'''\
{ob}
    {t.definition}"foo"{t.reset}: {t.string}"bar"{t.reset},
    {t.definition}"baz"{t.reset}: {t.number}1234{t.reset},
    {t.definition}"qux"{t.reset}: [
        {t.keyword}true{t.reset},
        {t.keyword}false{t.reset},
        {t.keyword}null{t.reset}
    ],
    {t.definition}"xyz"{t.reset}: [
        {t.number}NaN{t.reset},
        {t.number}-Infinity{t.reset},
        {t.number}Infinity{t.reset}
    ]
{cb}'''),
        )

        for input_, expected in cases:
            with self.subTest(input=input_):
                with open(infile, "w", encoding="utf-8") as fp:
                    fp.write(input_)
                _, stdout_b, _ = assert_python_ok(
                    '-m', self.module, infile, FORCE_COLOR='1', __isolated='1'
                )
                stdout = stdout_b.decode()
                stdout = stdout.replace('\r\n', '\n')  # normalize line endings
                stdout = stdout.strip()
                self.assertEqual(stdout, expected)


@support.requires_subprocess()
class TestTool(TestMain):
    module = 'json.tool'


if __name__ == "__main__":
    unittest.main()
