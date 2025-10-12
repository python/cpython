import base64
import datetime
import decimal
import sys
import time
import unittest
from unittest import mock
import xmlrpc.client as xmlrpclib
import xmlrpc.server
import http.client
import http, http.server
import socket
import threading
import re
import io
import contextlib
from test import support
from test.support import os_helper
from test.support import socket_helper
from test.support import threading_helper
from test.support import ALWAYS_EQ, LARGEST, SMALLEST

try:
    import gzip
except ImportError:
    gzip = None

support.requires_working_socket(module=True)

alist = [{'astring': 'foo@bar.baz.spam',
          'afloat': 7283.43,
          'anint': 2**20,
          'ashortlong': 2,
          'anotherlist': ['.zyx.41'],
          'abase64': xmlrpclib.Binary(b"my dog has fleas"),
          'b64bytes': b"my dog has fleas",
          'b64bytearray': bytearray(b"my dog has fleas"),
          'boolean': False,
          'unicode': '\u4000\u6000\u8000',
          'ukey\u4000': 'regular value',
          'datetime1': xmlrpclib.DateTime('20050210T11:41:23'),
          'datetime2': xmlrpclib.DateTime(
                        (2005, 2, 10, 11, 41, 23, 0, 1, -1)),
          'datetime3': xmlrpclib.DateTime(
                        datetime.datetime(2005, 2, 10, 11, 41, 23)),
          }]

