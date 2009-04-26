import base64
import datetime
import sys
import time
import unittest
import xmlrpclib
import SimpleXMLRPCServer
import threading
import mimetools
import httplib
import socket
import StringIO
import os
from test import test_support

try:
    unicode
except NameError:
    have_unicode = False
else:
    have_unicode = True

alist = [{'astring': 'foo@bar.baz.spam',
          'afloat': 7283.43,
          'anint': 2**20,
          'ashortlong': 2L,
          'anotherlist': ['.zyx.41'],
          'abase64': xmlrpclib.Binary("my dog has fleas"),
          'boolean': xmlrpclib.False,
          'unicode': u'\u4000\u6000\u8000',
          u'ukey\u4000': 'regular value',
          'datetime1': xmlrpclib.DateTime('20050210T11:41:23'),
          'datetime2': xmlrpclib.DateTime(
                        (2005, 02, 10, 11, 41, 23, 0, 1, -1)),
          'datetime3': xmlrpclib.DateTime(
                        datetime.datetime(2005, 02, 10, 11, 41, 23)),
          }]

class XMLRPCTestCase(unittest.TestCase):

    def test_dump_load(self):
        self.assertEquals(alist,
                          xmlrpclib.loads(xmlrpclib.dumps((alist,)))[0][0])

    def test_dump_bare_datetime(self):
        # This checks that an unwrapped datetime.date object can be handled
        # by the marshalling code.  This can't be done via test_dump_load()
        # since with use_datetime set to 1 the unmarshaller would create
        # datetime objects for the 'datetime[123]' keys as well
        dt = datetime.datetime(2005, 02, 10, 11, 41, 23)
        s = xmlrpclib.dumps((dt,))
        (newdt,), m = xmlrpclib.loads(s, use_datetime=1)
        self.assertEquals(newdt, dt)
        self.assertEquals(m, None)

        (newdt,), m = xmlrpclib.loads(s, use_datetime=0)
        self.assertEquals(newdt, xmlrpclib.DateTime('20050210T11:41:23'))

    def test_datetime_before_1900(self):
        # same as before but with a date before 1900
        dt = datetime.datetime(1, 02, 10, 11, 41, 23)
        s = xmlrpclib.dumps((dt,))
        (newdt,), m = xmlrpclib.loads(s, use_datetime=1)
        self.assertEquals(newdt, dt)
        self.assertEquals(m, None)

        (newdt,), m = xmlrpclib.loads(s, use_datetime=0)
        self.assertEquals(newdt, xmlrpclib.DateTime('00010210T11:41:23'))

    def test_cmp_datetime_DateTime(self):
        now = datetime.datetime.now()
        dt = xmlrpclib.DateTime(now.timetuple())
        self.assert_(dt == now)
        self.assert_(now == dt)
        then = now + datetime.timedelta(seconds=4)
        self.assert_(then >= dt)
        self.assert_(dt < then)

    def test_bug_1164912 (self):
        d = xmlrpclib.DateTime()
        ((new_d,), dummy) = xmlrpclib.loads(xmlrpclib.dumps((d,),
                                            methodresponse=True))
        self.assert_(isinstance(new_d.value, str))

        # Check that the output of dumps() is still an 8-bit string
        s = xmlrpclib.dumps((new_d,), methodresponse=True)
        self.assert_(isinstance(s, str))

    def test_newstyle_class(self):
        class T(object):
            pass
        t = T()
        t.x = 100
        t.y = "Hello"
        ((t2,), dummy) = xmlrpclib.loads(xmlrpclib.dumps((t,)))
        self.assertEquals(t2, t.__dict__)

    def test_dump_big_long(self):
        self.assertRaises(OverflowError, xmlrpclib.dumps, (2L**99,))

    def test_dump_bad_dict(self):
        self.assertRaises(TypeError, xmlrpclib.dumps, ({(1,2,3): 1},))

    def test_dump_recursive_seq(self):
        l = [1,2,3]
        t = [3,4,5,l]
        l.append(t)
        self.assertRaises(TypeError, xmlrpclib.dumps, (l,))

    def test_dump_recursive_dict(self):
        d = {'1':1, '2':1}
        t = {'3':3, 'd':d}
        d['t'] = t
        self.assertRaises(TypeError, xmlrpclib.dumps, (d,))

    def test_dump_big_int(self):
        if sys.maxint > 2L**31-1:
            self.assertRaises(OverflowError, xmlrpclib.dumps,
                              (int(2L**34),))

        xmlrpclib.dumps((xmlrpclib.MAXINT, xmlrpclib.MININT))
        self.assertRaises(OverflowError, xmlrpclib.dumps, (xmlrpclib.MAXINT+1,))
        self.assertRaises(OverflowError, xmlrpclib.dumps, (xmlrpclib.MININT-1,))

        def dummy_write(s):
            pass

        m = xmlrpclib.Marshaller()
        m.dump_int(xmlrpclib.MAXINT, dummy_write)
        m.dump_int(xmlrpclib.MININT, dummy_write)
        self.assertRaises(OverflowError, m.dump_int, xmlrpclib.MAXINT+1, dummy_write)
        self.assertRaises(OverflowError, m.dump_int, xmlrpclib.MININT-1, dummy_write)


    def test_dump_none(self):
        value = alist + [None]
        arg1 = (alist + [None],)
        strg = xmlrpclib.dumps(arg1, allow_none=True)
        self.assertEquals(value,
                          xmlrpclib.loads(strg)[0][0])
        self.assertRaises(TypeError, xmlrpclib.dumps, (arg1,))

    def test_default_encoding_issues(self):
        # SF bug #1115989: wrong decoding in '_stringify'
        utf8 = """<?xml version='1.0' encoding='iso-8859-1'?>
                  <params>
                    <param><value>
                      <string>abc \x95</string>
                      </value></param>
                    <param><value>
                      <struct>
                        <member>
                          <name>def \x96</name>
                          <value><string>ghi \x97</string></value>
                          </member>
                        </struct>
                      </value></param>
                  </params>
                  """

        # sys.setdefaultencoding() normally doesn't exist after site.py is
        # loaded.  reload(sys) is the way to get it back.
        old_encoding = sys.getdefaultencoding()
        setdefaultencoding_existed = hasattr(sys, "setdefaultencoding")
        reload(sys) # ugh!
        sys.setdefaultencoding("iso-8859-1")
        try:
            (s, d), m = xmlrpclib.loads(utf8)
        finally:
            sys.setdefaultencoding(old_encoding)
            if not setdefaultencoding_existed:
                del sys.setdefaultencoding

        items = d.items()
        if have_unicode:
            self.assertEquals(s, u"abc \x95")
            self.assert_(isinstance(s, unicode))
            self.assertEquals(items, [(u"def \x96", u"ghi \x97")])
            self.assert_(isinstance(items[0][0], unicode))
            self.assert_(isinstance(items[0][1], unicode))
        else:
            self.assertEquals(s, "abc \xc2\x95")
            self.assertEquals(items, [("def \xc2\x96", "ghi \xc2\x97")])


