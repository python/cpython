"""
Package generated from Macintosh HD:Internet:Internet-programma's:Netscape Communicatoré-map:Netscape Communicatoré
Resource aete resid 0 
"""
import aetools
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


class Netscape(Required_suite_Events,
		Standard_Suite_Events,
		Standard_URL_suite_Events,
		WorldWideWeb_suite_Events,
		Mozilla_suite_Events,
		PowerPlant_Events,
		Text_Events,
		aetools.TalkTo):
	_signature = 'MOSS'