class XMLRPCTestCase(unittest.TestCase):

    def test_dump_load(self):
        dump = xmlrpclib.dumps((alist,))
        load = xmlrpclib.loads(dump)
        self.assertEqual(alist, load[0][0])

    def test_dump_bare_datetime(self):
        # This checks that an unwrapped datetime.date object can be handled
        # by the marshalling code.  This can't be done via test_dump_load()
        # since with use_builtin_types set to 1 the unmarshaller would create
        # datetime objects for the 'datetime[123]' keys as well
        dt = datetime.datetime(2005, 2, 10, 11, 41, 23)
        self.assertEqual(dt, xmlrpclib.DateTime('20050210T11:41:23'))
        s = xmlrpclib.dumps((dt,))

        result, m = xmlrpclib.loads(s, use_builtin_types=True)
        (newdt,) = result
        self.assertEqual(newdt, dt)
        self.assertIs(type(newdt), datetime.datetime)
        self.assertIsNone(m)

        result, m = xmlrpclib.loads(s, use_builtin_types=False)
        (newdt,) = result
        self.assertEqual(newdt, dt)
        self.assertIs(type(newdt), xmlrpclib.DateTime)
        self.assertIsNone(m)

        result, m = xmlrpclib.loads(s, use_datetime=True)
        (newdt,) = result
        self.assertEqual(newdt, dt)
        self.assertIs(type(newdt), datetime.datetime)
        self.assertIsNone(m)

        result, m = xmlrpclib.loads(s, use_datetime=False)
        (newdt,) = result
        self.assertEqual(newdt, dt)
        self.assertIs(type(newdt), xmlrpclib.DateTime)
        self.assertIsNone(m)


    def test_datetime_before_1900(self):
        # same as before but with a date before 1900
        dt = datetime.datetime(1,  2, 10, 11, 41, 23)
        self.assertEqual(dt, xmlrpclib.DateTime('00010210T11:41:23'))
        s = xmlrpclib.dumps((dt,))

        result, m = xmlrpclib.loads(s, use_builtin_types=True)
        (newdt,) = result
        self.assertEqual(newdt, dt)
        self.assertIs(type(newdt), datetime.datetime)
        self.assertIsNone(m)

        result, m = xmlrpclib.loads(s, use_builtin_types=False)
        (newdt,) = result
        self.assertEqual(newdt, dt)
        self.assertIs(type(newdt), xmlrpclib.DateTime)
        self.assertIsNone(m)

    def test_bug_1164912 (self):
        d = xmlrpclib.DateTime()
        ((new_d,), dummy) = xmlrpclib.loads(xmlrpclib.dumps((d,),
                                            methodresponse=True))
        self.assertIsInstance(new_d.value, str)

        # Check that the output of dumps() is still an 8-bit string
        s = xmlrpclib.dumps((new_d,), methodresponse=True)
        self.assertIsInstance(s, str)

    def test_newstyle_class(self):
        class T(object):
            pass
        t = T()
        t.x = 100
        t.y = "Hello"
        ((t2,), dummy) = xmlrpclib.loads(xmlrpclib.dumps((t,)))
        self.assertEqual(t2, t.__dict__)

    def test_dump_big_long(self):
        self.assertRaises(OverflowError, xmlrpclib.dumps, (2**99,))

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
        if sys.maxsize > 2**31-1:
            self.assertRaises(OverflowError, xmlrpclib.dumps,
                              (int(2**34),))

        xmlrpclib.dumps((xmlrpclib.MAXINT, xmlrpclib.MININT))
        self.assertRaises(OverflowError, xmlrpclib.dumps,
                          (xmlrpclib.MAXINT+1,))
        self.assertRaises(OverflowError, xmlrpclib.dumps,
                          (xmlrpclib.MININT-1,))

        def dummy_write(s):
            pass

        m = xmlrpclib.Marshaller()
        m.dump_int(xmlrpclib.MAXINT, dummy_write)
        m.dump_int(xmlrpclib.MININT, dummy_write)
        self.assertRaises(OverflowError, m.dump_int,
                          xmlrpclib.MAXINT+1, dummy_write)
        self.assertRaises(OverflowError, m.dump_int,
                          xmlrpclib.MININT-1, dummy_write)

    def test_dump_double(self):
        xmlrpclib.dumps((float(2 ** 34),))
        xmlrpclib.dumps((float(xmlrpclib.MAXINT),
                         float(xmlrpclib.MININT)))
        xmlrpclib.dumps((float(xmlrpclib.MAXINT + 42),
                         float(xmlrpclib.MININT - 42)))

        def dummy_write(s):
            pass

        m = xmlrpclib.Marshaller()
        m.dump_double(xmlrpclib.MAXINT, dummy_write)
        m.dump_double(xmlrpclib.MININT, dummy_write)
        m.dump_double(xmlrpclib.MAXINT + 42, dummy_write)
        m.dump_double(xmlrpclib.MININT - 42, dummy_write)

    def test_dump_none(self):
        value = alist + [None]
        arg1 = (alist + [None],)
        strg = xmlrpclib.dumps(arg1, allow_none=True)
        self.assertEqual(value,
                          xmlrpclib.loads(strg)[0][0])
        self.assertRaises(TypeError, xmlrpclib.dumps, (arg1,))

    def test_dump_encoding(self):
        value = {'key\u20ac\xa4':
                 'value\u20ac\xa4'}
        strg = xmlrpclib.dumps((value,), encoding='iso-8859-15')
        strg = "<?xml version='1.0' encoding='iso-8859-15'?>" + strg
        self.assertEqual(xmlrpclib.loads(strg)[0][0], value)
        strg = strg.encode('iso-8859-15', 'xmlcharrefreplace')
        self.assertEqual(xmlrpclib.loads(strg)[0][0], value)

        strg = xmlrpclib.dumps((value,), encoding='iso-8859-15',
                               methodresponse=True)
        self.assertEqual(xmlrpclib.loads(strg)[0][0], value)
        strg = strg.encode('iso-8859-15', 'xmlcharrefreplace')
        self.assertEqual(xmlrpclib.loads(strg)[0][0], value)

        methodname = 'method\u20ac\xa4'
        strg = xmlrpclib.dumps((value,), encoding='iso-8859-15',
                               methodname=methodname)
        self.assertEqual(xmlrpclib.loads(strg)[0][0], value)
        self.assertEqual(xmlrpclib.loads(strg)[1], methodname)

    def test_dump_bytes(self):
        sample = b"my dog has fleas"
        self.assertEqual(sample, xmlrpclib.Binary(sample))
        for type_ in bytes, bytearray, xmlrpclib.Binary:
            value = type_(sample)
            s = xmlrpclib.dumps((value,))

            result, m = xmlrpclib.loads(s, use_builtin_types=True)
            (newvalue,) = result
            self.assertEqual(newvalue, sample)
            self.assertIs(type(newvalue), bytes)
            self.assertIsNone(m)

            result, m = xmlrpclib.loads(s, use_builtin_types=False)
            (newvalue,) = result
            self.assertEqual(newvalue, sample)
            self.assertIs(type(newvalue), xmlrpclib.Binary)
            self.assertIsNone(m)

    def test_loads_unsupported(self):
        ResponseError = xmlrpclib.ResponseError
        data = '<params><param><value><spam/></value></param></params>'
        self.assertRaises(ResponseError, xmlrpclib.loads, data)
        data = ('<params><param><value><array>'
                '<value><spam/></value>'
                '</array></value></param></params>')
        self.assertRaises(ResponseError, xmlrpclib.loads, data)
        data = ('<params><param><value><struct>'
                '<member><name>a</name><value><spam/></value></member>'
                '<member><name>b</name><value><spam/></value></member>'
                '</struct></value></param></params>')
        self.assertRaises(ResponseError, xmlrpclib.loads, data)

    def check_loads(self, s, value, **kwargs):
        dump = '<params><param><value>%s</value></param></params>' % s
        result, m = xmlrpclib.loads(dump, **kwargs)
        (newvalue,) = result
        self.assertEqual(newvalue, value)
        self.assertIs(type(newvalue), type(value))
        self.assertIsNone(m)

    def test_load_standard_types(self):
        check = self.check_loads
        check('string', 'string')
        check('<string>string</string>', 'string')
        check('<string>𝔘𝔫𝔦𝔠𝔬𝔡𝔢 string</string>', '𝔘𝔫𝔦𝔠𝔬𝔡𝔢 string')
        check('<int>2056183947</int>', 2056183947)
        check('<int>-2056183947</int>', -2056183947)
        check('<i4>2056183947</i4>', 2056183947)
        check('<double>46093.78125</double>', 46093.78125)
        check('<boolean>0</boolean>', False)
        check('<base64>AGJ5dGUgc3RyaW5n/w==</base64>',
              xmlrpclib.Binary(b'\x00byte string\xff'))
        check('<base64>AGJ5dGUgc3RyaW5n/w==</base64>',
              b'\x00byte string\xff', use_builtin_types=True)
        check('<dateTime.iso8601>20050210T11:41:23</dateTime.iso8601>',
              xmlrpclib.DateTime('20050210T11:41:23'))
        check('<dateTime.iso8601>20050210T11:41:23</dateTime.iso8601>',
              datetime.datetime(2005, 2, 10, 11, 41, 23),
              use_builtin_types=True)
        check('<array><data>'
              '<value><int>1</int></value><value><int>2</int></value>'
              '</data></array>', [1, 2])
        check('<struct>'
              '<member><name>b</name><value><int>2</int></value></member>'
              '<member><name>a</name><value><int>1</int></value></member>'
              '</struct>', {'a': 1, 'b': 2})

    def test_load_extension_types(self):
        check = self.check_loads
        check('<nil/>', None)
        check('<ex:nil/>', None)
        check('<i1>205</i1>', 205)
        check('<i2>20561</i2>', 20561)
        check('<i8>9876543210</i8>', 9876543210)
        check('<biginteger>98765432100123456789</biginteger>',
              98765432100123456789)
        check('<float>93.78125</float>', 93.78125)
        check('<bigdecimal>9876543210.0123456789</bigdecimal>',
              decimal.Decimal('9876543210.0123456789'))

    def test_limit_int(self):
        check = self.check_loads
        maxdigits = 5000
        with support.adjust_int_max_str_digits(maxdigits):
            s = '1' * (maxdigits + 1)
            with self.assertRaises(ValueError):
                check(f'<int>{s}</int>', None)
            with self.assertRaises(ValueError):
                check(f'<biginteger>{s}</biginteger>', None)

    def test_get_host_info(self):
        # see bug #3613, this raised a TypeError
        transp = xmlrpc.client.Transport()
        self.assertEqual(transp.get_host_info("user@host.tld"),
                          ('host.tld',
                           [('Authorization', 'Basic dXNlcg==')], {}))

    def test_ssl_presence(self):
        try:
            import ssl  # noqa: F401
        except ImportError:
            has_ssl = False
        else:
            has_ssl = True
        try:
            xmlrpc.client.ServerProxy('https://localhost:9999').bad_function()
        except NotImplementedError:
            self.assertFalse(has_ssl, "xmlrpc client's error with SSL support")
        except OSError:
            self.assertTrue(has_ssl)

    def test_keepalive_disconnect(self):
        class RequestHandler(http.server.BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.1"
            handled = False

            def do_POST(self):
                length = int(self.headers.get("Content-Length"))
                self.rfile.read(length)
                if self.handled:
                    self.close_connection = True
                    return
                response = xmlrpclib.dumps((5,), methodresponse=True)
                response = response.encode()
                self.send_response(http.HTTPStatus.OK)
                self.send_header("Content-Length", len(response))
                self.end_headers()
                self.wfile.write(response)
                self.handled = True
                self.close_connection = False

            def log_message(self, format, *args):
                # don't clobber sys.stderr
                pass

        def run_server():
            server.socket.settimeout(float(1))  # Don't hang if client fails
            server.handle_request()  # First request and attempt at second
            server.handle_request()  # Retried second request

        server = http.server.HTTPServer((socket_helper.HOST, 0), RequestHandler)
        self.addCleanup(server.server_close)
        thread = threading.Thread(target=run_server)
        thread.start()
        self.addCleanup(thread.join)
        url = "http://{}:{}/".format(*server.server_address)
        with xmlrpclib.ServerProxy(url) as p:
            self.assertEqual(p.method(), 5)
            self.assertEqual(p.method(), 5)


class SimpleXMLRPCDispatcherTestCase(unittest.TestCase):
    class DispatchExc(Exception):
        """Raised inside the dispatched functions when checking for
        chained exceptions"""

    def test_call_registered_func(self):
        """Calls explicitly registered function"""
        # Makes sure any exception raised inside the function has no other
        # exception chained to it

        exp_params = 1, 2, 3

        def dispatched_func(*params):
            raise self.DispatchExc(params)

        dispatcher = xmlrpc.server.SimpleXMLRPCDispatcher()
        dispatcher.register_function(dispatched_func)
        with self.assertRaises(self.DispatchExc) as exc_ctx:
            dispatcher._dispatch('dispatched_func', exp_params)
        self.assertEqual(exc_ctx.exception.args, (exp_params,))
        self.assertIsNone(exc_ctx.exception.__cause__)
        self.assertIsNone(exc_ctx.exception.__context__)

    def test_call_instance_func(self):
        """Calls a registered instance attribute as a function"""
        # Makes sure any exception raised inside the function has no other
        # exception chained to it

        exp_params = 1, 2, 3

        class DispatchedClass:
            def dispatched_func(self, *params):
                raise SimpleXMLRPCDispatcherTestCase.DispatchExc(params)

        dispatcher = xmlrpc.server.SimpleXMLRPCDispatcher()
        dispatcher.register_instance(DispatchedClass())
        with self.assertRaises(self.DispatchExc) as exc_ctx:
            dispatcher._dispatch('dispatched_func', exp_params)
        self.assertEqual(exc_ctx.exception.args, (exp_params,))
        self.assertIsNone(exc_ctx.exception.__cause__)
        self.assertIsNone(exc_ctx.exception.__context__)

    def test_call_dispatch_func(self):
        """Calls the registered instance's `_dispatch` function"""
        # Makes sure any exception raised inside the function has no other
        # exception chained to it

        exp_method = 'method'
        exp_params = 1, 2, 3

        class TestInstance:
            def _dispatch(self, method, params):
                raise SimpleXMLRPCDispatcherTestCase.DispatchExc(
                    method, params)

        dispatcher = xmlrpc.server.SimpleXMLRPCDispatcher()
        dispatcher.register_instance(TestInstance())
        with self.assertRaises(self.DispatchExc) as exc_ctx:
            dispatcher._dispatch(exp_method, exp_params)
        self.assertEqual(exc_ctx.exception.args, (exp_method, exp_params))
        self.assertIsNone(exc_ctx.exception.__cause__)
        self.assertIsNone(exc_ctx.exception.__context__)

    def test_registered_func_is_none(self):
        """Calls explicitly registered function which is None"""

        dispatcher = xmlrpc.server.SimpleXMLRPCDispatcher()
        dispatcher.register_function(None, name='method')
        with self.assertRaisesRegex(Exception, 'method'):
            dispatcher._dispatch('method', ('param',))

    def test_instance_has_no_func(self):
        """Attempts to call nonexistent function on a registered instance"""

        dispatcher = xmlrpc.server.SimpleXMLRPCDispatcher()
        dispatcher.register_instance(object())
        with self.assertRaisesRegex(Exception, 'method'):
            dispatcher._dispatch('method', ('param',))

    def test_cannot_locate_func(self):
        """Calls a function that the dispatcher cannot locate"""

        dispatcher = xmlrpc.server.SimpleXMLRPCDispatcher()
        with self.assertRaisesRegex(Exception, 'method'):
            dispatcher._dispatch('method', ('param',))


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
        self.assertEqual(newf, {'faultCode': 42, 'faultString': 'Test Fault'})
        self.assertEqual(m, None)

        s = xmlrpclib.Marshaller().dumps(f)
        self.assertRaises(xmlrpclib.Fault, xmlrpclib.loads, s)

    def test_dotted_attribute(self):
        # this will raise AttributeError because code don't want us to use
        # private methods
        self.assertRaises(AttributeError,
                          xmlrpc.server.resolve_dotted_attribute, str, '__add')
        self.assertTrue(xmlrpc.server.resolve_dotted_attribute(str, 'title'))

class DateTimeTestCase(unittest.TestCase):
    def test_default(self):
        with mock.patch('time.localtime') as localtime_mock:
            time_struct = time.struct_time(
                [2013, 7, 15, 0, 24, 49, 0, 196, 0])
            localtime_mock.return_value = time_struct
            localtime = time.localtime()
            t = xmlrpclib.DateTime()
            self.assertEqual(str(t),
                             time.strftime("%Y%m%dT%H:%M:%S", localtime))

    def test_time(self):
        d = 1181399930.036952
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t),
                         time.strftime("%Y%m%dT%H:%M:%S", time.localtime(d)))

    def test_time_tuple(self):
        d = (2007,6,9,10,38,50,5,160,0)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), '20070609T10:38:50')

    def test_time_struct(self):
        d = time.localtime(1181399930.036952)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), time.strftime("%Y%m%dT%H:%M:%S", d))

    def test_datetime_datetime(self):
        # naive (no tzinfo)
        d = datetime.datetime(2007,1,2,3,4,5)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), '20070102T03:04:05')

        # aware (with tzinfo): the timezone is ignored
        d = datetime.datetime(2023, 6, 12, 13, 30, tzinfo=datetime.UTC)
        t = xmlrpclib.DateTime(d)
        self.assertEqual(str(t), '20230612T13:30:00')

    def test_repr(self):
        d = datetime.datetime(2007,1,2,3,4,5)
        t = xmlrpclib.DateTime(d)
        val ="<DateTime '20070102T03:04:05' at %#x>" % id(t)
        self.assertEqual(repr(t), val)

    def test_decode(self):
        d = ' 20070908T07:11:13  '
        t1 = xmlrpclib.DateTime()
        t1.decode(d)
        tref = xmlrpclib.DateTime(datetime.datetime(2007,9,8,7,11,13))
        self.assertEqual(t1, tref)

        t2 = xmlrpclib._datetime(d)
        self.assertEqual(t2, tref)

    def test_comparison(self):
        now = datetime.datetime.now()
        dtime = xmlrpclib.DateTime(now.timetuple())

        # datetime vs. DateTime
        self.assertTrue(dtime == now)
        self.assertTrue(now == dtime)
        then = now + datetime.timedelta(seconds=4)
        self.assertTrue(then >= dtime)
        self.assertTrue(dtime < then)

        # str vs. DateTime
        dstr = now.strftime("%Y%m%dT%H:%M:%S")
        self.assertTrue(dtime == dstr)
        self.assertTrue(dstr == dtime)
        dtime_then = xmlrpclib.DateTime(then.timetuple())
        self.assertTrue(dtime_then >= dstr)
        self.assertTrue(dstr < dtime_then)

        # some other types
        dbytes = dstr.encode('ascii')
        dtuple = now.timetuple()
        self.assertFalse(dtime == 1970)
        self.assertTrue(dtime != dbytes)
        self.assertFalse(dtime == bytearray(dbytes))
        self.assertTrue(dtime != dtuple)
        with self.assertRaises(TypeError):
            dtime < float(1970)
        with self.assertRaises(TypeError):
            dtime > dbytes
        with self.assertRaises(TypeError):
            dtime <= bytearray(dbytes)
        with self.assertRaises(TypeError):
            dtime >= dtuple

        self.assertTrue(dtime == ALWAYS_EQ)
        self.assertFalse(dtime != ALWAYS_EQ)
        self.assertTrue(dtime < LARGEST)
        self.assertFalse(dtime > LARGEST)
        self.assertTrue(dtime <= LARGEST)
        self.assertFalse(dtime >= LARGEST)
        self.assertFalse(dtime < SMALLEST)
        self.assertTrue(dtime > SMALLEST)
        self.assertFalse(dtime <= SMALLEST)
        self.assertTrue(dtime >= SMALLEST)


