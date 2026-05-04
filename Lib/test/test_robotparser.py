import io
import os
import threading
import unittest
import urllib.robotparser
from test import support
from test.support import socket_helper
from test.support import threading_helper
from http.server import BaseHTTPRequestHandler, HTTPServer


class BaseRobotTest:
    robots_txt = ''
    agent = 'test_robotparser'
    good = []
    bad = []
    site_maps = None

    def __init_subclass__(cls):
        super().__init_subclass__()
        # Remove tests that do nothing.
        if not cls.good:
            cls.test_good_urls = None
        if not cls.bad:
            cls.test_bad_urls = None

    def setUp(self):
        lines = io.StringIO(self.robots_txt).readlines()
        self.parser = urllib.robotparser.RobotFileParser()
        self.parser.parse(lines)

    def get_agent_and_url(self, url):
        if isinstance(url, tuple):
            agent, url = url
            return agent, url
        return self.agent, url

    def test_good_urls(self):
        for url in self.good:
            agent, url = self.get_agent_and_url(url)
            with self.subTest(url=url, agent=agent):
                self.assertTrue(self.parser.can_fetch(agent, url))

    def test_bad_urls(self):
        for url in self.bad:
            agent, url = self.get_agent_and_url(url)
            with self.subTest(url=url, agent=agent):
                self.assertFalse(self.parser.can_fetch(agent, url))

    def test_site_maps(self):
        self.assertEqual(self.parser.site_maps(), self.site_maps)


class UserAgentWildcardTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Disallow: /cyberworld/map/ # This is an infinite virtual URL space
Disallow: /tmp/ # these will soon disappear
Disallow: /foo.html
    """
    good = ['/', '/test.html']
    bad = ['/cyberworld/map/index.html', '/tmp/xxx', '/foo.html']


class CrawlDelayAndCustomAgentTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
# robots.txt for http://www.example.com/

User-agent: *
Crawl-delay: 1
Request-rate: 3/15
Disallow: /cyberworld/map/ # This is an infinite virtual URL space

# Cybermapper knows where to go.
User-agent: cybermapper
Disallow:
    """
    good = ['/', '/test.html', ('cybermapper', '/cyberworld/map/index.html')]
    bad = ['/cyberworld/map/index.html']


class SitemapTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
# robots.txt for http://www.example.com/

User-agent: *
Sitemap: http://www.gstatic.com/s2/sitemaps/profiles-sitemap.xml
Sitemap: http://www.google.com/hostednews/sitemap_index.xml
Request-rate: 3/15
Disallow: /cyberworld/map/ # This is an infinite virtual URL space

    """
    good = ['/', '/test.html']
    bad = ['/cyberworld/map/index.html']
    site_maps = ['http://www.gstatic.com/s2/sitemaps/profiles-sitemap.xml',
                 'http://www.google.com/hostednews/sitemap_index.xml']


class RejectAllRobotsTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
# go away
User-agent: *
Disallow: /
    """
    good = []
    bad = ['/cyberworld/map/index.html', '/', '/tmp/']


class BaseRequestRateTest(BaseRobotTest):
    request_rate = None
    crawl_delay = None

    def test_request_rate(self):
        parser = self.parser
        for url in self.good + self.bad:
            agent, url = self.get_agent_and_url(url)
            with self.subTest(url=url, agent=agent):
                self.assertEqual(parser.crawl_delay(agent), self.crawl_delay)

                parsed_request_rate = parser.request_rate(agent)
                self.assertEqual(parsed_request_rate, self.request_rate)
                if self.request_rate is not None:
                    self.assertIsInstance(
                        parsed_request_rate,
                        urllib.robotparser.RequestRate
                    )
                    self.assertEqual(
                        parsed_request_rate.requests,
                        self.request_rate.requests
                    )
                    self.assertEqual(
                        parsed_request_rate.seconds,
                        self.request_rate.seconds
                    )


class EmptyFileTest(BaseRequestRateTest, unittest.TestCase):
    robots_txt = ''
    good = ['/foo']


