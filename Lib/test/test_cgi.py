from test.support import run_unittest, check_warnings
import cgi
import os
import sys
import tempfile
import unittest
import warnings
from collections import namedtuple
from io import StringIO, BytesIO

class HackedSysModule:
    # The regression test will have real values in sys.argv, which
    # will completely confuse the test of the cgi module
    argv = []
    stdin = sys.stdin

cgi.sys = HackedSysModule()

class ComparableException:
    def __init__(self, err):
        self.err = err

    def __str__(self):
        return str(self.err)

    def __eq__(self, anExc):
        if not isinstance(anExc, Exception):
            return NotImplemented
        return (self.err.__class__ == anExc.__class__ and
                self.err.args == anExc.args)

    def __getattr__(self, attr):
        return getattr(self.err, attr)

def do_test(buf, method):
    env = {}
    if method == "GET":
        fp = None
        env['REQUEST_METHOD'] = 'GET'
        env['QUERY_STRING'] = buf
    elif method == "POST":
        fp = BytesIO(buf.encode('latin-1')) # FieldStorage expects bytes
        env['REQUEST_METHOD'] = 'POST'
        env['CONTENT_TYPE'] = 'application/x-www-form-urlencoded'
        env['CONTENT_LENGTH'] = str(len(buf))
    else:
        raise ValueError("unknown method: %s" % method)
    try:
        return cgi.parse(fp, env, strict_parsing=1)
    except Exception as err:
        return ComparableException(err)

parse_strict_test_cases = [
    ("", ValueError("bad query field: ''")),
    ("&", ValueError("bad query field: ''")),
    ("&&", ValueError("bad query field: ''")),
    (";", ValueError("bad query field: ''")),
    (";&;", ValueError("bad query field: ''")),
    # Should the next few really be valid?
    ("=", {}),
    ("=&=", {}),
    ("=;=", {}),
    # This rest seem to make sense
    ("=a", {'': ['a']}),
    ("&=a", ValueError("bad query field: ''")),
    ("=a&", ValueError("bad query field: ''")),
    ("=&a", ValueError("bad query field: 'a'")),
    ("b=a", {'b': ['a']}),
    ("b+=a", {'b ': ['a']}),
    ("a=b=a", {'a': ['b=a']}),
    ("a=+b=a", {'a': [' b=a']}),
    ("&b=a", ValueError("bad query field: ''")),
    ("b&=a", ValueError("bad query field: 'b'")),
    ("a=a+b&b=b+c", {'a': ['a b'], 'b': ['b c']}),
    ("a=a+b&a=b+a", {'a': ['a b', 'b a']}),
    ("x=1&y=2.0&z=2-3.%2b0", {'x': ['1'], 'y': ['2.0'], 'z': ['2-3.+0']}),
    ("x=1;y=2.0&z=2-3.%2b0", {'x': ['1'], 'y': ['2.0'], 'z': ['2-3.+0']}),
    ("x=1;y=2.0;z=2-3.%2b0", {'x': ['1'], 'y': ['2.0'], 'z': ['2-3.+0']}),
    ("Hbc5161168c542333633315dee1182227:key_store_seqid=400006&cuyer=r&view=bustomer&order_id=0bb2e248638833d48cb7fed300000f1b&expire=964546263&lobale=en-US&kid=130003.300038&ss=env",
     {'Hbc5161168c542333633315dee1182227:key_store_seqid': ['400006'],
      'cuyer': ['r'],
      'expire': ['964546263'],
      'kid': ['130003.300038'],
      'lobale': ['en-US'],
      'order_id': ['0bb2e248638833d48cb7fed300000f1b'],
      'ss': ['env'],
      'view': ['bustomer'],
      }),

    ("group_id=5470&set=custom&_assigned_to=31392&_status=1&_category=100&SUBMIT=Browse",
     {'SUBMIT': ['Browse'],
      '_assigned_to': ['31392'],
      '_category': ['100'],
      '_status': ['1'],
      'group_id': ['5470'],
      'set': ['custom'],
      })
    ]

def norm(seq):
    return sorted(seq, key=repr)

def first_elts(list):
    return [p[0] for p in list]

def first_second_elts(list):
    return [(p[0], p[1][0]) for p in list]

def gen_result(data, environ):
    encoding = 'latin-1'
    fake_stdin = BytesIO(data.encode(encoding))
    fake_stdin.seek(0)
    form = cgi.FieldStorage(fp=fake_stdin, environ=environ, encoding=encoding)

    result = {}
    for k, v in dict(form).items():
        result[k] = isinstance(v, list) and form.getlist(k) or v.value

    return result