class BinaryTestCase(unittest.TestCase):

    # XXX What should str(Binary(b"\xff")) return?  I'm choosing "\xff"
    # for now (i.e. interpreting the binary data as Latin-1-encoded
    # text).  But this feels very unsatisfactory.  Perhaps we should
    # only define repr(), and return r"Binary(b'\xff')" instead?

    def test_default(self):
        t = xmlrpclib.Binary()
        self.assertEqual(str(t), '')

    def test_string(self):
        d = b'\x01\x02\x03abc123\xff\xfe'
        t = xmlrpclib.Binary(d)
        self.assertEqual(str(t), str(d, "latin-1"))

    def test_decode(self):
        d = b'\x01\x02\x03abc123\xff\xfe'
        de = base64.encodebytes(d)
        t1 = xmlrpclib.Binary()
        t1.decode(de)
        self.assertEqual(str(t1), str(d, "latin-1"))

        t2 = xmlrpclib._binary(de)
        self.assertEqual(str(t2), str(d, "latin-1"))


ADDR = PORT = URL = None

# The evt is set twice.  First when the server is ready to serve.
# Second when the server has been shutdown.  The user must clear
# the event after it has been set the first time to catch the second set.
def http_server(evt, numrequests, requestHandler=None, encoding=None):
    class TestInstanceClass:
        def div(self, x, y):
            return x // y

        def _methodHelp(self, name):
            if name == 'div':
                return 'This is the div function'

        class Fixture:
            @staticmethod
            def getData():
                return '42'

    class MyXMLRPCServer(xmlrpc.server.SimpleXMLRPCServer):
        def get_request(self):
            # Ensure the socket is always non-blocking.  On Linux, socket
            # attributes are not inherited like they are on *BSD and Windows.
            s, port = self.socket.accept()
            s.setblocking(True)
            return s, port

    if not requestHandler:
        requestHandler = xmlrpc.server.SimpleXMLRPCRequestHandler
    serv = MyXMLRPCServer(("localhost", 0), requestHandler,
                          encoding=encoding,
                          logRequests=False, bind_and_activate=False)
    try:
        serv.server_bind()
        global ADDR, PORT, URL
        ADDR, PORT = serv.socket.getsockname()
        #connect to IP address directly.  This avoids socket.create_connection()
        #trying to connect to "localhost" using all address families, which
        #causes slowdown e.g. on vista which supports AF_INET6.  The server listens
        #on AF_INET only.
        URL = "http://%s:%d"%(ADDR, PORT)
        serv.server_activate()
        serv.register_introspection_functions()
        serv.register_multicall_functions()
        serv.register_function(pow)
        serv.register_function(lambda x: x, 'têšt')
        @serv.register_function
        def my_function():
            '''This is my function'''
            return True
        @serv.register_function(name='add')
        def _(x, y):
            return x + y
        testInstance = TestInstanceClass()
        serv.register_instance(testInstance, allow_dotted_names=True)
        evt.set()

        # handle up to 'numrequests' requests
        while numrequests > 0:
            serv.handle_request()
            numrequests -= 1

    except TimeoutError:
        pass
    finally:
        serv.socket.close()
        PORT = None
        evt.set()