class CrawlDelayAndRequestRateTest(BaseRequestRateTest, unittest.TestCase):
    robots_txt = """\
User-agent: figtree
Crawl-delay: 3
Request-rate: 9/30
Disallow: /tmp
Disallow: /a%3cd.html
Disallow: /a%2fb.html
Disallow: /%7ejoe/index.html
    """
    agent = 'figtree'
    request_rate = urllib.robotparser.RequestRate(9, 30)
    crawl_delay = 3
    good = [('figtree', '/foo.html')]
    bad = ['/tmp', '/tmp.html', '/tmp/a.html', '/a%3cd.html', '/a%3Cd.html',
           '/a%2fb.html', '/~joe/index.html']


class DifferentAgentTest(CrawlDelayAndRequestRateTest):
    agent = 'FigTree Robot libwww-perl/5.04'


class InvalidRequestRateTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Disallow: /tmp/
Disallow: /a%3Cd.html
Disallow: /a/b.html
Disallow: /%7ejoe/index.html
Crawl-delay: 3
Request-rate: 9/banana
    """
    good = ['/tmp']
    bad = ['/tmp/', '/tmp/a.html', '/a%3cd.html', '/a%3Cd.html', '/a/b.html',
           '/%7Ejoe/index.html']
    crawl_delay = 3


class InvalidCrawlDelayTest(BaseRobotTest, unittest.TestCase):
    # From bug report #523041
    robots_txt = """\
User-Agent: *
Disallow: /.
Crawl-delay: pears
    """
    good = ['/foo.html']
    # bug report says "/" should be denied, but that is not in the RFC
    bad = []


class AnotherInvalidRequestRateTest(BaseRobotTest, unittest.TestCase):
    # also test that Allow and Diasallow works well with each other
    robots_txt = """\
User-agent: Googlebot
Allow: /folder1/myfile.html
Disallow: /folder1/
Request-rate: whale/banana
    """
    agent = 'Googlebot'
    good = ['/folder1/myfile.html']
    bad = ['/folder1/anotherfile.html']


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
Disallow: /another/path?
Disallow: /yet/one/path?name=value&more
    """
    good = ['/some/path', '/some/path?',
            '/some/path%3Fname=value', '/some/path?name%3Dvalue',
            '/another/path', '/another/path%3F',
            '/yet/one/path?name=value%26more']
    bad = ['/some/path?name=value'
           '/another/path?', '/another/path?name=value',
           '/yet/one/path?name=value&more']


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


class PercentEncodingTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Disallow: /a1/Z-._~ # unreserved characters
Disallow: /a2/%5A%2D%2E%5F%7E # percent-encoded unreserved characters
Disallow: /u1/%F0%9F%90%8D # percent-encoded ASCII Unicode character
Disallow: /u2/%f0%9f%90%8d
Disallow: /u3/\U0001f40d # raw non-ASCII Unicode character
Disallow: /v1/%F0 # percent-encoded non-ASCII octet
Disallow: /v2/%f0
Disallow: /v3/\udcf0 # raw non-ASCII octet
Disallow: /p1%xy # raw percent
Disallow: /p2%
Disallow: /p3%25xy # percent-encoded percent
Disallow: /p4%2525xy # double percent-encoded percent
Disallow: /john%20smith # space
Disallow: /john doe
Disallow: /trailingspace%20
Disallow: /question%3Fq=v # not query
Disallow: /hash%23f # not fragment
Disallow: /dollar%24
Disallow: /asterisk%2A
Disallow: /sub/dir
Disallow: /slash%2F
Disallow: /query/question?q=%3F
Disallow: /query/raw/question?q=?
Disallow: /query/eq?q%3Dv
Disallow: /query/amp?q=v%26a
"""
    good = [
        '/u1/%F0', '/u1/%f0',
        '/u2/%F0', '/u2/%f0',
        '/u3/%F0', '/u3/%f0',
        '/p1%2525xy', '/p2%f0', '/p3%2525xy', '/p4%xy', '/p4%25xy',
        '/question?q=v',
        '/dollar', '/asterisk',
        '/query/eq?q=v',
        '/query/amp?q=v&a',
    ]
    bad = [
        '/a1/Z-._~', '/a1/%5A%2D%2E%5F%7E',
        '/a2/Z-._~', '/a2/%5A%2D%2E%5F%7E',
        '/u1/%F0%9F%90%8D', '/u1/%f0%9f%90%8d', '/u1/\U0001f40d',
        '/u2/%F0%9F%90%8D', '/u2/%f0%9f%90%8d', '/u2/\U0001f40d',
        '/u3/%F0%9F%90%8D', '/u3/%f0%9f%90%8d', '/u3/\U0001f40d',
        '/v1/%F0', '/v1/%f0', '/v1/\udcf0', '/v1/\U0001f40d',
        '/v2/%F0', '/v2/%f0', '/v2/\udcf0', '/v2/\U0001f40d',
        '/v3/%F0', '/v3/%f0', '/v3/\udcf0', '/v3/\U0001f40d',
        '/p1%xy', '/p1%25xy',
        '/p2%', '/p2%25', '/p2%2525', '/p2%xy',
        '/p3%xy', '/p3%25xy',
        '/p4%2525xy',
        '/john%20smith', '/john smith',
        '/john%20doe', '/john doe',
        '/trailingspace%20', '/trailingspace ',
        '/question%3Fq=v',
        '/hash#f', '/hash%23f',
        '/dollar$', '/dollar%24',
        '/asterisk*', '/asterisk%2A',
        '/sub/dir', '/sub%2Fdir',
        '/slash%2F', '/slash/',
        '/query/question?q=?', '/query/question?q=%3F',
        '/query/raw/question?q=?', '/query/raw/question?q=%3F',
        '/query/eq?q%3Dv',
        '/query/amp?q=v%26a',
    ]
    # other reserved characters
    for c in ":/#[]@!$&'()*+,;=":
        robots_txt += f'Disallow: /raw{c}\nDisallow: /pc%{ord(c):02X}\n'
        bad.append(f'/raw{c}')
        bad.append(f'/raw%{ord(c):02X}')
        bad.append(f'/pc{c}')
        bad.append(f'/pc%{ord(c):02X}')


class DefaultEntryTest(BaseRequestRateTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Crawl-delay: 1
Request-rate: 3/15
Disallow: /cyberworld/map/
    """
    request_rate = urllib.robotparser.RequestRate(3, 15)
    crawl_delay = 1
    good = ['/', '/test.html']
    bad = ['/cyberworld/map/index.html']


class StringFormattingTest(BaseRobotTest, unittest.TestCase):
    robots_txt = """\
User-agent: *
Crawl-delay: 1
Request-rate: 3/15
Disallow: /cyberworld/map/ # This is an infinite virtual URL space

# Cybermapper knows where to go.
User-agent: cybermapper
Disallow: /some/path
    """

    expected_output = """\
User-agent: cybermapper
Disallow: /some/path

User-agent: *
Crawl-delay: 1
Request-rate: 3/15
Disallow: /cyberworld/map/\
"""

    def test_string_formatting(self):
        self.assertEqual(str(self.parser), self.expected_output)


@unittest.skipUnless(
    support.has_socket_support,
    "Socket server requires working socket."
)
class BaseLocalNetworkTestCase:

    def setUp(self):
        # clear _opener global variable
        self.addCleanup(urllib.request.urlcleanup)

        self.server = HTTPServer((socket_helper.HOST, 0), self.RobotHandler)

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


SAMPLE_ROBOTS_TXT = b'''\
User-agent: test_robotparser
Disallow: /utf8/\xf0\x9f\x90\x8d
Disallow: /non-utf8/\xf0
Disallow: //[spam]/path
'''


