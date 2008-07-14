"""
Package generated from /Applications/Internet Explorer.app
"""

from warnings import warnpy3k
warnpy3k("In 3.x, the Explorer module is removed.", stacklevel=2)

import aetools
Error = aetools.Error
import Standard_Suite
import URL_Suite
import Netscape_Suite
import Microsoft_Internet_Explorer
import Web_Browser_Suite
import Required_Suite


_code_to_module = {
    '****' : Standard_Suite,
    'GURL' : URL_Suite,
    'MOSS' : Netscape_Suite,
    'MSIE' : Microsoft_Internet_Explorer,
    'WWW!' : Web_Browser_Suite,
    'reqd' : Required_Suite,
}



_code_to_fullname = {
    '****' : ('Explorer.Standard_Suite', 'Standard_Suite'),
    'GURL' : ('Explorer.URL_Suite', 'URL_Suite'),
    'MOSS' : ('Explorer.Netscape_Suite', 'Netscape_Suite'),
    'MSIE' : ('Explorer.Microsoft_Internet_Explorer', 'Microsoft_Internet_Explorer'),
    'WWW!' : ('Explorer.Web_Browser_Suite', 'Web_Browser_Suite'),
    'reqd' : ('Explorer.Required_Suite', 'Required_Suite'),
}

from Standard_Suite import *
from URL_Suite import *
from Netscape_Suite import *
from Microsoft_Internet_Explorer import *
from Web_Browser_Suite import *
from Required_Suite import *

def getbaseclasses(v):
    if not getattr(v, '_propdict', None):
        v._propdict = {}
        v._elemdict = {}
        for superclassname in getattr(v, '_superclassnames', []):
            superclass = eval(superclassname)
            getbaseclasses(superclass)
            v._propdict.update(getattr(superclass, '_propdict', {}))
            v._elemdict.update(getattr(superclass, '_elemdict', {}))
        v._propdict.update(getattr(v, '_privpropdict', {}))
        v._elemdict.update(getattr(v, '_privelemdict', {}))

import StdSuites

#
# Set property and element dictionaries now that all classes have been defined
#
getbaseclasses(application)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
}


class Explorer(Standard_Suite_Events,
        URL_Suite_Events,
        Netscape_Suite_Events,
        Microsoft_Internet_Explorer_Events,
        Web_Browser_Suite_Events,
        Required_Suite_Events,
        aetools.TalkTo):
    _signature = 'MSIE'

    _moduleName = 'Explorer'

    _elemdict = application._elemdict
    _propdict = application._propdict
