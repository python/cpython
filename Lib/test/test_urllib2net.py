#!/usr/bin/env python

import unittest
from test import test_support
from test.test_urllib2 import sanepathname2url

import socket
import urllib2
import sys
import os
import mimetools


def _urlopen_with_retry(host, *args, **kwargs):
    # Connecting to remote hosts is flaky.  Make it more robust
    # by retrying the connection several times.
    for i in range(3):
        try:
            return urllib2.urlopen(host, *args, **kwargs)
        except urllib2.URLError, last_exc:
            continue
        except:
            raise
    raise last_exc



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
        import socket, httplib, gc

        # calling .close() on urllib2's response objects should close the
        # underlying socket

        # delve deep into response to fetch socket._socketobject
        response = _urlopen_with_retry("http://www.python.org/")
        abused_fileobject = response.fp
        self.assert_(abused_fileobject.__class__ is socket._fileobject)
        httpresponse = abused_fileobject._sock
        self.assert_(httpresponse.__class__ is httplib.HTTPResponse)
        fileobject = httpresponse.fp
        self.assert_(fileobject.__class__ is socket._fileobject)

        self.assert_(not fileobject.closed)
        response.close()
        self.assert_(fileobject.closed)

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
            'ftp://ftp.kernel.org/pub/linux/kernel/non-existant-file',
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
            self._test_urls(urls, self._extra_handlers(), urllib2.urlopen)
        finally:
            os.remove(TESTFN)

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

    def _test_urls(self, urls, handlers, urlopen=_urlopen_with_retry):
        import socket
        import time
        import logging
        debug = logging.getLogger("test_urllib2").debug

        urllib2.install_opener(urllib2.build_opener(*handlers))

        for url in urls:
            if isinstance(url, tuple):
                url, req, expected_err = url
            else:
                req = expected_err = None
            debug(url)
            try:
                f = urlopen(url, req)
            except EnvironmentError, err:
                debug(err)
                if expected_err:
                    msg = ("Didn't get expected error(s) %s for %s %s, got %s: %s" %
                           (expected_err, url, req, type(err), err))
                    self.assert_(isinstance(err, expected_err), msg)
            else:
                with test_support.transient_internet():
                    buf = f.read()
                f.close()
                debug("read %d bytes" % len(buf))
            debug("******** next url coming up...")
            time.sleep(0.1)

    def _extra_handlers(self):
        handlers = []

        cfh = urllib2.CacheFTPHandler()
        cfh.setTimeout(1)
        handlers.append(cfh)

        return handlers


class TimeoutTest(unittest.TestCase):
    def test_http_basic(self):
        u = _urlopen_with_retry("http://www.python.org")
        self.assertTrue(u.fp._sock.fp._sock.gettimeout() is None)

    def test_http_NoneWithdefault(self):
        prev = socket.getdefaulttimeout()
        socket.setdefaulttimeout(60)
        try:
            u = _urlopen_with_retry("http://www.python.org", timeout=None)
            self.assertEqual(u.fp._sock.fp._sock.gettimeout(), 60)
        finally:
            socket.setdefaulttimeout(prev)

    def test_http_Value(self):
        u = _urlopen_with_retry("http://www.python.org", timeout=120)
        self.assertEqual(u.fp._sock.fp._sock.gettimeout(), 120)

    def test_http_NoneNodefault(self):
        u = _urlopen_with_retry("http://www.python.org", timeout=None)
        self.assertTrue(u.fp._sock.fp._sock.gettimeout() is None)

    FTP_HOST = "ftp://ftp.mirror.nl/pub/mirror/gnu/"

    def test_ftp_basic(self):
        u = _urlopen_with_retry(self.FTP_HOST)
        self.assertTrue(u.fp.fp._sock.gettimeout() is None)

    def test_ftp_NoneWithdefault(self):
        prev = socket.getdefaulttimeout()
        socket.setdefaulttimeout(60)
        try:
            u = _urlopen_with_retry(self.FTP_HOST, timeout=None)
            self.assertEqual(u.fp.fp._sock.gettimeout(), 60)
        finally:
            socket.setdefaulttimeout(prev)

    def test_ftp_NoneNodefault(self):
        u = _urlopen_with_retry(self.FTP_HOST, timeout=None)
        self.assertTrue(u.fp.fp._sock.gettimeout() is None)

    def test_ftp_Value(self):
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