def http_multi_server(evt, numrequests, requestHandler=None):
    class TestInstanceClass:
        def div(self, x, y):
            return x // y

        def _methodHelp(self, name):
            if name == 'div':
                return 'This is the div function'

    def my_function():
        '''This is my function'''
        return True

    class MyXMLRPCServer(xmlrpc.server.MultiPathXMLRPCServer):
        def get_request(self):
            # Ensure the socket is always non-blocking.  On Linux, socket
            # attributes are not inherited like they are on *BSD and Windows.
            s, port = self.socket.accept()
            s.setblocking(True)
            return s, port

    if not requestHandler:
        requestHandler = xmlrpc.server.SimpleXMLRPCRequestHandler
    class MyRequestHandler(requestHandler):
        rpc_paths = []

    class BrokenDispatcher:
        def _marshaled_dispatch(self, data, dispatch_method=None, path=None):
            raise RuntimeError("broken dispatcher")

    serv = MyXMLRPCServer(("localhost", 0), MyRequestHandler,
                          logRequests=False, bind_and_activate=False)
    serv.socket.settimeout(3)
    serv.server_bind()
    try:
        global ADDR, PORT, URL
        ADDR, PORT = serv.socket.getsockname()
        #connect to IP address directly.  This avoids socket.create_connection()
        #trying to connect to "localhost" using all address families, which
        #causes slowdown e.g. on vista which supports AF_INET6.  The server listens
        #on AF_INET only.
        URL = "http://%s:%d"%(ADDR, PORT)
        serv.server_activate()
        paths = [
            "/foo", "/foo/bar",
            "/foo?k=v", "/foo#frag", "/foo?k=v#frag",
            "", "/", "/RPC2", "?k=v", "#frag",
        ]
        for path in paths:
            d = serv.add_dispatcher(path, xmlrpc.server.SimpleXMLRPCDispatcher())
            d.register_introspection_functions()
            d.register_multicall_functions()
            d.register_function(lambda p=path: p, 'test')
        serv.get_dispatcher(paths[0]).register_function(pow)
        serv.get_dispatcher(paths[1]).register_function(lambda x,y: x+y, 'add')
        serv.add_dispatcher("/is/broken", BrokenDispatcher())
        evt.set()

        # handle up to 'numrequests' requests
        while numrequests > 0:
            serv.handle_request()
            numrequests -= 1

    except TimeoutError:
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
        # Ignore OSErrors here.
        exc_mess = str(e)

    if exc_mess and 'temporarily unavailable' in exc_mess.lower():
        return True

