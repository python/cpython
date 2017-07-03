import os
import robotparser
import unittest
from test import support
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import StringIO
try:
    import threading
except ImportError:
    threading = None


class BaseRobotTest:
    robots_txt = ''
    agent = 'test_robotparser'
    good = []
    bad = []

    def setUp(self):
        lines = StringIO.StringIO(self.robots_txt).readlines()
        self.parser = robotparser.RobotFileParser()
        self.parser.parse(lines)

    def get_agent_and_url(self, url):
        if isinstance(url, tuple):
            agent, url = url
            return agent, url
        return self.agent, url

    def test_good_urls(self):
        for url in self.good:
            agent, url = self.get_agent_and_url(url)
            self.assertTrue(self.parser.can_fetch(agent, url))

    def test_bad_urls(self):
        for url in self.bad:
            agent, url = self.get_agent_and_url(url)
            self.assertFalse(self.parser.can_fetch(agent, url))


class UserAgentWildcardTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Disallow: /cyberworld/map/ # This is an infinite virtual URL space
Disallow: /tmp/ # these will soon disappear
Disallow: /foo.html
    """
    good = ['/', '/test.html']
    bad = ['/cyberworld/map/index.html', '/tmp/xxx', '/foo.html']


class RejectAllRobotsTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
# go away
User-agent: *
Disallow: /
    """
    good = []
    bad = ['/cyberworld/map/index.html', '/', '/tmp/']


class UserAgentOrderingTest(BaseRobotTest, unittest.TestCase):
    # the order of User-agent should be correct. note
    # that this file is incorrect because "Googlebot" is a
    # substring of "Googlebot-Mobile"
    robots_txt = """\
User-agent: Googlebot
Disallow: /

User-agent: Googlebot-Mobile
Allow: /
    """
    agent = 'Googlebot'
    bad = ['/something.jpg']


class UserAgentGoogleMobileTest(UserAgentOrderingTest):
    agent = 'Googlebot-Mobile'


class GoogleURLOrderingTest(BaseRobotTest, unittest.TestCase):
    # Google also got the order wrong. You need
    # to specify the URLs from more specific to more general
    robots_txt = """\
User-agent: Googlebot
Allow: /folder1/myfile.html
Disallow: /folder1/
    """
    agent = 'googlebot'
    good = ['/folder1/myfile.html']
    bad = ['/folder1/anotherfile.html']


class DisallowQueryStringTest(BaseRobotTest, unittest.TestCase):
    # see issue #6325 for details
    robots_txt = """\
User-agent: *
Disallow: /some/path?name=value
    """
    good = ['/some/path']
    bad = ['/some/path?name=value']


class UseFirstUserAgentWildcardTest(BaseRobotTest, unittest.TestCase):
    # obey first * entry (#4108)
    robots_txt = """\
User-agent: *
Disallow: /some/path

User-agent: *
Disallow: /another/path
    """
    good = ['/another/path']
    bad = ['/some/path']


class EmptyQueryStringTest(BaseRobotTest, unittest.TestCase):
    # normalize the URL first (#17403)
    robots_txt = """\
User-agent: *
Allow: /some/path?
Disallow: /another/path?
    """
    good = ['/some/path?']
    bad = ['/another/path?']


class DefaultEntryTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Crawl-delay: 1
Request-rate: 3/15
Disallow: /cyberworld/map/
    """
    good = ['/', '/test.html']
    bad = ['/cyberworld/map/index.html']


class RobotHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_error(403, "Forbidden access")

    def log_message(self, format, *args):
        pass


@unittest.skipUnless(threading, 'threading required for this test')
class PasswordProtectedSiteTestCase(unittest.TestCase):

    def setUp(self):
        self.server = HTTPServer((support.HOST, 0), RobotHandler)

        self.t = threading.Thread(
            name='HTTPServer serving',
            target=self.server.serve_forever,
            # Short poll interval to make the test finish quickly.
            # Time between requests is short enough that we won't wake
            # up spuriously too many times.
            kwargs={'poll_interval':0.01})
        self.t.daemon = True  # In case this function raises.
        self.t.start()

    def tearDown(self):
        self.server.shutdown()
        self.t.join()
        self.server.server_close()

    @support.reap_threads
    def testPasswordProtectedSite(self):
        addr = self.server.server_address
        url = 'http://' + support.HOST + ':' + str(addr[1])
        robots_url = url + "/robots.txt"
        parser = robotparser.RobotFileParser()
        parser.set_url(url)
        parser.read()
        self.assertFalse(parser.can_fetch("*", robots_url))


class NetworkTestCase(unittest.TestCase):

    base_url = 'http://www.pythontest.net/'
    robots_txt = '{}elsewhere/robots.txt'.format(base_url)

    @classmethod
    def setUpClass(cls):
        support.requires('network')
        with support.transient_internet(cls.base_url):
            cls.parser = robotparser.RobotFileParser(cls.robots_txt)
            cls.parser.read()

    def url(self, path):
        return '{}{}{}'.format(
            self.base_url, path, '/' if not os.path.splitext(path)[1] else ''
        )

    def test_basic(self):
        self.assertFalse(self.parser.disallow_all)
        self.assertFalse(self.parser.allow_all)
        self.assertGreater(self.parser.mtime(), 0)

    def test_can_fetch(self):
        self.assertTrue(self.parser.can_fetch('*', self.url('elsewhere')))
        self.assertFalse(self.parser.can_fetch('Nutch', self.base_url))
        self.assertFalse(self.parser.can_fetch('Nutch', self.url('brian')))
        self.assertFalse(self.parser.can_fetch('Nutch', self.url('webstats')))
        self.assertFalse(self.parser.can_fetch('*', self.url('webstats')))
        self.assertTrue(self.parser.can_fetch('*', self.base_url))

    def test_read_404(self):
        parser = robotparser.RobotFileParser(self.url('i-robot.txt'))
        parser.read()
        self.assertTrue(parser.allow_all)
        self.assertFalse(parser.disallow_all)
        self.assertEqual(parser.mtime(), 0)


def test_main():
    support.run_unittest(
        UserAgentWildcardTest,
        RejectAllRobotsTest,
        UserAgentOrderingTest,
        UserAgentGoogleMobileTest,
        GoogleURLOrderingTest,
        DisallowQueryStringTest,
        UseFirstUserAgentWildcardTest,
        EmptyQueryStringTest,
        DefaultEntryTest,
        PasswordProtectedSiteTestCase,
        NetworkTestCase)


if __name__ == "__main__":
    test_main()
