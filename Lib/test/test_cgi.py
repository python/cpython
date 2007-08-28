from test.test_support import run_unittest
import cgi
import os
import sys
import tempfile
import unittest
from io import StringIO

class HackedSysModule:
    # The regression test will have real values in sys.argv, which
    # will completely confuse the test of the cgi module
    argv = []
    stdin = sys.stdin

cgi.sys = HackedSysModule()

try:
    from io import StringIO
except ImportError:
    from io import StringIO

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
        fp = StringIO(buf)
        env['REQUEST_METHOD'] = 'POST'
        env['CONTENT_TYPE'] = 'application/x-www-form-urlencoded'
        env['CONTENT_LENGTH'] = str(len(buf))
    else:
        raise ValueError, "unknown method: %s" % method
    try:
        return cgi.parse(fp, env, strict_parsing=1)
    except Exception as err:
        return ComparableException(err)

# A list of test cases.  Each test case is a a two-tuple that contains
# a string with the query and a dictionary with the expected result.

parse_qsl_test_cases = [
    ("", []),
    ("&", []),
    ("&&", []),
    ("=", [('', '')]),
    ("=a", [('', 'a')]),
    ("a", [('a', '')]),
    ("a=", [('a', '')]),
    ("a=", [('a', '')]),
    ("&a=b", [('a', 'b')]),
    ("a=a+b&b=b+c", [('a', 'a b'), ('b', 'b c')]),
    ("a=1&a=2", [('a', '1'), ('a', '2')]),
]

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


class CgiTests(unittest.TestCase):

    def test_qsl(self):
        for orig, expect in parse_qsl_test_cases:
            result = cgi.parse_qsl(orig, keep_blank_values=True)
            self.assertEqual(result, expect, "Error parsing %s" % repr(orig))

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
            if type(expect) == type({}):
                # test dict interface
                self.assertEqual(len(expect), len(fcd))
                self.assertEqual(norm(expect.keys()), norm(fcd.keys()))
                self.assertEqual(norm(expect.values()), norm(fcd.values()))
                self.assertEqual(norm(expect.items()), norm(fcd.items()))
                self.assertEqual(fcd.get("nonexistent field", "default"), "default")
                self.assertEqual(len(sd), len(fs))
                self.assertEqual(norm(sd.keys()), norm(fs.keys()))
                self.assertEqual(fs.getvalue("nonexistent field", "default"), "default")
                # test individual fields
                for key in expect.keys():
                    expect_val = expect[key]
                    self.assert_(key in fcd)
                    self.assertEqual(norm(fcd[key]), norm(expect[key]))
                    self.assertEqual(fcd.get(key, "default"), fcd[key])
                    self.assert_(key in fs)
                    if len(expect_val) > 1:
                        single_value = 0
                    else:
                        single_value = 1
                    try:
                        val = sd[key]
                    except IndexError:
                        self.failIf(single_value)
                        self.assertEqual(fs.getvalue(key), expect_val)
                    else:
                        self.assert_(single_value)
                        self.assertEqual(val, expect_val[0])
                        self.assertEqual(fs.getvalue(key), expect_val[0])
                    self.assertEqual(norm(sd.getlist(key)), norm(expect_val))
                    if single_value:
                        self.assertEqual(norm(sd.values()),
                               first_elts(norm(expect.values())))
                        self.assertEqual(norm(sd.items()),
                               first_second_elts(norm(expect.items())))

    def test_weird_formcontentdict(self):
        # Test the weird FormContentDict classes
        env = {'QUERY_STRING': "x=1&y=2.0&z=2-3.%2b0&1=1abc"}
        expect = {'x': 1, 'y': 2.0, 'z': '2-3.+0', '1': '1abc'}
        d = cgi.InterpFormContentDict(env)
        for k, v in expect.items():
            self.assertEqual(d[k], v)
        for k, v in d.items():
            self.assertEqual(expect[k], v)
        self.assertEqual(norm(expect.values()), norm(d.values()))

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

        f = TestReadlineFile(tempfile.TemporaryFile("w+"))
        f.write('x' * 256 * 1024)
        f.seek(0)
        env = {'REQUEST_METHOD':'PUT'}
        fs = cgi.FieldStorage(fp=f, environ=env)
        # if we're not chunking properly, readline is only called twice
        # (by read_binary); if we are chunking properly, it will be called 5 times
        # as long as the chunksize is 1 << 16.
        self.assert_(f.numcalls > 2)

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
        self.assertEquals(len(fs.list), 4)
        expect = [{'name':'id', 'filename':None, 'value':'1234'},
                  {'name':'title', 'filename':None, 'value':''},
                  {'name':'file', 'filename':'test.txt','value':'Testing 123.\n'},
                  {'name':'submit', 'filename':None, 'value':' Add '}]
        for x in range(len(fs.list)):
            for k, exp in expect[x].items():
                got = getattr(fs.list[x], k)
                self.assertEquals(got, exp)

def test_main():
    run_unittest(CgiTests)

if __name__ == '__main__':
    test_main()