def make_request_and_skipIf(condition, reason):
    # If we skip the test, we have to make a request because
    # the server created in setUp blocks expecting one to come in.
    if not condition:
        return lambda func: func
    def decorator(func):
        def make_request_and_skip(self):
            try:
                xmlrpclib.ServerProxy(URL).my_function()
            except (xmlrpclib.ProtocolError, OSError) as e:
                if not is_unavailable_exception(e):
                    raise
            raise unittest.SkipTest(reason)
        return make_request_and_skip
    return decorator

class BaseServerTestCase(unittest.TestCase):
    requestHandler = None
    request_count = 1
    threadFunc = staticmethod(http_server)

    def setUp(self):
        # enable traceback reporting
        xmlrpc.server.SimpleXMLRPCServer._send_traceback_header = True

        self.evt = threading.Event()
        # start server thread to handle requests
        serv_args = (self.evt, self.request_count, self.requestHandler)
        thread = threading.Thread(target=self.threadFunc, args=serv_args)
        thread.start()
        self.addCleanup(thread.join)

        # wait for the server to be ready
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        # wait on the server thread to terminate
        self.evt.wait()

        # disable traceback reporting
        xmlrpc.server.SimpleXMLRPCServer._send_traceback_header = False

class SimpleServerTestCase(BaseServerTestCase):
    def test_simple1(self):
        try:
            p = xmlrpclib.ServerProxy(URL)
            self.assertEqual(p.pow(6,8), 6**8)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_nonascii(self):
        start_string = 'P\N{LATIN SMALL LETTER Y WITH CIRCUMFLEX}t'
        end_string = 'h\N{LATIN SMALL LETTER O WITH HORN}n'
        try:
            p = xmlrpclib.ServerProxy(URL)
            self.assertEqual(p.add(start_string, end_string),
                             start_string + end_string)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_client_encoding(self):
        start_string = '\u20ac'
        end_string = '\xa4'

        try:
            p = xmlrpclib.ServerProxy(URL, encoding='iso-8859-15')
            self.assertEqual(p.add(start_string, end_string),
                             start_string + end_string)
        except (xmlrpclib.ProtocolError, socket.error) as e:
            # ignore failures due to non-blocking socket unavailable errors.
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_nonascii_methodname(self):
        try:
            p = xmlrpclib.ServerProxy(URL, encoding='ascii')
            self.assertEqual(p.têšt(42), 42)
        except (xmlrpclib.ProtocolError, socket.error) as e:
            # ignore failures due to non-blocking socket unavailable errors.
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_404(self):
        # send POST with http.client, it should return 404 header and
        # 'Not Found' message.
        with contextlib.closing(http.client.HTTPConnection(ADDR, PORT)) as conn:
            conn.request('POST', '/this-is-not-valid')
            response = conn.getresponse()

        self.assertEqual(response.status, 404)
        self.assertEqual(response.reason, 'Not Found')

    def test_introspection1(self):
        expected_methods = set(['pow', 'div', 'my_function', 'add', 'têšt',
                                'system.listMethods', 'system.methodHelp',
                                'system.methodSignature', 'system.multicall',
                                'Fixture'])
        try:
            p = xmlrpclib.ServerProxy(URL)
            meth = p.system.listMethods()
            self.assertEqual(set(meth), expected_methods)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))


    def test_introspection2(self):
        try:
            # test _methodHelp()
            p = xmlrpclib.ServerProxy(URL)
            divhelp = p.system.methodHelp('div')
            self.assertEqual(divhelp, 'This is the div function')
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    @make_request_and_skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_introspection3(self):
        try:
            # test native doc
            p = xmlrpclib.ServerProxy(URL)
            myfunction = p.system.methodHelp('my_function')
            self.assertEqual(myfunction, 'This is my function')
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_introspection4(self):
        # the SimpleXMLRPCServer doesn't support signatures, but
        # at least check that we can try making the call
        try:
            p = xmlrpclib.ServerProxy(URL)
            divsig = p.system.methodSignature('div')
            self.assertEqual(divsig, 'signatures not supported')
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_multicall(self):
        try:
            p = xmlrpclib.ServerProxy(URL)
            multicall = xmlrpclib.MultiCall(p)
            multicall.add(2,3)
            multicall.pow(6,8)
            multicall.div(127,42)
            add_result, pow_result, div_result = multicall()
            self.assertEqual(add_result, 2+3)
            self.assertEqual(pow_result, 6**8)
            self.assertEqual(div_result, 127//42)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_non_existing_multicall(self):
        try:
            p = xmlrpclib.ServerProxy(URL)
            multicall = xmlrpclib.MultiCall(p)
            multicall.this_is_not_exists()
            result = multicall()

            # result.results contains;
            # [{'faultCode': 1, 'faultString': '<class \'exceptions.Exception\'>:'
            #   'method "this_is_not_exists" is not supported'>}]

            self.assertEqual(result.results[0]['faultCode'], 1)
            self.assertEqual(result.results[0]['faultString'],
                '<class \'Exception\'>:method "this_is_not_exists" '
                'is not supported')
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_dotted_attribute(self):
        # Raises an AttributeError because private methods are not allowed.
        self.assertRaises(AttributeError,
                          xmlrpc.server.resolve_dotted_attribute, str, '__add')

        self.assertTrue(xmlrpc.server.resolve_dotted_attribute(str, 'title'))
        # Get the test to run faster by sending a request with test_simple1.
        # This avoids waiting for the socket timeout.
        self.test_simple1()

    def test_allow_dotted_names_true(self):
        # XXX also need allow_dotted_names_false test.
        server = xmlrpclib.ServerProxy("http://%s:%d/RPC2" % (ADDR, PORT))
        data = server.Fixture.getData()
        self.assertEqual(data, '42')

    def test_unicode_host(self):
        server = xmlrpclib.ServerProxy("http://%s:%d/RPC2" % (ADDR, PORT))
        self.assertEqual(server.add("a", "\xe9"), "a\xe9")

    def test_partial_post(self):
        # Check that a partial POST doesn't make the server loop: issue #14001.
        with contextlib.closing(socket.create_connection((ADDR, PORT))) as conn:
            conn.send('POST /RPC2 HTTP/1.0\r\n'
                      'Content-Length: 100\r\n\r\n'
                      'bye HTTP/1.1\r\n'
                      f'Host: {ADDR}:{PORT}\r\n'
                      'Accept-Encoding: identity\r\n'
                      'Content-Length: 0\r\n\r\n'.encode('ascii'))

    def test_context_manager(self):
        with xmlrpclib.ServerProxy(URL) as server:
            server.add(2, 3)
            self.assertNotEqual(server('transport')._connection,
                                (None, None))
        self.assertEqual(server('transport')._connection,
                         (None, None))

    def test_context_manager_method_error(self):
        try:
            with xmlrpclib.ServerProxy(URL) as server:
                server.add(2, "a")
        except xmlrpclib.Fault:
            pass
        self.assertEqual(server('transport')._connection,
                         (None, None))


class SimpleServerEncodingTestCase(BaseServerTestCase):
    @staticmethod
    def threadFunc(evt, numrequests, requestHandler=None, encoding=None):
        http_server(evt, numrequests, requestHandler, 'iso-8859-15')

    def test_server_encoding(self):
        start_string = '\u20ac'
        end_string = '\xa4'

        try:
            p = xmlrpclib.ServerProxy(URL)
            self.assertEqual(p.add(start_string, end_string),
                             start_string + end_string)
        except (xmlrpclib.ProtocolError, socket.error) as e:
            # ignore failures due to non-blocking socket unavailable errors.
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))


