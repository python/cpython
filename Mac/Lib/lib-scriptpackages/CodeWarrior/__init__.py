"""
Package generated from Moes:Applications (Mac OS 9):Metrowerks CodeWarrior 7.0:Metrowerks CodeWarrior:CodeWarrior IDE 4.2.6
Resource aete resid 0 AppleEvent Suites
"""
import aetools
Error = aetools.Error
import Required
import Standard_Suite
import CodeWarrior_suite
import Metrowerks_Shell_Suite


_code_to_module = {
	'reqd' : Required,
	'CoRe' : Standard_Suite,
	'CWIE' : CodeWarrior_suite,
	'MMPR' : Metrowerks_Shell_Suite,
}



_code_to_fullname = {
	'reqd' : ('CodeWarrior.Required', 'Required'),
	'CoRe' : ('CodeWarrior.Standard_Suite', 'Standard_Suite'),
	'CWIE' : ('CodeWarrior.CodeWarrior_suite', 'CodeWarrior_suite'),
	'MMPR' : ('CodeWarrior.Metrowerks_Shell_Suite', 'Metrowerks_Shell_Suite'),
}

from Required import *
from Standard_Suite import *
from CodeWarrior_suite import *
from Metrowerks_Shell_Suite import *


class CodeWarrior(Required_Events,
		Standard_Suite_Events,
		CodeWarrior_suite_Events,
		Metrowerks_Shell_Suite_Events,
		aetools.TalkTo):
	_signature = 'CWIE'

