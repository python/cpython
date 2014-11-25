import unittest, StringIO, robotparser
from test import test_support
from urllib2 import urlopen, HTTPError

HAVE_HTTPS = True
try:
    from urllib2 import HTTPSHandler
except ImportError:
    HAVE_HTTPS = False

class RobotTestCase(unittest.TestCase):
    def __init__(self, index, parser, url, good, agent):
        unittest.TestCase.__init__(self)
        if good:
            self.str = "RobotTest(%d, good, %s)" % (index, url)
        else:
            self.str = "RobotTest(%d, bad, %s)" % (index, url)
        self.parser = parser
        self.url = url
        self.good = good
        self.agent = agent

    def runTest(self):
        if isinstance(self.url, tuple):
            agent, url = self.url
        else:
            url = self.url
            agent = self.agent
        if self.good:
            self.assertTrue(self.parser.can_fetch(agent, url))
        else:
            self.assertFalse(self.parser.can_fetch(agent, url))

    def __str__(self):
        return self.str

tests = unittest.TestSuite()

def RobotTest(index, robots_txt, good_urls, bad_urls,
              agent="test_robotparser"):

    lines = StringIO.StringIO(robots_txt).readlines()
    parser = robotparser.RobotFileParser()
    parser.parse(lines)
    for url in good_urls:
        tests.addTest(RobotTestCase(index, parser, url, 1, agent))
    for url in bad_urls:
        tests.addTest(RobotTestCase(index, parser, url, 0, agent))

# Examples from http://www.robotstxt.org/wc/norobots.html (fetched 2002)

# 1.
doc = """
User-agent: *
Disallow: /cyberworld/map/ # This is an infinite virtual URL space
Disallow: /tmp/ # these will soon disappear
Disallow: /foo.html
"""

good = ['/','/test.html']
bad = ['/cyberworld/map/index.html','/tmp/xxx','/foo.html']

RobotTest(1, doc, good, bad)

# 2.
doc = """
# robots.txt for http://www.example.com/

User-agent: *
Disallow: /cyberworld/map/ # This is an infinite virtual URL space

# Cybermapper knows where to go.
User-agent: cybermapper
Disallow:

"""

good = ['/','/test.html',('cybermapper','/cyberworld/map/index.html')]
bad = ['/cyberworld/map/index.html']

RobotTest(2, doc, good, bad)

# 3.
doc = """
# go away
User-agent: *
Disallow: /
"""

good = []
bad = ['/cyberworld/map/index.html','/','/tmp/']

RobotTest(3, doc, good, bad)

# Examples from http://www.robotstxt.org/wc/norobots-rfc.html (fetched 2002)

# 4.
doc = """
User-agent: figtree
Disallow: /tmp
Disallow: /a%3cd.html
Disallow: /a%2fb.html
Disallow: /%7ejoe/index.html
"""

good = [] # XFAIL '/a/b.html'
bad = ['/tmp','/tmp.html','/tmp/a.html',
       '/a%3cd.html','/a%3Cd.html','/a%2fb.html',
       '/~joe/index.html'
       ]

RobotTest(4, doc, good, bad, 'figtree')
RobotTest(5, doc, good, bad, 'FigTree Robot libwww-perl/5.04')

# 6.
doc = """
User-agent: *
Disallow: /tmp/
Disallow: /a%3Cd.html
Disallow: /a/b.html
Disallow: /%7ejoe/index.html
"""

good = ['/tmp',] # XFAIL: '/a%2fb.html'
bad = ['/tmp/','/tmp/a.html',
       '/a%3cd.html','/a%3Cd.html',"/a/b.html",
       '/%7Ejoe/index.html']

RobotTest(6, doc, good, bad)

# From bug report #523041

# 7.
doc = """
User-Agent: *
Disallow: /.
"""

good = ['/foo.html']
bad = [] # Bug report says "/" should be denied, but that is not in the RFC

RobotTest(7, doc, good, bad)