class MultiPathServerTestCase(BaseServerTestCase):
    threadFunc = staticmethod(http_multi_server)
    request_count = 2
    def test_path1(self):
        p = xmlrpclib.ServerProxy(URL+"/foo")
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertRaises(xmlrpclib.Fault, p.add, 6, 8)

    def test_path2(self):
        p = xmlrpclib.ServerProxy(URL+"/foo/bar")
        self.assertEqual(p.add(6,8), 6+8)
        self.assertRaises(xmlrpclib.Fault, p.pow, 6, 8)

    @support.requires_resource('walltime')
    def test_path3(self):
        p = xmlrpclib.ServerProxy(URL+"/is/broken")
        self.assertRaises(xmlrpclib.Fault, p.add, 6, 8)

    @support.requires_resource('walltime')
    def test_invalid_path(self):
        p = xmlrpclib.ServerProxy(URL+"/invalid")
        self.assertRaises(xmlrpclib.Fault, p.add, 6, 8)

    @support.requires_resource('walltime')
    def test_path_query_fragment(self):
        p = xmlrpclib.ServerProxy(URL+"/foo?k=v#frag")
        self.assertEqual(p.test(), "/foo?k=v#frag")

    @support.requires_resource('walltime')
    def test_path_fragment(self):
        p = xmlrpclib.ServerProxy(URL+"/foo#frag")
        self.assertEqual(p.test(), "/foo#frag")

    @support.requires_resource('walltime')
    def test_path_query(self):
        p = xmlrpclib.ServerProxy(URL+"/foo?k=v")
        self.assertEqual(p.test(), "/foo?k=v")

    @support.requires_resource('walltime')
    def test_empty_path(self):
        p = xmlrpclib.ServerProxy(URL)
        self.assertEqual(p.test(), "/RPC2")

    @support.requires_resource('walltime')
    def test_root_path(self):
        p = xmlrpclib.ServerProxy(URL + "/")
        self.assertEqual(p.test(), "/")

    @support.requires_resource('walltime')
    def test_empty_path_query(self):
        p = xmlrpclib.ServerProxy(URL + "?k=v")
        self.assertEqual(p.test(), "?k=v")

    @support.requires_resource('walltime')
    def test_empty_path_fragment(self):
        p = xmlrpclib.ServerProxy(URL + "#frag")
        self.assertEqual(p.test(), "#frag")


#A test case that verifies that a server using the HTTP/1.1 keep-alive mechanism
#does indeed serve subsequent requests on the same connection
class BaseKeepaliveServerTestCase(BaseServerTestCase):
    #a request handler that supports keep-alive and logs requests into a
    #class variable
    class RequestHandler(xmlrpc.server.SimpleXMLRPCRequestHandler):
        parentClass = xmlrpc.server.SimpleXMLRPCRequestHandler
        protocol_version = 'HTTP/1.1'
        myRequests = []
        def handle(self):
            self.myRequests.append([])
            self.reqidx = len(self.myRequests)-1
            return self.parentClass.handle(self)
        def handle_one_request(self):
            result = self.parentClass.handle_one_request(self)
            self.myRequests[self.reqidx].append(self.raw_requestline)
            return result

    requestHandler = RequestHandler
    def setUp(self):
        #clear request log
        self.RequestHandler.myRequests = []
        return BaseServerTestCase.setUp(self)

#A test case that verifies that a server using the HTTP/1.1 keep-alive mechanism
#does indeed serve subsequent requests on the same connection
class KeepaliveServerTestCase1(BaseKeepaliveServerTestCase):
    def test_two(self):
        p = xmlrpclib.ServerProxy(URL)
        #do three requests.
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertEqual(p.pow(6,8), 6**8)
        p("close")()

        #they should have all been handled by a single request handler
        self.assertEqual(len(self.RequestHandler.myRequests), 1)

        #check that we did at least two (the third may be pending append
        #due to thread scheduling)
        self.assertGreaterEqual(len(self.RequestHandler.myRequests[-1]), 2)


#test special attribute access on the serverproxy, through the __call__
#function.
class KeepaliveServerTestCase2(BaseKeepaliveServerTestCase):
    #ask for two keepalive requests to be handled.
    request_count=2

    def test_close(self):
        p = xmlrpclib.ServerProxy(URL)
        #do some requests with close.
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertEqual(p.pow(6,8), 6**8)
        p("close")() #this should trigger a new keep-alive request
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertEqual(p.pow(6,8), 6**8)
        self.assertEqual(p.pow(6,8), 6**8)
        p("close")()

        #they should have all been two request handlers, each having logged at least
        #two complete requests
        self.assertEqual(len(self.RequestHandler.myRequests), 2)
        self.assertGreaterEqual(len(self.RequestHandler.myRequests[-1]), 2)
        self.assertGreaterEqual(len(self.RequestHandler.myRequests[-2]), 2)


    def test_transport(self):
        p = xmlrpclib.ServerProxy(URL)
        #do some requests with close.
        self.assertEqual(p.pow(6,8), 6**8)
        p("transport").close() #same as above, really.
        self.assertEqual(p.pow(6,8), 6**8)
        p("close")()
        self.assertEqual(len(self.RequestHandler.myRequests), 2)