class CgiTests(unittest.TestCase):

    def test_parse_multipart(self):
        fp = BytesIO(POSTDATA.encode('latin1'))
        env = {'boundary': BOUNDARY.encode('latin1'),
               'CONTENT-LENGTH': '558'}
        result = cgi.parse_multipart(fp, env)
        expected = {'submit': [b' Add '], 'id': [b'1234'],
                    'file': [b'Testing 123.\n'], 'title': [b'']}
        self.assertEqual(result, expected)

    def test_fieldstorage_properties(self):
        fs = cgi.FieldStorage()
        self.assertFalse(fs)
        self.assertIn("FieldStorage", repr(fs))
        self.assertEqual(list(fs), list(fs.keys()))
        fs.list.append(namedtuple('MockFieldStorage', 'name')('fieldvalue'))
        self.assertTrue(fs)

    def test_fieldstorage_invalid(self):
        self.assertRaises(TypeError, cgi.FieldStorage, "not-a-file-obj",
                                                            environ={"REQUEST_METHOD":"PUT"})
        self.assertRaises(TypeError, cgi.FieldStorage, "foo", "bar")
        fs = cgi.FieldStorage(headers={'content-type':'text/plain'})
        self.assertRaises(TypeError, bool, fs)

    def test_escape(self):
        # cgi.escape() is deprecated.
        with warnings.catch_warnings():
            warnings.filterwarnings('ignore', 'cgi\.escape',
                                     DeprecationWarning)
            self.assertEqual("test &amp; string", cgi.escape("test & string"))
            self.assertEqual("&lt;test string&gt;", cgi.escape("<test string>"))
            self.assertEqual("&quot;test string&quot;", cgi.escape('"test string"', True))

    def test_strict(self):
        for orig, expect in parse_strict_test_cases:
            # Test basic parsing
            d = do_test(orig, "GET")
            self.assertEqual(d, expect, "Error parsing %s method GET" % repr(orig))
            d = do_test(orig, "POST")
            self.assertEqual(d, expect, "Error parsing %s method POST" % repr(orig))

            env = {'QUERY_STRING': orig}
            fs = cgi.FieldStorage(environ=env)
            if isinstance(expect, dict):
                # test dict interface
                self.assertEqual(len(expect), len(fs))
                self.assertCountEqual(expect.keys(), fs.keys())
                ##self.assertEqual(norm(expect.values()), norm(fs.values()))
                ##self.assertEqual(norm(expect.items()), norm(fs.items()))
                self.assertEqual(fs.getvalue("nonexistent field", "default"), "default")
                # test individual fields
                for key in expect.keys():
                    expect_val = expect[key]
                    self.assertIn(key, fs)
                    if len(expect_val) > 1:
                        self.assertEqual(fs.getvalue(key), expect_val)
                    else:
                        self.assertEqual(fs.getvalue(key), expect_val[0])

    def test_log(self):
        cgi.log("Testing")

        cgi.logfp = StringIO()
        cgi.initlog("%s", "Testing initlog 1")
        cgi.log("%s", "Testing log 2")
        self.assertEqual(cgi.logfp.getvalue(), "Testing initlog 1\nTesting log 2\n")
        if os.path.exists("/dev/null"):
            cgi.logfp = None
            cgi.logfile = "/dev/null"
            cgi.initlog("%s", "Testing log 3")
            self.addCleanup(cgi.closelog)
            cgi.log("Testing log 4")

    def test_fieldstorage_readline(self):
        # FieldStorage uses readline, which has the capacity to read all
        # contents of the input file into memory; we use readline's size argument
        # to prevent that for files that do not contain any newlines in
        # non-GET/HEAD requests
        class TestReadlineFile:
            def __init__(self, file):
                self.file = file
                self.numcalls = 0

            def readline(self, size=None):
                self.numcalls += 1
                if size:
                    return self.file.readline(size)
                else:
                    return self.file.readline()

            def __getattr__(self, name):
                file = self.__dict__['file']
                a = getattr(file, name)
                if not isinstance(a, int):
                    setattr(self, name, a)
                return a

        f = TestReadlineFile(tempfile.TemporaryFile("wb+"))
        self.addCleanup(f.close)
        f.write(b'x' * 256 * 1024)
        f.seek(0)
        env = {'REQUEST_METHOD':'PUT'}
        fs = cgi.FieldStorage(fp=f, environ=env)
        self.addCleanup(fs.file.close)
        # if we're not chunking properly, readline is only called twice
        # (by read_binary); if we are chunking properly, it will be called 5 times
        # as long as the chunksize is 1 << 16.
        self.assertGreater(f.numcalls, 2)
        f.close()

    def test_fieldstorage_multipart(self):
        #Test basic FieldStorage multipart parsing
        env = {
            'REQUEST_METHOD': 'POST',
            'CONTENT_TYPE': 'multipart/form-data; boundary={}'.format(BOUNDARY),
            'CONTENT_LENGTH': '558'}
        fp = BytesIO(POSTDATA.encode('latin-1'))
        fs = cgi.FieldStorage(fp, environ=env, encoding="latin-1")
        self.assertEqual(len(fs.list), 4)
        expect = [{'name':'id', 'filename':None, 'value':'1234'},
                  {'name':'title', 'filename':None, 'value':''},
                  {'name':'file', 'filename':'test.txt', 'value':b'Testing 123.\n'},
                  {'name':'submit', 'filename':None, 'value':' Add '}]
        for x in range(len(fs.list)):
            for k, exp in expect[x].items():
                got = getattr(fs.list[x], k)
                self.assertEqual(got, exp)

    def test_fieldstorage_multipart_non_ascii(self):
        #Test basic FieldStorage multipart parsing
        env = {'REQUEST_METHOD':'POST',
            'CONTENT_TYPE': 'multipart/form-data; boundary={}'.format(BOUNDARY),
            'CONTENT_LENGTH':'558'}
        for encoding in ['iso-8859-1','utf-8']:
            fp = BytesIO(POSTDATA_NON_ASCII.encode(encoding))
            fs = cgi.FieldStorage(fp, environ=env,encoding=encoding)
            self.assertEqual(len(fs.list), 1)
            expect = [{'name':'id', 'filename':None, 'value':'\xe7\xf1\x80'}]
            for x in range(len(fs.list)):
                for k, exp in expect[x].items():
                    got = getattr(fs.list[x], k)
                    self.assertEqual(got, exp)

    def test_fieldstorage_multipart_maxline(self):
        # Issue #18167
        maxline = 1 << 16
        self.maxDiff = None
        def check(content):
            data = """---123
Content-Disposition: form-data; name="upload"; filename="fake.txt"
Content-Type: text/plain

%s
---123--
""".replace('\n', '\r\n') % content
            environ = {
                'CONTENT_LENGTH':   str(len(data)),
                'CONTENT_TYPE':     'multipart/form-data; boundary=-123',
                'REQUEST_METHOD':   'POST',
            }
            self.assertEqual(gen_result(data, environ),
                             {'upload': content.encode('latin1')})
        check('x' * (maxline - 1))
        check('x' * (maxline - 1) + '\r')
        check('x' * (maxline - 1) + '\r' + 'y' * (maxline - 1))

    def test_fieldstorage_multipart_w3c(self):
        # Test basic FieldStorage multipart parsing (W3C sample)
        env = {
            'REQUEST_METHOD': 'POST',
            'CONTENT_TYPE': 'multipart/form-data; boundary={}'.format(BOUNDARY_W3),
            'CONTENT_LENGTH': str(len(POSTDATA_W3))}
        fp = BytesIO(POSTDATA_W3.encode('latin-1'))
        fs = cgi.FieldStorage(fp, environ=env, encoding="latin-1")
        self.assertEqual(len(fs.list), 2)
        self.assertEqual(fs.list[0].name, 'submit-name')
        self.assertEqual(fs.list[0].value, 'Larry')
        self.assertEqual(fs.list[1].name, 'files')
        files = fs.list[1].value
        self.assertEqual(len(files), 2)
        expect = [{'name': None, 'filename': 'file1.txt', 'value': b'... contents of file1.txt ...'},
                  {'name': None, 'filename': 'file2.gif', 'value': b'...contents of file2.gif...'}]
        for x in range(len(files)):
            for k, exp in expect[x].items():
                got = getattr(files[x], k)
                self.assertEqual(got, exp)

    _qs_result = {
        'key1': 'value1',
        'key2': ['value2x', 'value2y'],
        'key3': 'value3',
        'key4': 'value4'
    }
    def testQSAndUrlEncode(self):
        data = "key2=value2x&key3=value3&key4=value4"
        environ = {
            'CONTENT_LENGTH':   str(len(data)),
            'CONTENT_TYPE':     'application/x-www-form-urlencoded',
            'QUERY_STRING':     'key1=value1&key2=value2y',
            'REQUEST_METHOD':   'POST',
        }
        v = gen_result(data, environ)
        self.assertEqual(self._qs_result, v)

    def testQSAndFormData(self):
        data = """---123
Content-Disposition: form-data; name="key2"

value2y
---123
Content-Disposition: form-data; name="key3"

value3
---123
Content-Disposition: form-data; name="key4"

value4
---123--
"""
        environ = {
            'CONTENT_LENGTH':   str(len(data)),
            'CONTENT_TYPE':     'multipart/form-data; boundary=-123',
            'QUERY_STRING':     'key1=value1&key2=value2x',
            'REQUEST_METHOD':   'POST',
        }
        v = gen_result(data, environ)
        self.assertEqual(self._qs_result, v)

    def testQSAndFormDataFile(self):
        data = """---123
Content-Disposition: form-data; name="key2"

value2y
---123
Content-Disposition: form-data; name="key3"

value3
---123
Content-Disposition: form-data; name="key4"

value4
---123
Content-Disposition: form-data; name="upload"; filename="fake.txt"
Content-Type: text/plain

this is the content of the fake file

---123--
"""
        environ = {
            'CONTENT_LENGTH':   str(len(data)),
            'CONTENT_TYPE':     'multipart/form-data; boundary=-123',
            'QUERY_STRING':     'key1=value1&key2=value2x',
            'REQUEST_METHOD':   'POST',
        }
        result = self._qs_result.copy()
        result.update({
            'upload': b'this is the content of the fake file\n'
        })
        v = gen_result(data, environ)
        self.assertEqual(result, v)

    def test_deprecated_parse_qs(self):
        # this func is moved to urllib.parse, this is just a sanity check
        with check_warnings(('cgi.parse_qs is deprecated, use urllib.parse.'
                             'parse_qs instead', DeprecationWarning)):
            self.assertEqual({'a': ['A1'], 'B': ['B3'], 'b': ['B2']},
                             cgi.parse_qs('a=A1&b=B2&B=B3'))

    def test_deprecated_parse_qsl(self):
        # this func is moved to urllib.parse, this is just a sanity check
        with check_warnings(('cgi.parse_qsl is deprecated, use urllib.parse.'
                             'parse_qsl instead', DeprecationWarning)):
            self.assertEqual([('a', 'A1'), ('b', 'B2'), ('B', 'B3')],
                             cgi.parse_qsl('a=A1&b=B2&B=B3'))

    def test_parse_header(self):
        self.assertEqual(
            cgi.parse_header("text/plain"),
            ("text/plain", {}))
        self.assertEqual(
            cgi.parse_header("text/vnd.just.made.this.up ; "),
            ("text/vnd.just.made.this.up", {}))
        self.assertEqual(
            cgi.parse_header("text/plain;charset=us-ascii"),
            ("text/plain", {"charset": "us-ascii"}))
        self.assertEqual(
            cgi.parse_header('text/plain ; charset="us-ascii"'),
            ("text/plain", {"charset": "us-ascii"}))
        self.assertEqual(
            cgi.parse_header('text/plain ; charset="us-ascii"; another=opt'),
            ("text/plain", {"charset": "us-ascii", "another": "opt"}))
        self.assertEqual(
            cgi.parse_header('attachment; filename="silly.txt"'),
            ("attachment", {"filename": "silly.txt"}))
        self.assertEqual(
            cgi.parse_header('attachment; filename="strange;name"'),
            ("attachment", {"filename": "strange;name"}))
        self.assertEqual(
            cgi.parse_header('attachment; filename="strange;name";size=123;'),
            ("attachment", {"filename": "strange;name", "size": "123"}))
        self.assertEqual(
            cgi.parse_header('form-data; name="files"; filename="fo\\"o;bar"'),
            ("form-data", {"name": "files", "filename": 'fo"o;bar'}))


