"""
Package generated from Moes:Applications (Mac OS 9):Internet Explorer 5:Internet Explorer
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


class Explorer(Required_Suite_Events,
		Standard_Suite_Events,
		Web_Browser_Suite_Events,
		URL_Suite_Events,
		Microsoft_Internet_Explorer_Events,
		Netscape_Suite_Events,
		aetools.TalkTo):
	_signature = 'MSIE'

