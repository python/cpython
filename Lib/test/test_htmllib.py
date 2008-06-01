import formatter
import unittest

from test import test_support
htmllib = test_support.import_module('htmllib', deprecated=True)


class AnchorCollector(htmllib.HTMLParser):
    def __init__(self, *args, **kw):
        self.__anchors = []
        htmllib.HTMLParser.__init__(self, *args, **kw)

    def get_anchor_info(self):
        return self.__anchors

    def anchor_bgn(self, *args):
        self.__anchors.append(args)

class DeclCollector(htmllib.HTMLParser):
    def __init__(self, *args, **kw):
        self.__decls = []
        htmllib.HTMLParser.__init__(self, *args, **kw)

    def get_decl_info(self):
        return self.__decls

    def unknown_decl(self, data):
        self.__decls.append(data)


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

    def test_decl_collection(self):
        # See SF patch #545300
        parser = DeclCollector(formatter.NullFormatter(), verbose=1)
        parser.feed(
            """<html>
            <body>
            hallo
            <![if !supportEmptyParas]>&nbsp;<![endif]>
            </body>
            </html>
            """)
        parser.close()
        self.assertEquals(parser.get_decl_info(),
                          ["if !supportEmptyParas",
                           "endif"
                           ])

def test_main():
    test_support.run_unittest(HTMLParserTestCase)


if __name__ == "__main__":
    test_main()