class HelperTestCase(unittest.TestCase):
    def test_escape(self):
        self.assertEqual(xmlrpclib.escape("a&b"), "a&amp;b")
        self.assertEqual(xmlrpclib.escape("a<b"), "a&lt;b")
        self.assertEqual(xmlrpclib.escape("a>b"), "a&gt;b")

class FaultTestCase(unittest.TestCase):
    def test_repr(self):
        f = xmlrpclib.Fault(42, 'Test Fault')
        self.assertEqual(repr(f), "<Fault 42: 'Test Fault'>")
        self.assertEqual(repr(f), str(f))

    def test_dump_fault(self):
        f = xmlrpclib.Fault(42, 'Test Fault')
        s = xmlrpclib.dumps((f,))
        (newf,), m = xmlrpclib.loads(s)
        self.assertEquals(newf, {'faultCode': 42, 'faultString': 'Test Fault'})
        self.assertEquals(m, None)

        s = xmlrpclib.Marshaller().dumps(f)
        self.assertRaises(xmlrpclib.Fault, xmlrpclib.loads, s)


class DateTimeTestCase(unittest.TestCase):
    def test_default(self):
        t = xmlrpclib.DateTime()

    def test_time(self):
        d = 1181399930.036952
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), time.strftime("%Y%m%dT%H:%M:%S", time.localtime(d)))

    def test_time_tuple(self):
        d = (2007,6,9,10,38,50,5,160,0)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), '20070609T10:38:50')

    def test_time_struct(self):
        d = time.localtime(1181399930.036952)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t),  time.strftime("%Y%m%dT%H:%M:%S", d))

    def test_datetime_datetime(self):
        d = datetime.datetime(2007,1,2,3,4,5)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), '20070102T03:04:05')

    def test_repr(self):
        d = datetime.datetime(2007,1,2,3,4,5)
        t = xmlrpclib.DateTime(d)
        val ="<DateTime '20070102T03:04:05' at %x>" % id(t)
        self.assertEqual(repr(t), val)

    def test_decode(self):
        d = ' 20070908T07:11:13  '
        t1 = xmlrpclib.DateTime()
        t1.decode(d)
        tref = xmlrpclib.DateTime(datetime.datetime(2007,9,8,7,11,13))
        self.assertEqual(t1, tref)

        t2 = xmlrpclib._datetime(d)
        self.assertEqual(t1, tref)

