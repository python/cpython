from warnings import warnpy3k

warnpy3k(("The htmlentitydefs module has been renamed to html.entities"
          " in Python 3.0"), stacklevel=2)

from sys import modules
import html.entities
modules["htmlentitydefs"] = html.entities
