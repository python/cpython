#!/usr/bin/env python

import unittest
from test import test_support
from test.test_urllib2 import sanepathname2url

import socket
import urllib2
import os
import sys

TIMEOUT = 60  # seconds


def _retry_thrice(func, exc, *args, **kwargs):
    for i in range(3):
        try:
            return func(*args, **kwargs)
        except exc, last_exc:
            continue
        except:
            raise
    raise last_exc

def _wrap_with_retry_thrice(func, exc):
    def wrapped(*args, **kwargs):
        return _retry_thrice(func, exc, *args, **kwargs)
    return wrapped

# Connecting to remote hosts is flaky.  Make it more robust by retrying
# the connection several times.
_urlopen_with_retry = _wrap_with_retry_thrice(urllib2.urlopen, urllib2.URLError)


class AuthTests(unittest.TestCase):
    """Tests urllib2 authentication features."""

## Disabled at the moment since there is no page under python.org which
## could be used to HTTP authentication.
#
#    def test_basic_auth(self):
#        import httplib
#
#        test_url = "http://www.python.org/test/test_urllib2/basic_auth"
#        test_hostport = "www.python.org"
#        test_realm = 'Test Realm'
#        test_user = 'test.test_urllib2net'
#        test_password = 'blah'
#
#        # failure
#        try:
#            _urlopen_with_retry(test_url)
#        except urllib2.HTTPError, exc:
#            self.assertEqual(exc.code, 401)
#        else:
#            self.fail("urlopen() should have failed with 401")
#
#        # success
#        auth_handler = urllib2.HTTPBasicAuthHandler()
#        auth_handler.add_password(test_realm, test_hostport,
#                                  test_user, test_password)
#        opener = urllib2.build_opener(auth_handler)
#        f = opener.open('http://localhost/')
#        response = _urlopen_with_retry("http://www.python.org/")
#
#        # The 'userinfo' URL component is deprecated by RFC 3986 for security
#        # reasons, let's not implement it!  (it's already implemented for proxy
#        # specification strings (that is, URLs or authorities specifying a
#        # proxy), so we must keep that)
#        self.assertRaises(httplib.InvalidURL,
#                          urllib2.urlopen, "http://evil:thing@example.com")


class CloseSocketTest(unittest.TestCase):

    def test_close(self):
        import httplib

        # calling .close() on urllib2's response objects should close the
        # underlying socket

        # delve deep into response to fetch socket._socketobject
        response = _urlopen_with_retry("http://www.python.org/")
        abused_fileobject = response.fp
        self.assertTrue(abused_fileobject.__class__ is socket._fileobject)
        httpresponse = abused_fileobject._sock
        self.assertTrue(httpresponse.__class__ is httplib.HTTPResponse)
        fileobject = httpresponse.fp
        self.assertTrue(fileobject.__class__ is socket._fileobject)

        self.assertTrue(not fileobject.closed)
        response.close()
        self.assertTrue(fileobject.closed)

class OtherNetworkTests(unittest.TestCase):
    def setUp(self):
        if 0:  # for debugging
            import logging
            logger = logging.getLogger("test_urllib2net")
            logger.addHandler(logging.StreamHandler())

    # XXX The rest of these tests aren't very good -- they don't check much.
    # They do sometimes catch some major disasters, though.

    def test_ftp(self):
        urls = [
            'ftp://ftp.kernel.org/pub/linux/kernel/README',
            'ftp://ftp.kernel.org/pub/linux/kernel/non-existent-file',
            #'ftp://ftp.kernel.org/pub/leenox/kernel/test',
            'ftp://gatekeeper.research.compaq.com/pub/DEC/SRC'
                '/research-reports/00README-Legal-Rules-Regs',
            ]
        self._test_urls(urls, self._extra_handlers())

    def test_file(self):
        TESTFN = test_support.TESTFN
        f = open(TESTFN, 'w')
        try:
            f.write('hi there\n')
            f.close()
            urls = [
                'file:'+sanepathname2url(os.path.abspath(TESTFN)),
                ('file:///nonsensename/etc/passwd', None, urllib2.URLError),
                ]
            self._test_urls(urls, self._extra_handlers(), retry=True)
        finally:
            os.remove(TESTFN)

        self.assertRaises(ValueError, urllib2.urlopen,'./relative_path/to/file')

    # XXX Following test depends on machine configurations that are internal
    # to CNRI.  Need to set up a public server with the right authentication
    # configuration for test purposes.

##     def test_cnri(self):
##         if socket.gethostname() == 'bitdiddle':
##             localhost = 'bitdiddle.cnri.reston.va.us'
##         elif socket.gethostname() == 'bitdiddle.concentric.net':
##             localhost = 'localhost'
##         else:
##             localhost = None
##         if localhost is not None:
##             urls = [
##                 'file://%s/etc/passwd' % localhost,
##                 'http://%s/simple/' % localhost,
##                 'http://%s/digest/' % localhost,
##                 'http://%s/not/found.h' % localhost,
##                 ]

##             bauth = HTTPBasicAuthHandler()
##             bauth.add_password('basic_test_realm', localhost, 'jhylton',
##                                'password')
##             dauth = HTTPDigestAuthHandler()
##             dauth.add_password('digest_test_realm', localhost, 'jhylton',
##                                'password')