class BinaryTestCase(unittest.TestCase):
    def test_default(self):
        t = xmlrpclib.Binary()
        self.assertEqual(str(t), '')

    def test_string(self):
        d = '\x01\x02\x03abc123\xff\xfe'
        t = xmlrpclib.Binary(d)
        self.assertEqual(str(t), d)

    def test_decode(self):
        d = '\x01\x02\x03abc123\xff\xfe'
        de = base64.encodestring(d)
        t1 = xmlrpclib.Binary()
        t1.decode(de)
        self.assertEqual(str(t1), d)

        t2 = xmlrpclib._binary(de)
        self.assertEqual(str(t2), d)


PORT = None

# The evt is set twice.  First when the server is ready to serve.
# Second when the server has been shutdown.  The user must clear
# the event after it has been set the first time to catch the second set.
def http_server(evt, numrequests):
    class TestInstanceClass:
        def div(self, x, y):
            return x // y

        def _methodHelp(self, name):
            if name == 'div':
                return 'This is the div function'

    def my_function():
        '''This is my function'''
        return True

    class MyXMLRPCServer(SimpleXMLRPCServer.SimpleXMLRPCServer):
        def get_request(self):
            # Ensure the socket is always non-blocking.  On Linux, socket
            # attributes are not inherited like they are on *BSD and Windows.
            s, port = self.socket.accept()
            s.setblocking(True)
            return s, port

    try:
        serv = MyXMLRPCServer(("localhost", 0),
                              logRequests=False, bind_and_activate=False)
        serv.socket.settimeout(3)
        serv.server_bind()
        global PORT
        PORT = serv.socket.getsockname()[1]
        serv.server_activate()
        serv.register_introspection_functions()
        serv.register_multicall_functions()
        serv.register_function(pow)
        serv.register_function(lambda x,y: x+y, 'add')
        serv.register_function(my_function)
        serv.register_instance(TestInstanceClass())
        evt.set()

        # handle up to 'numrequests' requests
        while numrequests > 0:
            serv.handle_request()
            numrequests -= 1

    except socket.timeout:
        pass
    finally:
        serv.socket.close()
        PORT = None
        evt.set()

# This function prevents errors like:
#    <ProtocolError for localhost:57527/RPC2: 500 Internal Server Error>
def is_unavailable_exception(e):
    '''Returns True if the given ProtocolError is the product of a server-side
       exception caused by the 'temporarily unavailable' response sometimes
       given by operations on non-blocking sockets.'''

    # sometimes we get a -1 error code and/or empty headers
    try:
        if e.errcode == -1 or e.headers is None:
            return True
        exc_mess = e.headers.get('X-exception')
    except AttributeError:
        # Ignore socket.errors here.
        exc_mess = str(e)

    if exc_mess and 'temporarily unavailable' in exc_mess.lower():
        return True

    return False

