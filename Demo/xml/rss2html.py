import sys

from xml.sax import make_parser, handler

# --- Templates

top = \
"""
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
  <TITLE>%s</TITLE>
</HEAD>

<BODY>
<H1>%s</H1>
"""

bottom = \
"""
</ul>

<HR>
<ADDRESS>
Converted to HTML by sax_rss2html.py.
</ADDRESS>

</BODY>
</HTML>
"""

# --- The ContentHandler

class RSSHandler(handler.ContentHandler):

    def __init__(self, out = sys.stdout):
        handler.ContentHandler.__init__(self)
        self._out = out

        self._text = ""
        self._parent = None
        self._list_started = 0
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
                    self._list_started = 1

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

parser = make_parser()
parser.setContentHandler(RSSHandler())
parser.parse(sys.argv[1])
