"""
Package generated from Macintosh HD:Internet:Internet-programma's:Internet Explorer 4.5-map:Internet Explorer 4.5
Resource aete resid 0 
"""
import aetools
import Web_Browser_Suite
import URL_Suite
import Microsoft_Internet_Explorer
import Netscape_Suite
import Required_Suite


_code_to_module = {
	'WWW!' : Web_Browser_Suite,
	'GURL' : URL_Suite,
	'MSIE' : Microsoft_Internet_Explorer,
	'MOSS' : Netscape_Suite,
	'reqd' : Required_Suite,
}



_code_to_fullname = {
	'WWW!' : ('Explorer.Web_Browser_Suite', 'Web_Browser_Suite'),
	'GURL' : ('Explorer.URL_Suite', 'URL_Suite'),
	'MSIE' : ('Explorer.Microsoft_Internet_Explorer', 'Microsoft_Internet_Explorer'),
	'MOSS' : ('Explorer.Netscape_Suite', 'Netscape_Suite'),
	'reqd' : ('Explorer.Required_Suite', 'Required_Suite'),
}

from Web_Browser_Suite import *
from URL_Suite import *
from Microsoft_Internet_Explorer import *
from Netscape_Suite import *
from Required_Suite import *


class Explorer(Web_Browser_Suite_Events,
		URL_Suite_Events,
		Microsoft_Internet_Explorer_Events,
		Netscape_Suite_Events,
		Required_Suite_Events,
		aetools.TalkTo):
	_signature = 'MSIE'