# NOTE: The tests in SimpleServerTestCase will ignore failures caused by
# "temporarily unavailable" exceptions raised in SimpleXMLRPCServer.  This
# condition occurs infrequently on some platforms, frequently on others, and
# is apparently caused by using SimpleXMLRPCServer with a non-blocking socket.
# If the server class is updated at some point in the future to handle this
# situation more gracefully, these tests should be modified appropriately.

class SimpleServerTestCase(unittest.TestCase):
    def setUp(self):
        # enable traceback reporting
        SimpleXMLRPCServer.SimpleXMLRPCServer._send_traceback_header = True

        self.evt = threading.Event()
        # start server thread to handle requests
        serv_args = (self.evt, 1)
        threading.Thread(target=http_server, args=serv_args).start()

        # wait for the server to be ready
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        # wait on the server thread to terminate
        self.evt.wait()

        # disable traceback reporting
        SimpleXMLRPCServer.SimpleXMLRPCServer._send_traceback_header = False

    def test_simple1(self):
        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            self.assertEqual(p.pow(6,8), 6**8)
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    # [ch] The test 404 is causing lots of false alarms.
    def XXXtest_404(self):
        # send POST with httplib, it should return 404 header and
        # 'Not Found' message.
        conn = httplib.HTTPConnection('localhost', PORT)
        conn.request('POST', '/this-is-not-valid')
        response = conn.getresponse()
        conn.close()

        self.assertEqual(response.status, 404)
        self.assertEqual(response.reason, 'Not Found')

    def test_introspection1(self):
        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            meth = p.system.listMethods()
            expected_methods = set(['pow', 'div', 'my_function', 'add',
                                    'system.listMethods', 'system.methodHelp',
                                    'system.methodSignature', 'system.multicall'])
            self.assertEqual(set(meth), expected_methods)
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_introspection2(self):
        try:
            # test _methodHelp()
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            divhelp = p.system.methodHelp('div')
            self.assertEqual(divhelp, 'This is the div function')
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_introspection3(self):
        try:
            # test native doc
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            myfunction = p.system.methodHelp('my_function')
            self.assertEqual(myfunction, 'This is my function')
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_introspection4(self):
        # the SimpleXMLRPCServer doesn't support signatures, but
        # at least check that we can try making the call
        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            divsig = p.system.methodSignature('div')
            self.assertEqual(divsig, 'signatures not supported')
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_multicall(self):
        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            multicall = xmlrpclib.MultiCall(p)
            multicall.add(2,3)
            multicall.pow(6,8)
            multicall.div(127,42)
            add_result, pow_result, div_result = multicall()
            self.assertEqual(add_result, 2+3)
            self.assertEqual(pow_result, 6**8)
            self.assertEqual(div_result, 127//42)
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_non_existing_multicall(self):
        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            multicall = xmlrpclib.MultiCall(p)
            multicall.this_is_not_exists()
            result = multicall()

            # result.results contains;
            # [{'faultCode': 1, 'faultString': '<type \'exceptions.Exception\'>:'
            #   'method "this_is_not_exists" is not supported'>}]

            self.assertEqual(result.results[0]['faultCode'], 1)
            self.assertEqual(result.results[0]['faultString'],
                '<type \'exceptions.Exception\'>:method "this_is_not_exists" '
                'is not supported')
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_dotted_attribute(self):
        # Raises an AttributeError because private methods are not allowed.
        self.assertRaises(AttributeError,
                          SimpleXMLRPCServer.resolve_dotted_attribute, str, '__add')

        self.assert_(SimpleXMLRPCServer.resolve_dotted_attribute(str, 'title'))
        # Get the test to run faster by sending a request with test_simple1.
        # This avoids waiting for the socket timeout.
        self.test_simple1()

# This is a contrived way to make a failure occur on the server side
# in order to test the _send_traceback_header flag on the server
class FailingMessageClass(mimetools.Message):
    def __getitem__(self, key):
        key = key.lower()
        if key == 'content-length':
            return 'I am broken'
        return mimetools.Message.__getitem__(self, key)


class FailingServerTestCase(unittest.TestCase):
    def setUp(self):
        self.evt = threading.Event()
        # start server thread to handle requests
        serv_args = (self.evt, 1)
        threading.Thread(target=http_server, args=serv_args).start()

        # wait for the server to be ready
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        # wait on the server thread to terminate
        self.evt.wait()
        # reset flag
        SimpleXMLRPCServer.SimpleXMLRPCServer._send_traceback_header = False
        # reset message class
        SimpleXMLRPCServer.SimpleXMLRPCRequestHandler.MessageClass = mimetools.Message

    def test_basic(self):
        # check that flag is false by default
        flagval = SimpleXMLRPCServer.SimpleXMLRPCServer._send_traceback_header
        self.assertEqual(flagval, False)

        # enable traceback reporting
        SimpleXMLRPCServer.SimpleXMLRPCServer._send_traceback_header = True

        # test a call that shouldn't fail just as a smoke test
        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            self.assertEqual(p.pow(6,8), 6**8)
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_fail_no_info(self):
        # use the broken message class
        SimpleXMLRPCServer.SimpleXMLRPCRequestHandler.MessageClass = FailingMessageClass

        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            p.pow(6,8)
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e) and hasattr(e, "headers"):
                # The two server-side error headers shouldn't be sent back in this case
                self.assertTrue(e.headers.get("X-exception") is None)
                self.assertTrue(e.headers.get("X-traceback") is None)
        else:
            self.fail('ProtocolError not raised')

    def test_fail_with_info(self):
        # use the broken message class
        SimpleXMLRPCServer.SimpleXMLRPCRequestHandler.MessageClass = FailingMessageClass

        # Check that errors in the server send back exception/traceback
        # info when flag is set
        SimpleXMLRPCServer.SimpleXMLRPCServer._send_traceback_header = True

        try:
            p = xmlrpclib.ServerProxy('http://localhost:%d' % PORT)
            p.pow(6,8)
        except (xmlrpclib.ProtocolError, socket.error), e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e) and hasattr(e, "headers"):
                # We should get error info in the response
                expected_err = "invalid literal for int() with base 10: 'I am broken'"
                self.assertEqual(e.headers.get("x-exception"), expected_err)
                self.assertTrue(e.headers.get("x-traceback") is not None)
        else:
            self.fail('ProtocolError not raised')

