"""
Package generated from /Volumes/Sap/Applications (Mac OS 9)/Internet Explorer 5/Internet Explorer
Resource aete resid 0 
"""
import aetools
Error = aetools.Error
import Required_Suite
import Standard_Suite
import Web_Browser_Suite
import URL_Suite
import Microsoft_Internet_Explorer
import Netscape_Suite


_code_to_module = {
	'reqd' : Required_Suite,
	'****' : Standard_Suite,
	'WWW!' : Web_Browser_Suite,
	'GURL' : URL_Suite,
	'MSIE' : Microsoft_Internet_Explorer,
	'MOSS' : Netscape_Suite,
}



_code_to_fullname = {
	'reqd' : ('Explorer.Required_Suite', 'Required_Suite'),
	'****' : ('Explorer.Standard_Suite', 'Standard_Suite'),
	'WWW!' : ('Explorer.Web_Browser_Suite', 'Web_Browser_Suite'),
	'GURL' : ('Explorer.URL_Suite', 'URL_Suite'),
	'MSIE' : ('Explorer.Microsoft_Internet_Explorer', 'Microsoft_Internet_Explorer'),
	'MOSS' : ('Explorer.Netscape_Suite', 'Netscape_Suite'),
}

from Required_Suite import *
from Standard_Suite import *
from Web_Browser_Suite import *
from URL_Suite import *
from Microsoft_Internet_Explorer import *
from Netscape_Suite import *
def getbaseclasses(v):
	if hasattr(v, '_superclassnames') and not hasattr(v, '_propdict'):
		v._propdict = {}
		v._elemdict = {}
		for superclass in v._superclassnames:
			v._propdict.update(getattr(eval(superclass), '_privpropdict', {}))
			v._elemdict.update(getattr(eval(superclass), '_privelemdict', {}))
		v._propdict.update(v._privpropdict)
		v._elemdict.update(v._privelemdict)

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


class Explorer(Required_Suite_Events,
		Standard_Suite_Events,
		Web_Browser_Suite_Events,
		URL_Suite_Events,
		Microsoft_Internet_Explorer_Events,
		Netscape_Suite_Events,
		aetools.TalkTo):
	_signature = 'MSIE'

	_moduleName = 'Explorer'