##             self._test_urls(urls, self._extra_handlers()+[bauth, dauth])

    def test_urlwithfrag(self):
        urlwith_frag = "http://docs.python.org/2/glossary.html#glossary"
        with test_support.transient_internet(urlwith_frag):
            req = urllib2.Request(urlwith_frag)
            res = urllib2.urlopen(req)
            self.assertEqual(res.geturl(),
                    "http://docs.python.org/2/glossary.html#glossary")

    def test_fileno(self):
        req = urllib2.Request("http://www.python.org")
        opener = urllib2.build_opener()
        res = opener.open(req)
        try:
            res.fileno()
        except AttributeError:
            self.fail("HTTPResponse object should return a valid fileno")
        finally:
            res.close()

    def test_custom_headers(self):
        url = "http://www.example.com"
        with test_support.transient_internet(url):
            opener = urllib2.build_opener()
            request = urllib2.Request(url)
            self.assertFalse(request.header_items())
            opener.open(request)
            self.assertTrue(request.header_items())
            self.assertTrue(request.has_header('User-agent'))
            request.add_header('User-Agent','Test-Agent')
            opener.open(request)
            self.assertEqual(request.get_header('User-agent'),'Test-Agent')

    def test_sites_no_connection_close(self):
        # Some sites do not send Connection: close header.
        # Verify that those work properly. (#issue12576)

        URL = 'http://www.imdb.com' # No Connection:close
        with test_support.transient_internet(URL):
            req = urllib2.urlopen(URL)
            res = req.read()
            self.assertTrue(res)

    def _test_urls(self, urls, handlers, retry=True):
        import time
        import logging
        debug = logging.getLogger("test_urllib2").debug

        urlopen = urllib2.build_opener(*handlers).open
        if retry:
            urlopen = _wrap_with_retry_thrice(urlopen, urllib2.URLError)

        for url in urls:
            if isinstance(url, tuple):
                url, req, expected_err = url
            else:
                req = expected_err = None
            with test_support.transient_internet(url):
                debug(url)
                try:
                    f = urlopen(url, req, TIMEOUT)
                except EnvironmentError as err:
                    debug(err)
                    if expected_err:
                        msg = ("Didn't get expected error(s) %s for %s %s, got %s: %s" %
                               (expected_err, url, req, type(err), err))
                        self.assertIsInstance(err, expected_err, msg)
                except urllib2.URLError as err:
                    if isinstance(err[0], socket.timeout):
                        print >>sys.stderr, "<timeout: %s>" % url
                        continue
                    else:
                        raise
                else:
                    try:
                        with test_support.transient_internet(url):
                            buf = f.read()
                            debug("read %d bytes" % len(buf))
                    except socket.timeout:
                        print >>sys.stderr, "<timeout: %s>" % url
                    f.close()
            debug("******** next url coming up...")
            time.sleep(0.1)

    def _extra_handlers(self):
        handlers = []

        cfh = urllib2.CacheFTPHandler()
        self.addCleanup(cfh.clear_cache)
        cfh.setTimeout(1)
        handlers.append(cfh)

        return handlers


class TimeoutTest(unittest.TestCase):
    def test_http_basic(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        url = "http://www.python.org"
        with test_support.transient_internet(url, timeout=None):
            u = _urlopen_with_retry(url)
            self.assertTrue(u.fp._sock.fp._sock.gettimeout() is None)

    def test_http_default_timeout(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        url = "http://www.python.org"
        with test_support.transient_internet(url):
            socket.setdefaulttimeout(60)
            try:
                u = _urlopen_with_retry(url)
            finally:
                socket.setdefaulttimeout(None)
            self.assertEqual(u.fp._sock.fp._sock.gettimeout(), 60)

    def test_http_no_timeout(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        url = "http://www.python.org"
        with test_support.transient_internet(url):
            socket.setdefaulttimeout(60)
            try:
                u = _urlopen_with_retry(url, timeout=None)
            finally:
                socket.setdefaulttimeout(None)
            self.assertTrue(u.fp._sock.fp._sock.gettimeout() is None)

    def test_http_timeout(self):
        url = "http://www.python.org"
        with test_support.transient_internet(url):
            u = _urlopen_with_retry(url, timeout=120)
            self.assertEqual(u.fp._sock.fp._sock.gettimeout(), 120)

    FTP_HOST = "ftp://ftp.mirror.nl/pub/gnu/"

    def test_ftp_basic(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        with test_support.transient_internet(self.FTP_HOST, timeout=None):
            u = _urlopen_with_retry(self.FTP_HOST)
            self.assertTrue(u.fp.fp._sock.gettimeout() is None)

    def test_ftp_default_timeout(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        with test_support.transient_internet(self.FTP_HOST):
            socket.setdefaulttimeout(60)
            try:
                u = _urlopen_with_retry(self.FTP_HOST)
            finally:
                socket.setdefaulttimeout(None)
            self.assertEqual(u.fp.fp._sock.gettimeout(), 60)

    def test_ftp_no_timeout(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        with test_support.transient_internet(self.FTP_HOST):
            socket.setdefaulttimeout(60)
            try:
                u = _urlopen_with_retry(self.FTP_HOST, timeout=None)
            finally:
                socket.setdefaulttimeout(None)
            self.assertTrue(u.fp.fp._sock.gettimeout() is None)

    def test_ftp_timeout(self):
        with test_support.transient_internet(self.FTP_HOST):
            u = _urlopen_with_retry(self.FTP_HOST, timeout=60)
            self.assertEqual(u.fp.fp._sock.gettimeout(), 60)


def test_main():
    test_support.requires("network")
    test_support.run_unittest(AuthTests,
                              OtherNetworkTests,
                              CloseSocketTest,
                              TimeoutTest,
                              )

if __name__ == "__main__":
    test_main()