#A test case that verifies that gzip encoding works in both directions
#(for a request and the response)
@unittest.skipIf(gzip is None, 'requires gzip')
class GzipServerTestCase(BaseServerTestCase):
    #a request handler that supports keep-alive and logs requests into a
    #class variable
    class RequestHandler(xmlrpc.server.SimpleXMLRPCRequestHandler):
        parentClass = xmlrpc.server.SimpleXMLRPCRequestHandler
        protocol_version = 'HTTP/1.1'

        def do_POST(self):
            #store content of last request in class
            self.__class__.content_length = int(self.headers["content-length"])
            return self.parentClass.do_POST(self)
    requestHandler = RequestHandler

    class Transport(xmlrpclib.Transport):
        #custom transport, stores the response length for our perusal
        fake_gzip = False
        def parse_response(self, response):
            self.response_length=int(response.getheader("content-length", 0))
            return xmlrpclib.Transport.parse_response(self, response)

        def send_content(self, connection, body):
            if self.fake_gzip:
                #add a lone gzip header to induce decode error remotely
                connection.putheader("Content-Encoding", "gzip")
            return xmlrpclib.Transport.send_content(self, connection, body)

    def setUp(self):
        BaseServerTestCase.setUp(self)

    def test_gzip_request(self):
        t = self.Transport()
        t.encode_threshold = None
        p = xmlrpclib.ServerProxy(URL, transport=t)
        self.assertEqual(p.pow(6,8), 6**8)
        a = self.RequestHandler.content_length
        t.encode_threshold = 0 #turn on request encoding
        self.assertEqual(p.pow(6,8), 6**8)
        b = self.RequestHandler.content_length
        self.assertTrue(a>b)
        p("close")()

    def test_bad_gzip_request(self):
        t = self.Transport()
        t.encode_threshold = None
        t.fake_gzip = True
        p = xmlrpclib.ServerProxy(URL, transport=t)
        cm = self.assertRaisesRegex(xmlrpclib.ProtocolError,
                                    re.compile(r"\b400\b"))
        with cm:
            p.pow(6, 8)
        p("close")()

    def test_gzip_response(self):
        t = self.Transport()
        p = xmlrpclib.ServerProxy(URL, transport=t)
        old = self.requestHandler.encode_threshold
        self.requestHandler.encode_threshold = None #no encoding
        self.assertEqual(p.pow(6,8), 6**8)
        a = t.response_length
        self.requestHandler.encode_threshold = 0 #always encode
        self.assertEqual(p.pow(6,8), 6**8)
        p("close")()
        b = t.response_length
        self.requestHandler.encode_threshold = old
        self.assertTrue(a>b)


@unittest.skipIf(gzip is None, 'requires gzip')
class GzipUtilTestCase(unittest.TestCase):

    def test_gzip_decode_limit(self):
        max_gzip_decode = 20 * 1024 * 1024
        data = b'\0' * max_gzip_decode
        encoded = xmlrpclib.gzip_encode(data)
        decoded = xmlrpclib.gzip_decode(encoded)
        self.assertEqual(len(decoded), max_gzip_decode)

        data = b'\0' * (max_gzip_decode + 1)
        encoded = xmlrpclib.gzip_encode(data)

        with self.assertRaisesRegex(ValueError,
                                    "max gzipped payload length exceeded"):
            xmlrpclib.gzip_decode(encoded)

        xmlrpclib.gzip_decode(encoded, max_decode=-1)


class HeadersServerTestCase(BaseServerTestCase):
    class RequestHandler(xmlrpc.server.SimpleXMLRPCRequestHandler):
        test_headers = None

        def do_POST(self):
            self.__class__.test_headers = self.headers
            return super().do_POST()
    requestHandler = RequestHandler
    standard_headers = [
        'Host', 'Accept-Encoding', 'Content-Type', 'User-Agent',
        'Content-Length']

    def setUp(self):
        self.RequestHandler.test_headers = None
        return super().setUp()

    def assertContainsAdditionalHeaders(self, headers, additional):
        expected_keys = sorted(self.standard_headers + list(additional.keys()))
        self.assertListEqual(sorted(headers.keys()), expected_keys)

        for key, value in additional.items():
            self.assertEqual(headers.get(key), value)

    def test_header(self):
        p = xmlrpclib.ServerProxy(URL, headers=[('X-Test', 'foo')])
        self.assertEqual(p.pow(6, 8), 6**8)

        headers = self.RequestHandler.test_headers
        self.assertContainsAdditionalHeaders(headers, {'X-Test': 'foo'})

    def test_header_many(self):
        p = xmlrpclib.ServerProxy(
            URL, headers=[('X-Test', 'foo'), ('X-Test-Second', 'bar')])
        self.assertEqual(p.pow(6, 8), 6**8)

        headers = self.RequestHandler.test_headers
        self.assertContainsAdditionalHeaders(
            headers, {'X-Test': 'foo', 'X-Test-Second': 'bar'})

    def test_header_empty(self):
        p = xmlrpclib.ServerProxy(URL, headers=[])
        self.assertEqual(p.pow(6, 8), 6**8)

        headers = self.RequestHandler.test_headers
        self.assertContainsAdditionalHeaders(headers, {})

    def test_header_tuple(self):
        p = xmlrpclib.ServerProxy(URL, headers=(('X-Test', 'foo'),))
        self.assertEqual(p.pow(6, 8), 6**8)

        headers = self.RequestHandler.test_headers
        self.assertContainsAdditionalHeaders(headers, {'X-Test': 'foo'})

    def test_header_items(self):
        p = xmlrpclib.ServerProxy(URL, headers={'X-Test': 'foo'}.items())
        self.assertEqual(p.pow(6, 8), 6**8)

        headers = self.RequestHandler.test_headers
        self.assertContainsAdditionalHeaders(headers, {'X-Test': 'foo'})


#Test special attributes of the ServerProxy object
class ServerProxyTestCase(unittest.TestCase):
    def setUp(self):
        unittest.TestCase.setUp(self)
        # Actual value of the URL doesn't matter if it is a string in
        # the correct format.
        self.url = 'http://fake.localhost'

    def test_close(self):
        p = xmlrpclib.ServerProxy(self.url)
        self.assertEqual(p('close')(), None)

    def test_transport(self):
        t = xmlrpclib.Transport()
        p = xmlrpclib.ServerProxy(self.url, transport=t)
        self.assertEqual(p('transport'), t)


