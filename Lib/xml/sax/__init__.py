"""Simple API for XML (SAX) implementation for Python.

This module provides an implementation of the SAX 2 interface;
information about the Java version of the interface can be found at
http://www.megginson.com/SAX/.  The Python version of the interface is
documented at <...>.

This package contains the following modules:

saxutils -- Implementation of the convenience functions normally used
            to work with SAX.

saxlib -- Implementation of the shared SAX 2 classes.

drv_pyexpat -- Driver that allows use of the Expat parser with the classes
               defined in saxlib.

"""

from handler import *
from expatreader import *
from _exceptions import *
from saxutils import *
from _exceptions import SAXParseException
import xmlreader