class LocalNetworkTestCase(BaseLocalNetworkTestCase, unittest.TestCase):
    class RobotHandler(BaseHTTPRequestHandler):

        def do_GET(self):
            self.send_response(200)
            self.end_headers()
            self.wfile.write(SAMPLE_ROBOTS_TXT)

        def log_message(self, format, *args):
            pass

    @threading_helper.reap_threads
    def testRead(self):
        # Test that reading a weird robots.txt doesn't fail.
        addr = self.server.server_address
        url = f'http://{socket_helper.HOST}:{addr[1]}'
        robots_url = url + '/robots.txt'
        parser = urllib.robotparser.RobotFileParser()
        parser.set_url(robots_url)
        parser.read()
        # And it can even interpret the weird paths in some reasonable way.
        agent = 'test_robotparser'
        self.assertTrue(parser.can_fetch(agent, robots_url))
        self.assertTrue(parser.can_fetch(agent, url + '/utf8/'))
        self.assertFalse(parser.can_fetch(agent, url + '/utf8/\U0001f40d'))
        self.assertFalse(parser.can_fetch(agent, url + '/utf8/%F0%9F%90%8D'))
        self.assertFalse(parser.can_fetch(agent, url + '/utf8/\U0001f40d'))
        self.assertTrue(parser.can_fetch(agent, url + '/non-utf8/'))
        self.assertFalse(parser.can_fetch(agent, url + '/non-utf8/%F0'))
        self.assertFalse(parser.can_fetch(agent, url + '/non-utf8/\U0001f40d'))
        self.assertFalse(parser.can_fetch(agent, url + '/%2F[spam]/path'))


class PasswordProtectedSiteTestCase(BaseLocalNetworkTestCase, unittest.TestCase):
    class RobotHandler(BaseHTTPRequestHandler):

        def do_GET(self):
            self.send_error(403, "Forbidden access")

        def log_message(self, format, *args):
            pass

    @threading_helper.reap_threads
    def testPasswordProtectedSite(self):
        addr = self.server.server_address
        url = 'http://' + socket_helper.HOST + ':' + str(addr[1])
        robots_url = url + "/robots.txt"
        parser = urllib.robotparser.RobotFileParser()
        parser.set_url(url)
        parser.read()
        self.assertFalse(parser.can_fetch("*", robots_url))


@support.requires_working_socket()
class NetworkTestCase(unittest.TestCase):

    base_url = 'http://www.pythontest.net/'
    robots_txt = '{}elsewhere/robots.txt'.format(base_url)

    @classmethod
    def setUpClass(cls):
        support.requires('network')
        with socket_helper.transient_internet(cls.base_url):
            cls.parser = urllib.robotparser.RobotFileParser(cls.robots_txt)
            cls.parser.read()

    def url(self, path):
        return '{}{}{}'.format(
            self.base_url, path, '/' if not os.path.splitext(path)[1] else ''
        )

    def test_basic(self):
        self.assertFalse(self.parser.disallow_all)
        self.assertFalse(self.parser.allow_all)
        self.assertGreater(self.parser.mtime(), 0)
        self.assertFalse(self.parser.crawl_delay('*'))
        self.assertFalse(self.parser.request_rate('*'))

    def test_can_fetch(self):
        self.assertTrue(self.parser.can_fetch('*', self.url('elsewhere')))
        self.assertFalse(self.parser.can_fetch('Nutch', self.base_url))
        self.assertFalse(self.parser.can_fetch('Nutch', self.url('brian')))
        self.assertFalse(self.parser.can_fetch('Nutch', self.url('webstats')))
        self.assertFalse(self.parser.can_fetch('*', self.url('webstats')))
        self.assertTrue(self.parser.can_fetch('*', self.base_url))

    def test_read_404(self):
        parser = urllib.robotparser.RobotFileParser(self.url('i-robot.txt'))
        parser.read()
        self.assertTrue(parser.allow_all)
        self.assertFalse(parser.disallow_all)
        self.assertEqual(parser.mtime(), 0)
        self.assertIsNone(parser.crawl_delay('*'))
        self.assertIsNone(parser.request_rate('*'))

if __name__=='__main__':
    unittest.main()
