from warnings import warnpy3k

warnpy3k(("The HTMLParser module has been renamed to html.parser"
          " in Python 3.0"), stacklevel=2)

from sys import modules
import html.parser
modules["HTMLParser"] = html.parser
