import unittest, robotparser
import io
from test import test_support

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
            self.failUnless(self.parser.can_fetch(agent, url))
        else:
            self.failIf(self.parser.can_fetch(agent, url))

    def __str__(self):
        return self.str

tests = unittest.TestSuite()

def RobotTest(index, robots_txt, good_urls, bad_urls,
              agent="test_robotparser"):

    lines = io.StringIO(robots_txt).readlines()
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

class TestCase(unittest.TestCase):
    def runTest(self):
        test_support.requires('network')
        # whole site is password-protected.
        url = 'http://mueblesmoraleda.com'
        parser = robotparser.RobotFileParser()
        parser.set_url(url)
        parser.read()
        self.assertEqual(parser.can_fetch("*", url+"/robots.txt"), False)

def test_main():
    test_support.run_unittest(tests)
    TestCase().run()

if __name__=='__main__':
    test_support.Verbose = 1
    test_main()