# From Google: http://www.google.com/support/webmasters/bin/answer.py?hl=en&answer=40364

# 8.
doc = """
User-agent: Googlebot
Allow: /folder1/myfile.html
Disallow: /folder1/
"""

good = ['/folder1/myfile.html']
bad = ['/folder1/anotherfile.html']

RobotTest(8, doc, good, bad, agent="Googlebot")

# 9.  This file is incorrect because "Googlebot" is a substring of
#     "Googlebot-Mobile", so test 10 works just like test 9.
doc = """
User-agent: Googlebot
Disallow: /

User-agent: Googlebot-Mobile
Allow: /
"""

good = []
bad = ['/something.jpg']

RobotTest(9, doc, good, bad, agent="Googlebot")

good = []
bad = ['/something.jpg']

RobotTest(10, doc, good, bad, agent="Googlebot-Mobile")

# 11.  Get the order correct.
doc = """
User-agent: Googlebot-Mobile
Allow: /

User-agent: Googlebot
Disallow: /
"""

good = []
bad = ['/something.jpg']

RobotTest(11, doc, good, bad, agent="Googlebot")

good = ['/something.jpg']
bad = []

RobotTest(12, doc, good, bad, agent="Googlebot-Mobile")


# 13.  Google also got the order wrong in #8.  You need to specify the
#      URLs from more specific to more general.
doc = """
User-agent: Googlebot
Allow: /folder1/myfile.html
Disallow: /folder1/
"""

good = ['/folder1/myfile.html']
bad = ['/folder1/anotherfile.html']

RobotTest(13, doc, good, bad, agent="googlebot")


# 14. For issue #6325 (query string support)
doc = """
User-agent: *
Disallow: /some/path?name=value
"""

good = ['/some/path']
bad = ['/some/path?name=value']

RobotTest(14, doc, good, bad)

# 15. For issue #4108 (obey first * entry)
doc = """
User-agent: *
Disallow: /some/path

User-agent: *
Disallow: /another/path
"""

good = ['/another/path']
bad = ['/some/path']

RobotTest(15, doc, good, bad)

# 16. Empty query (issue #17403). Normalizing the url first.
doc = """
User-agent: *
Allow: /some/path?
Disallow: /another/path?
"""

good = ['/some/path?']
bad = ['/another/path?']

RobotTest(16, doc, good, bad)


class NetworkTestCase(unittest.TestCase):

    def testPasswordProtectedSite(self):
        test_support.requires('network')
        with test_support.transient_internet('mueblesmoraleda.com'):
            url = 'http://mueblesmoraleda.com'
            robots_url = url + "/robots.txt"
            # First check the URL is usable for our purposes, since the
            # test site is a bit flaky.
            try:
                urlopen(robots_url)
            except HTTPError as e:
                if e.code not in {401, 403}:
                    self.skipTest(
                        "%r should return a 401 or 403 HTTP error, not %r"
                        % (robots_url, e.code))
            else:
                self.skipTest(
                    "%r should return a 401 or 403 HTTP error, not succeed"
                    % (robots_url))
            parser = robotparser.RobotFileParser()
            parser.set_url(url)
            try:
                parser.read()
            except IOError:
                self.skipTest('%s is unavailable' % url)
            self.assertEqual(parser.can_fetch("*", robots_url), False)

    @unittest.skipUnless(HAVE_HTTPS, 'need SSL support to download license')
    @test_support.system_must_validate_cert
    def testPythonOrg(self):
        test_support.requires('network')
        with test_support.transient_internet('www.python.org'):
            parser = robotparser.RobotFileParser(
                "https://www.python.org/robots.txt")
            parser.read()
            self.assertTrue(
                parser.can_fetch("*", "https://www.python.org/robots.txt"))


def test_main():
    test_support.run_unittest(tests)
    test_support.run_unittest(NetworkTestCase)

if __name__=='__main__':
    test_support.verbose = 1
    test_main()
