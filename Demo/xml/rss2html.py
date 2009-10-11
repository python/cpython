"""
A demo that reads in an RSS XML document and emits an HTML file containing
a list of the individual items in the feed.
"""

import sys
import codecs

from xml.sax import make_parser, handler

# --- Templates

top = """\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
  <title>%s</title>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>

<body>
<h1>%s</h1>
"""

bottom = """
</ul>

<hr>
<address>
Converted to HTML by rss2html.py.
</address>

</body>
</html>
"""

# --- The ContentHandler

class RSSHandler(handler.ContentHandler):

    def __init__(self, out=sys.stdout):
        handler.ContentHandler.__init__(self)
        self._out = codecs.getwriter('utf-8')(out)

        self._text = ""
        self._parent = None
        self._list_started = False
        self._title = None
        self._link = None
        self._descr = ""

    # ContentHandler methods

    def startElement(self, name, attrs):
        if name == "channel" or name == "image" or name == "item":
            self._parent = name

        self._text = ""

    def endElement(self, name):
        if self._parent == "channel":
            if name == "title":
                self._out.write(top % (self._text, self._text))
            elif name == "description":
                self._out.write("<p>%s</p>\n" % self._text)

        elif self._parent == "item":
            if name == "title":
                self._title = self._text
            elif name == "link":
                self._link = self._text
            elif name == "description":
                self._descr = self._text
            elif name == "item":
                if not self._list_started:
                    self._out.write("<ul>\n")
                    self._list_started = True

                self._out.write('  <li><a href="%s">%s</a> %s\n' %
                                (self._link, self._title, self._descr))

                self._title = None
                self._link = None
                self._descr = ""

        if name == "rss":
            self._out.write(bottom)

    def characters(self, content):
        self._text = self._text + content

# --- Main program

if __name__ == '__main__':
    parser = make_parser()
    parser.setContentHandler(RSSHandler())
    parser.parse(sys.argv[1])
