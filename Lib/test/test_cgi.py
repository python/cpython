from io import BytesIO
from test.test_support import run_unittest, check_warnings
import cgi
import os
import sys
import tempfile
import unittest

from collections import namedtuple

class HackedSysModule:
    # The regression test will have real values in sys.argv, which
    # will completely confuse the test of the cgi module
    argv = []
    stdin = sys.stdin

cgi.sys = HackedSysModule()

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

class ComparableException:
    def __init__(self, err):
        self.err = err

    def __str__(self):
        return str(self.err)

    def __cmp__(self, anExc):
        if not isinstance(anExc, Exception):
            return -1
        x = cmp(self.err.__class__, anExc.__class__)
        if x != 0:
            return x
        return cmp(self.err.args, anExc.args)

    def __getattr__(self, attr):
        return getattr(self.err, attr)

def do_test(buf, method):
    env = {}
    if method == "GET":
        fp = None
        env['REQUEST_METHOD'] = 'GET'
        env['QUERY_STRING'] = buf
    elif method == "POST":
        fp = StringIO(buf)
        env['REQUEST_METHOD'] = 'POST'
        env['CONTENT_TYPE'] = 'application/x-www-form-urlencoded'
        env['CONTENT_LENGTH'] = str(len(buf))
    else:
        raise ValueError, "unknown method: %s" % method
    try:
        return cgi.parse(fp, env, strict_parsing=1)
    except StandardError, err:
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

def first_elts(list):
    return map(lambda x:x[0], list)

def first_second_elts(list):
    return map(lambda p:(p[0], p[1][0]), list)

def gen_result(data, environ):
    fake_stdin = StringIO(data)
    fake_stdin.seek(0)
    form = cgi.FieldStorage(fp=fake_stdin, environ=environ)

    result = {}
    for k, v in dict(form).items():
        result[k] = isinstance(v, list) and form.getlist(k) or v.value

    return result

class CgiTests(unittest.TestCase):

    def test_escape(self):
        self.assertEqual("test &amp; string", cgi.escape("test & string"))
        self.assertEqual("&lt;test string&gt;", cgi.escape("<test string>"))
        self.assertEqual("&quot;test string&quot;", cgi.escape('"test string"', True))

    def test_strict(self):
        for orig, expect in parse_strict_test_cases:
            # Test basic parsing
            d = do_test(orig, "GET")
            self.assertEqual(d, expect, "Error parsing %s" % repr(orig))
            d = do_test(orig, "POST")
            self.assertEqual(d, expect, "Error parsing %s" % repr(orig))

            env = {'QUERY_STRING': orig}
            fcd = cgi.FormContentDict(env)
            sd = cgi.SvFormContentDict(env)
            fs = cgi.FieldStorage(environ=env)
            if isinstance(expect, dict):
                # test dict interface
                self.assertEqual(len(expect), len(fcd))
                self.assertItemsEqual(expect.keys(), fcd.keys())
                self.assertItemsEqual(expect.values(), fcd.values())
                self.assertItemsEqual(expect.items(), fcd.items())
                self.assertEqual(fcd.get("nonexistent field", "default"), "default")
                self.assertEqual(len(sd), len(fs))
                self.assertItemsEqual(sd.keys(), fs.keys())
                self.assertEqual(fs.getvalue("nonexistent field", "default"), "default")
                # test individual fields
                for key in expect.keys():
                    expect_val = expect[key]
                    self.assertTrue(fcd.has_key(key))
                    self.assertItemsEqual(fcd[key], expect[key])
                    self.assertEqual(fcd.get(key, "default"), fcd[key])
                    self.assertTrue(fs.has_key(key))
                    if len(expect_val) > 1:
                        single_value = 0
                    else:
                        single_value = 1
                    try:
                        val = sd[key]
                    except IndexError:
                        self.assertFalse(single_value)
                        self.assertEqual(fs.getvalue(key), expect_val)
                    else:
                        self.assertTrue(single_value)
                        self.assertEqual(val, expect_val[0])
                        self.assertEqual(fs.getvalue(key), expect_val[0])
                    self.assertItemsEqual(sd.getlist(key), expect_val)
                    if single_value:
                        self.assertItemsEqual(sd.values(),
                                                first_elts(expect.values()))
                        self.assertItemsEqual(sd.items(),
                                                first_second_elts(expect.items()))

    def test_weird_formcontentdict(self):
        # Test the weird FormContentDict classes
        env = {'QUERY_STRING': "x=1&y=2.0&z=2-3.%2b0&1=1abc"}
        expect = {'x': 1, 'y': 2.0, 'z': '2-3.+0', '1': '1abc'}
        d = cgi.InterpFormContentDict(env)
        for k, v in expect.items():
            self.assertEqual(d[k], v)
        for k, v in d.items():
            self.assertEqual(expect[k], v)
        self.assertItemsEqual(expect.values(), d.values())

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

        f = TestReadlineFile(tempfile.TemporaryFile())
        f.write('x' * 256 * 1024)
        f.seek(0)
        env = {'REQUEST_METHOD':'PUT'}
        fs = cgi.FieldStorage(fp=f, environ=env)
        # if we're not chunking properly, readline is only called twice
        # (by read_binary); if we are chunking properly, it will be called 5 times
        # as long as the chunksize is 1 << 16.
        self.assertGreater(f.numcalls, 2)

    def test_fieldstorage_invalid(self):
        fs = cgi.FieldStorage()
        self.assertFalse(fs)
        self.assertRaises(TypeError, bool(fs))
        self.assertEqual(list(fs), list(fs.keys()))
        fs.list.append(namedtuple('MockFieldStorage', 'name')('fieldvalue'))
        self.assertTrue(fs)

    def test_fieldstorage_multipart(self):
        #Test basic FieldStorage multipart parsing
        env = {'REQUEST_METHOD':'POST', 'CONTENT_TYPE':'multipart/form-data; boundary=---------------------------721837373350705526688164684', 'CONTENT_LENGTH':'558'}
        postdata = """-----------------------------721837373350705526688164684
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
        fs = cgi.FieldStorage(fp=StringIO(postdata), environ=env)
        self.assertEqual(len(fs.list), 4)
        expect = [{'name':'id', 'filename':None, 'value':'1234'},
                  {'name':'title', 'filename':None, 'value':''},
                  {'name':'file', 'filename':'test.txt','value':'Testing 123.\n'},
                  {'name':'submit', 'filename':None, 'value':' Add '}]
        for x in range(len(fs.list)):
            for k, exp in expect[x].items():
                got = getattr(fs.list[x], k)
                self.assertEqual(got, exp)

    def test_fieldstorage_multipart_maxline(self):
        # Issue #18167
        maxline = 1 << 16
        self.maxDiff = None
        def check(content):
            data = """