BOUNDARY = "---------------------------721837373350705526688164684"

POSTDATA = """-----------------------------721837373350705526688164684
Content-Disposition: form-data; name="id"

1234
-----------------------------721837373350705526688164684
Content-Disposition: form-data; name="title"


-----------------------------721837373350705526688164684
Content-Disposition: form-data; name="file"; filename="test.txt"
Content-Type: text/plain

Testing 123.

-----------------------------721837373350705526688164684
Content-Disposition: form-data; name="submit"

 Add\x20
-----------------------------721837373350705526688164684--
"""

POSTDATA_NON_ASCII = """-----------------------------721837373350705526688164684
Content-Disposition: form-data; name="id"

\xe7\xf1\x80
-----------------------------721837373350705526688164684
"""

# http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4
BOUNDARY_W3 = "AaB03x"
POSTDATA_W3 = """--AaB03x
Content-Disposition: form-data; name="submit-name"

Larry
--AaB03x
Content-Disposition: form-data; name="files"
Content-Type: multipart/mixed; boundary=BbC04y

--BbC04y
Content-Disposition: file; filename="file1.txt"
Content-Type: text/plain

... contents of file1.txt ...
--BbC04y
Content-Disposition: file; filename="file2.gif"
Content-Type: image/gif
Content-Transfer-Encoding: binary

...contents of file2.gif...
--BbC04y--
--AaB03x--
"""


def test_main():
    run_unittest(CgiTests)

if __name__ == '__main__':
    test_main()
