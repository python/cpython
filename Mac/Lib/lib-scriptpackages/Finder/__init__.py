"""
Package generated from Macintosh HD:Systeemmap:Finder
Resource aete resid 0 
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Earlier_terms
import Finder_Basics
import Finder_items
import Containers_and_folders
import Files_and_suitcases
import Window_classes
import Process_classes
import Type_Definitions
import Enumerations
import Obsolete_terms


_code_to_module = {
	'CoRe' : Standard_Suite,
	'tpnm' : Earlier_terms,
	'fndr' : Finder_Basics,
	'fndr' : Finder_items,
	'fndr' : Containers_and_folders,
	'fndr' : Files_and_suitcases,
	'fndr' : Window_classes,
	'fndr' : Process_classes,
	'tpdf' : Type_Definitions,
	'tpnm' : Enumerations,
	'tpnm' : Obsolete_terms,
}



_code_to_fullname = {
	'CoRe' : ('Finder.Standard_Suite', 'Standard_Suite'),
	'tpnm' : ('Finder.Earlier_terms', 'Earlier_terms'),
	'fndr' : ('Finder.Finder_Basics', 'Finder_Basics'),
	'fndr' : ('Finder.Finder_items', 'Finder_items'),
	'fndr' : ('Finder.Containers_and_folders', 'Containers_and_folders'),
	'fndr' : ('Finder.Files_and_suitcases', 'Files_and_suitcases'),
	'fndr' : ('Finder.Window_classes', 'Window_classes'),
	'fndr' : ('Finder.Process_classes', 'Process_classes'),
	'tpdf' : ('Finder.Type_Definitions', 'Type_Definitions'),
	'tpnm' : ('Finder.Enumerations', 'Enumerations'),
	'tpnm' : ('Finder.Obsolete_terms', 'Obsolete_terms'),
}

from Standard_Suite import *
from Earlier_terms import *
from Finder_Basics import *
from Finder_items import *
from Containers_and_folders import *
from Files_and_suitcases import *
from Window_classes import *
from Process_classes import *
from Type_Definitions import *
from Enumerations import *
from Obsolete_terms import *


class Finder(Standard_Suite_Events,
		Earlier_terms_Events,
		Finder_Basics_Events,
		Finder_items_Events,
		Containers_and_folders_Events,
		Files_and_suitcases_Events,
		Window_classes_Events,
		Process_classes_Events,
		Type_Definitions_Events,
		Enumerations_Events,
		Obsolete_terms_Events,
		aetools.TalkTo):
	_signature = 'MACS'