---123
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
            self.assertEqual(gen_result(data, environ), {'upload': content})
        check('x' * (maxline - 1))
        check('x' * (maxline - 1) + '\r')
        check('x' * (maxline - 1) + '\r' + 'y' * (maxline - 1))

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

    def test_max_num_fields(self):
        # For application/x-www-form-urlencoded
        data = '&'.join(['a=a']*11)
        environ = {
            'CONTENT_LENGTH': str(len(data)),
            'CONTENT_TYPE': 'application/x-www-form-urlencoded',
            'REQUEST_METHOD': 'POST',
        }

        with self.assertRaises(ValueError):
            cgi.FieldStorage(
                fp=BytesIO(data.encode()),
                environ=environ,
                max_num_fields=10,
            )

        # For multipart/form-data
        data = """---123
Content-Disposition: form-data; name="a"

3
---123
Content-Type: application/x-www-form-urlencoded

a=4
---123
Content-Type: application/x-www-form-urlencoded

a=5
---123--
"""
        environ = {
            'CONTENT_LENGTH':   str(len(data)),
            'CONTENT_TYPE':     'multipart/form-data; boundary=-123',
            'QUERY_STRING':     'a=1&a=2',
            'REQUEST_METHOD':   'POST',
        }

        # 2 GET entities
        # 1 top level POST entities
        # 1 entity within the second POST entity
        # 1 entity within the third POST entity
        with self.assertRaises(ValueError):
            cgi.FieldStorage(
                fp=BytesIO(data.encode()),
                environ=environ,
                max_num_fields=4,
            )
        cgi.FieldStorage(
            fp=BytesIO(data.encode()),
            environ=environ,
            max_num_fields=5,
        )

    def testQSAndFormData(self):
        data = """
---123
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
        data = """
---123
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
            'upload': 'this is the content of the fake file\n'
        })
        v = gen_result(data, environ)
        self.assertEqual(result, v)

    def test_deprecated_parse_qs(self):
        # this func is moved to urlparse, this is just a sanity check
        with check_warnings(('cgi.parse_qs is deprecated, use urlparse.'
                             'parse_qs instead', PendingDeprecationWarning)):
            self.assertEqual({'a': ['A1'], 'B': ['B3'], 'b': ['B2']},
                             cgi.parse_qs('a=A1&b=B2&B=B3'))

    def test_deprecated_parse_qsl(self):
        # this func is moved to urlparse, this is just a sanity check
        with check_warnings(('cgi.parse_qsl is deprecated, use urlparse.'
                             'parse_qsl instead', PendingDeprecationWarning)):
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


def test_main():
    run_unittest(CgiTests)

if __name__ == '__main__':
    test_main()
