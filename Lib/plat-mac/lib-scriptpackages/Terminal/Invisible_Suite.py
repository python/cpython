"""Suite Invisible Suite: Terms and Events for controlling the application
Level 1, version 1

Generated from /Applications/Utilities/Terminal.app/Contents/Resources/Terminal.rsrc
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'tpnm'

from StdSuites.Type_Names_Suite import *
class Invisible_Suite_Events(Type_Names_Suite_Events):

	pass


class application(aetools.ComponentItem):
	"""application - The application """
	want = 'capp'
class properties(aetools.NProperty):
	"""properties - every property of the application """
	which = 'pALL'
	want = 'reco'
application._superclassnames = []
application._privpropdict = {
	'properties' : properties,
}
application._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'capp' : application,
}

_propdeclarations = {
	'pALL' : properties,
}

_compdeclarations = {
}

_enumdeclarations = {
}