# This is a contrived way to make a failure occur on the server side
# in order to test the _send_traceback_header flag on the server
class FailingMessageClass(http.client.HTTPMessage):
    def get(self, key, failobj=None):
        key = key.lower()
        if key == 'content-length':
            return 'I am broken'
        return super().get(key, failobj)


class FailingServerTestCase(unittest.TestCase):
    def setUp(self):
        self.evt = threading.Event()
        # start server thread to handle requests
        serv_args = (self.evt, 1)
        thread = threading.Thread(target=http_server, args=serv_args)
        thread.start()
        self.addCleanup(thread.join)

        # wait for the server to be ready
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        # wait on the server thread to terminate
        self.evt.wait()
        # reset flag
        xmlrpc.server.SimpleXMLRPCServer._send_traceback_header = False
        # reset message class
        default_class = http.client.HTTPMessage
        xmlrpc.server.SimpleXMLRPCRequestHandler.MessageClass = default_class

    def test_basic(self):
        # check that flag is false by default
        flagval = xmlrpc.server.SimpleXMLRPCServer._send_traceback_header
        self.assertEqual(flagval, False)

        # enable traceback reporting
        xmlrpc.server.SimpleXMLRPCServer._send_traceback_header = True

        # test a call that shouldn't fail just as a smoke test
        try:
            p = xmlrpclib.ServerProxy(URL)
            self.assertEqual(p.pow(6,8), 6**8)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e):
                # protocol error; provide additional information in test output
                self.fail("%s\n%s" % (e, getattr(e, "headers", "")))

    def test_fail_no_info(self):
        # use the broken message class
        xmlrpc.server.SimpleXMLRPCRequestHandler.MessageClass = FailingMessageClass

        try:
            p = xmlrpclib.ServerProxy(URL)
            p.pow(6,8)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e) and hasattr(e, "headers"):
                # The two server-side error headers shouldn't be sent back in this case
                self.assertTrue(e.headers.get("X-exception") is None)
                self.assertTrue(e.headers.get("X-traceback") is None)
        else:
            self.fail('ProtocolError not raised')

    def test_fail_with_info(self):
        # use the broken message class
        xmlrpc.server.SimpleXMLRPCRequestHandler.MessageClass = FailingMessageClass

        # Check that errors in the server send back exception/traceback
        # info when flag is set
        xmlrpc.server.SimpleXMLRPCServer._send_traceback_header = True

        try:
            p = xmlrpclib.ServerProxy(URL)
            p.pow(6,8)
        except (xmlrpclib.ProtocolError, OSError) as e:
            # ignore failures due to non-blocking socket 'unavailable' errors
            if not is_unavailable_exception(e) and hasattr(e, "headers"):
                # We should get error info in the response
                expected_err = "invalid literal for int() with base 10: 'I am broken'"
                self.assertEqual(e.headers.get("X-exception"), expected_err)
                self.assertTrue(e.headers.get("X-traceback") is not None)
        else:
            self.fail('ProtocolError not raised')


@contextlib.contextmanager
def captured_stdout(encoding='utf-8'):
    """A variation on support.captured_stdout() which gives a text stream
    having a `buffer` attribute.
    """
    orig_stdout = sys.stdout
    sys.stdout = io.TextIOWrapper(io.BytesIO(), encoding=encoding)
    try:
        yield sys.stdout
    finally:
        sys.stdout = orig_stdout


class CGIHandlerTestCase(unittest.TestCase):
    def setUp(self):
        self.cgi = xmlrpc.server.CGIXMLRPCRequestHandler()

    def tearDown(self):
        self.cgi = None

    def test_cgi_get(self):
        with os_helper.EnvironmentVarGuard() as env:
            env['REQUEST_METHOD'] = 'GET'
            # if the method is GET and no request_text is given, it runs handle_get
            # get sysout output
            with captured_stdout(encoding=self.cgi.encoding) as data_out:
                self.cgi.handle_request()

            # parse Status header
            data_out.seek(0)
            handle = data_out.read()
            status = handle.split()[1]
            message = ' '.join(handle.split()[2:4])

            self.assertEqual(status, '400')
            self.assertEqual(message, 'Bad Request')


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

        with os_helper.EnvironmentVarGuard() as env, \
             captured_stdout(encoding=self.cgi.encoding) as data_out, \
             support.captured_stdin() as data_in:
            data_in.write(data)
            data_in.seek(0)
            env['CONTENT_LENGTH'] = str(len(data))
            self.cgi.handle_request()
        data_out.seek(0)

        # will respond exception, if so, our goal is achieved ;)
        handle = data_out.read()

        # start with 44th char so as not to get http header, we just
        # need only xml
        self.assertRaises(xmlrpclib.Fault, xmlrpclib.loads, handle[44:])

        # Also test the content-length returned  by handle_request
        # Using the same test method inorder to avoid all the datapassing
        # boilerplate code.
        # Test for bug: http://bugs.python.org/issue5040

        content = handle[handle.find("<?xml"):]

        self.assertEqual(
            int(re.search(r'Content-Length: (\d+)', handle).group(1)),
            len(content))


class UseBuiltinTypesTestCase(unittest.TestCase):

    def test_use_builtin_types(self):
        # SimpleXMLRPCDispatcher.__init__ accepts use_builtin_types, which
        # makes all dispatch of binary data as bytes instances, and all
        # dispatch of datetime argument as datetime.datetime instances.
        self.log = []
        expected_bytes = b"my dog has fleas"
        expected_date = datetime.datetime(2008, 5, 26, 18, 25, 12)
        marshaled = xmlrpclib.dumps((expected_bytes, expected_date), 'foobar')
        def foobar(*args):
            self.log.extend(args)
        handler = xmlrpc.server.SimpleXMLRPCDispatcher(
            allow_none=True, encoding=None, use_builtin_types=True)
        handler.register_function(foobar)
        handler._marshaled_dispatch(marshaled)
        self.assertEqual(len(self.log), 2)
        mybytes, mydate = self.log
        self.assertEqual(self.log, [expected_bytes, expected_date])
        self.assertIs(type(mydate), datetime.datetime)
        self.assertIs(type(mybytes), bytes)

    def test_cgihandler_has_use_builtin_types_flag(self):
        handler = xmlrpc.server.CGIXMLRPCRequestHandler(use_builtin_types=True)
        self.assertTrue(handler.use_builtin_types)

    def test_xmlrpcserver_has_use_builtin_types_flag(self):
        server = xmlrpc.server.SimpleXMLRPCServer(("localhost", 0),
            use_builtin_types=True)
        server.server_close()
        self.assertTrue(server.use_builtin_types)


def setUpModule():
    thread_info = threading_helper.threading_setup()
    unittest.addModuleCleanup(threading_helper.threading_cleanup, *thread_info)


if __name__ == "__main__":
    unittest.main()
