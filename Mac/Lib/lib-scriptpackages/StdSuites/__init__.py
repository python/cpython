"""
Package generated from Moes:Systeemmap:Extensies:AppleScript
Resource aeut resid 0 Standard Event Suites for English
"""
import aetools
Error = aetools.Error
import AppleScript_Suite
import Required_Suite
import Standard_Suite
import Text_Suite
import QuickDraw_Graphics_Suite
import QuickDraw_Graphics_Suppleme
import Table_Suite
import Macintosh_Connectivity_Clas
import Type_Names_Suite


_code_to_module = {
	'ascr' : AppleScript_Suite,
	'reqd' : Required_Suite,
	'core' : Standard_Suite,
	'TEXT' : Text_Suite,
	'qdrw' : QuickDraw_Graphics_Suite,
	'qdsp' : QuickDraw_Graphics_Suppleme,
	'tbls' : Table_Suite,
	'macc' : Macintosh_Connectivity_Clas,
	'tpnm' : Type_Names_Suite,
}



_code_to_fullname = {
	'ascr' : ('StdSuites.AppleScript_Suite', 'AppleScript_Suite'),
	'reqd' : ('StdSuites.Required_Suite', 'Required_Suite'),
	'core' : ('StdSuites.Standard_Suite', 'Standard_Suite'),
	'TEXT' : ('StdSuites.Text_Suite', 'Text_Suite'),
	'qdrw' : ('StdSuites.QuickDraw_Graphics_Suite', 'QuickDraw_Graphics_Suite'),
	'qdsp' : ('StdSuites.QuickDraw_Graphics_Suppleme', 'QuickDraw_Graphics_Suppleme'),
	'tbls' : ('StdSuites.Table_Suite', 'Table_Suite'),
	'macc' : ('StdSuites.Macintosh_Connectivity_Clas', 'Macintosh_Connectivity_Clas'),
	'tpnm' : ('StdSuites.Type_Names_Suite', 'Type_Names_Suite'),
}

from AppleScript_Suite import *
from Required_Suite import *
from Standard_Suite import *
from Text_Suite import *
from QuickDraw_Graphics_Suite import *
from QuickDraw_Graphics_Suppleme import *
from Table_Suite import *
from Macintosh_Connectivity_Clas import *
from Type_Names_Suite import *


class StdSuites(AppleScript_Suite_Events,
		Required_Suite_Events,
		Standard_Suite_Events,
		Text_Suite_Events,
		QuickDraw_Graphics_Suite_Events,
		QuickDraw_Graphics_Suppleme_Events,
		Table_Suite_Events,
		Macintosh_Connectivity_Clas_Events,
		Type_Names_Suite_Events,
		aetools.TalkTo):
	_signature = 'ascr'

