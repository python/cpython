"""
Package generated from /Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator™ Folder/Netscape Communicator™
Resource aete resid 0 
"""
import aetools
Error = aetools.Error
import Required_suite
import Standard_Suite
import Standard_URL_suite
import WorldWideWeb_suite
import Mozilla_suite
import PowerPlant
import Text


_code_to_module = {
	'reqd' : Required_suite,
	'CoRe' : Standard_Suite,
	'GURL' : Standard_URL_suite,
	'WWW!' : WorldWideWeb_suite,
	'MOSS' : Mozilla_suite,
	'ppnt' : PowerPlant,
	'TEXT' : Text,
}



_code_to_fullname = {
	'reqd' : ('Netscape.Required_suite', 'Required_suite'),
	'CoRe' : ('Netscape.Standard_Suite', 'Standard_Suite'),
	'GURL' : ('Netscape.Standard_URL_suite', 'Standard_URL_suite'),
	'WWW!' : ('Netscape.WorldWideWeb_suite', 'WorldWideWeb_suite'),
	'MOSS' : ('Netscape.Mozilla_suite', 'Mozilla_suite'),
	'ppnt' : ('Netscape.PowerPlant', 'PowerPlant'),
	'TEXT' : ('Netscape.Text', 'Text'),
}

from Required_suite import *
from Standard_Suite import *
from Standard_URL_suite import *
from WorldWideWeb_suite import *
from Mozilla_suite import *
from PowerPlant import *
from Text import *
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
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(text)
getbaseclasses(styleset)
getbaseclasses(StdSuites.Text_Suite.paragraph)
getbaseclasses(StdSuites.Text_Suite.character)
getbaseclasses(StdSuites.Text_Suite.text_style_info)
getbaseclasses(StdSuites.Text_Suite.word)
getbaseclasses(StdSuites.Text_Suite.text_flow)
getbaseclasses(StdSuites.Text_Suite.line)
getbaseclasses(StdSuites.Text_Suite.text)

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cwin' : window,
	'capp' : application,
	'ctxt' : text,
	'stys' : styleset,
	'cpar' : StdSuites.Text_Suite.paragraph,
	'cha ' : StdSuites.Text_Suite.character,
	'tsty' : StdSuites.Text_Suite.text_style_info,
	'cwor' : StdSuites.Text_Suite.word,
	'cflo' : StdSuites.Text_Suite.text_flow,
	'clin' : StdSuites.Text_Suite.line,
	'ctxt' : StdSuites.Text_Suite.text,
}


class Netscape(Required_suite_Events,
		Standard_Suite_Events,
		Standard_URL_suite_Events,
		WorldWideWeb_suite_Events,
		Mozilla_suite_Events,
		PowerPlant_Events,
		Text_Events,
		aetools.TalkTo):
	_signature = 'MOSS'

	_moduleName = 'Netscape'

