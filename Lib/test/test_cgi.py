from test_support import verify, verbose
import cgi
import os
import sys

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

# A list of test cases.  Each test case is a a two-tuple that contains
# a string with the query and a dictionary with the expected result.

parse_test_cases = [
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

def norm(list):
    if type(list) == type([]):
        list.sort()
    return list

def first_elts(list):
    return map(lambda x:x[0], list)

def first_second_elts(list):
    return map(lambda p:(p[0], p[1][0]), list)

def main():
    for orig, expect in parse_test_cases:
        # Test basic parsing
        print repr(orig)
        d = do_test(orig, "GET")
        verify(d == expect, "Error parsing %s" % repr(orig))
        d = do_test(orig, "POST")
        verify(d == expect, "Error parsing %s" % repr(orig))

        env = {'QUERY_STRING': orig}
        fcd = cgi.FormContentDict(env)
        sd = cgi.SvFormContentDict(env)
        fs = cgi.FieldStorage(environ=env)
        if type(expect) == type({}):
            # test dict interface
            verify(len(expect) == len(fcd))
            verify(norm(expect.keys()) == norm(fcd.keys()))
            verify(norm(expect.values()) == norm(fcd.values()))
            verify(norm(expect.items()) == norm(fcd.items()))
            verify(fcd.get("nonexistent field", "default") == "default")
            verify(len(sd) == len(fs))
            verify(norm(sd.keys()) == norm(fs.keys()))
            verify(fs.getvalue("nonexistent field", "default") == "default")
            # test individual fields
            for key in expect.keys():
                expect_val = expect[key]
                verify(fcd.has_key(key))
                verify(norm(fcd[key]) == norm(expect[key]))
                verify(fcd.get(key, "default") == fcd[key])
                verify(fs.has_key(key))
                if len(expect_val) > 1:
                    single_value = 0
                else:
                    single_value = 1
                try:
                    val = sd[key]
                except IndexError:
                    verify(not single_value)
                    verify(fs.getvalue(key) == expect_val)
                else:
                    verify(single_value)
                    verify(val == expect_val[0])
                    verify(fs.getvalue(key) == expect_val[0])
                verify(norm(sd.getlist(key)) == norm(expect_val))
                if single_value:
                    verify(norm(sd.values()) == \
                           first_elts(norm(expect.values())))
                    verify(norm(sd.items()) == \
                           first_second_elts(norm(expect.items())))

    # Test the weird FormContentDict classes
    env = {'QUERY_STRING': "x=1&y=2.0&z=2-3.%2b0&1=1abc"}
    expect = {'x': 1, 'y': 2.0, 'z': '2-3.+0', '1': '1abc'}
    d = cgi.InterpFormContentDict(env)
    for k, v in expect.items():
        verify(d[k] == v)
    for k, v in d.items():
        verify(expect[k] == v)
    verify(norm(expect.values()) == norm(d.values()))

    print "Testing log"
    cgi.initlog()
    cgi.log("Testing")
    cgi.logfp = sys.stdout
    cgi.initlog("%s", "Testing initlog 1")
    cgi.log("%s", "Testing log 2")
    if os.path.exists("/dev/null"):
        cgi.logfp = None
        cgi.logfile = "/dev/null"
        cgi.initlog("%s", "Testing log 3")
        cgi.log("Testing log 4")

main()
