import formatter
import htmllib
import unittest

import test_support


class AnchorCollector(htmllib.HTMLParser):
    def __init__(self, *args, **kw):
        self.__anchors = []
        htmllib.HTMLParser.__init__(self, *args, **kw)

    def get_anchor_info(self):
        return self.__anchors

    def anchor_bgn(self, *args):
        self.__anchors.append(args)


class HTMLParserTestCase(unittest.TestCase):
    def test_anchor_collection(self):
        # See SF bug #467059.
        parser = AnchorCollector(formatter.NullFormatter(), verbose=1)
        parser.feed(
            """<a href='http://foo.org/' name='splat'> </a>
            <a href='http://www.python.org/'> </a>
            <a name='frob'> </a>
            """)
        parser.close()
        self.assertEquals(parser.get_anchor_info(),
                          [('http://foo.org/', 'splat', ''),
                           ('http://www.python.org/', '', ''),
                           ('', 'frob', ''),
                           ])


def test_main():
    test_support.run_unittest(HTMLParserTestCase)


if __name__ == "__main__":
    test_main()