class CGIHandlerTestCase(unittest.TestCase):
    def setUp(self):
        self.cgi = SimpleXMLRPCServer.CGIXMLRPCRequestHandler()

    def tearDown(self):
        self.cgi = None

    def test_cgi_get(self):
        with test_support.EnvironmentVarGuard() as env:
            env.set('REQUEST_METHOD', 'GET')
            # if the method is GET and no request_text is given, it runs handle_get
            # get sysout output
            tmp = sys.stdout
            sys.stdout = open(test_support.TESTFN, "w")
            self.cgi.handle_request()
            sys.stdout.close()
            sys.stdout = tmp

            # parse Status header
            handle = open(test_support.TESTFN, "r").read()
            status = handle.split()[1]
            message = ' '.join(handle.split()[2:4])

            self.assertEqual(status, '400')
            self.assertEqual(message, 'Bad Request')

            os.remove(test_support.TESTFN)

    def test_cgi_xmlrpc_response(self):
        data = """<?xml version='1.0'?>
<methodCall>
    <methodName>test_method</methodName>
    <params>
        <param>
            <value><string>foo</string></value>
        </param>
        <param>
            <value><string>bar</string></value>
        </param>
     </params>
</methodCall>
"""
        open("xmldata.txt", "w").write(data)
        tmp1 = sys.stdin
        tmp2 = sys.stdout

        sys.stdin = open("xmldata.txt", "r")
        sys.stdout = open(test_support.TESTFN, "w")

        with test_support.EnvironmentVarGuard() as env:
            env.set('CONTENT_LENGTH', str(len(data)))
            self.cgi.handle_request()

        sys.stdin.close()
        sys.stdout.close()
        sys.stdin = tmp1
        sys.stdout = tmp2

        # will respond exception, if so, our goal is achieved ;)
        handle = open(test_support.TESTFN, "r").read()

        # start with 44th char so as not to get http header, we just need only xml
        self.assertRaises(xmlrpclib.Fault, xmlrpclib.loads, handle[44:])

        os.remove("xmldata.txt")
        os.remove(test_support.TESTFN)

