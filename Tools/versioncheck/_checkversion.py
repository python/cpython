"""This file (which is sourced, not imported) checks the version of the
"versioncheck" package. It is also an example of how to format your own
_checkversion.py file"""

import pyversioncheck

_PACKAGE="versioncheck"
_VERSION="1.0"
_URL="http://www.cwi.nl/~jack/versioncheck/curversion.txt"

try:
	_myverbose=VERBOSE
except NameError:
	_myverbose=1
	
pyversioncheck.versioncheck(_PACKAGE, _URL, _VERSION, verbose=_myverbose)