class FakeSocket:

    def __init__(self):
        self.data = StringIO.StringIO()

    def send(self, buf):
        self.data.write(buf)
        return len(buf)

    def sendall(self, buf):
        self.data.write(buf)

    def getvalue(self):
        return self.data.getvalue()

    def makefile(self, x, y):
        raise RuntimeError

class FakeTransport(xmlrpclib.Transport):
    """A Transport instance that records instead of sending a request.

    This class replaces the actual socket used by httplib with a
    FakeSocket object that records the request.  It doesn't provide a
    response.
    """

    def make_connection(self, host):
        conn = xmlrpclib.Transport.make_connection(self, host)
        conn._conn.sock = self.fake_socket = FakeSocket()
        return conn

class TransportSubclassTestCase(unittest.TestCase):

    def issue_request(self, transport_class):
        """Return an HTTP request made via transport_class."""
        transport = transport_class()
        proxy = xmlrpclib.ServerProxy("http://example.com/",
                                      transport=transport)
        try:
            proxy.pow(6, 8)
        except RuntimeError:
            return transport.fake_socket.getvalue()
        return None

    def test_custom_user_agent(self):
        class TestTransport(FakeTransport):

            def send_user_agent(self, conn):
                xmlrpclib.Transport.send_user_agent(self, conn)
                conn.putheader("X-Test", "test_custom_user_agent")

        req = self.issue_request(TestTransport)
        self.assert_("X-Test: test_custom_user_agent\r\n" in req)

    def test_send_host(self):
        class TestTransport(FakeTransport):

            def send_host(self, conn, host):
                xmlrpclib.Transport.send_host(self, conn, host)
                conn.putheader("X-Test", "test_send_host")

        req = self.issue_request(TestTransport)
        self.assert_("X-Test: test_send_host\r\n" in req)

    def test_send_request(self):
        class TestTransport(FakeTransport):

            def send_request(self, conn, url, body):
                xmlrpclib.Transport.send_request(self, conn, url, body)
                conn.putheader("X-Test", "test_send_request")

        req = self.issue_request(TestTransport)
        self.assert_("X-Test: test_send_request\r\n" in req)

    def test_send_content(self):
        class TestTransport(FakeTransport):

            def send_content(self, conn, body):
                conn.putheader("X-Test", "test_send_content")
                xmlrpclib.Transport.send_content(self, conn, body)

        req = self.issue_request(TestTransport)
        self.assert_("X-Test: test_send_content\r\n" in req)

def test_main():
    xmlrpc_tests = [XMLRPCTestCase, HelperTestCase, DateTimeTestCase,
         BinaryTestCase, FaultTestCase, TransportSubclassTestCase]

    # The test cases against a SimpleXMLRPCServer raise a socket error
    # 10035 (WSAEWOULDBLOCK) in the server thread handle_request call when
    # run on Windows. This only happens on the first test to run, but it
    # fails every time and so these tests are skipped on win32 platforms.
    if sys.platform != 'win32':
        xmlrpc_tests.append(SimpleServerTestCase)
        xmlrpc_tests.append(FailingServerTestCase)
        xmlrpc_tests.append(CGIHandlerTestCase)

    test_support.run_unittest(*xmlrpc_tests)

if __name__ == "__main__":
    test_main()
