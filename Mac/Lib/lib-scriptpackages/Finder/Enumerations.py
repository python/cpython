"""Suite Enumerations: Enumerations for the Finder
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'tpnm'

from StdSuites.Type_Names_Suite import *
class Enumerations_Events(Type_Names_Suite_Events):

	pass

_Enum_ipnl = {
	'General_Information_panel' : 'gpnl',	# 
	'Sharing_panel' : 'spnl',	# 
	'Memory_panel' : 'mpnl',	# 
	'Status_and_Configuration_panel' : 'scnl',	# 
	'Fonts_panel' : 'fpnl',	# 
}

_Enum_pple = {
	'General_Preferences_panel' : 'pgnp',	# 
	'Label_Preferences_panel' : 'plbp',	# 
	'Icon_View_Preferences_panel' : 'pivp',	# 
	'Button_View_Preferences_panel' : 'pbvp',	# 
	'List_View_Preferences_panel' : 'plvp',	# 
}

_Enum_earr = {
	'not_arranged' : 'narr',	# 
	'snap_to_grid' : 'grda',	# 
	'arranged_by_name' : 'nama',	# 
	'arranged_by_modification_date' : 'mdta',	# 
	'arranged_by_creation_date' : 'cdta',	# 
	'arranged_by_size' : 'siza',	# 
	'arranged_by_kind' : 'kina',	# 
	'arranged_by_label' : 'laba',	# 
}

_Enum_sodr = {
	'normal' : 'snrm',	# 
	'reversed' : 'srvs',	# 
}

_Enum_isiz = {
	'mini' : 'miic',	# 
	'small' : 'smic',	# 
	'large' : 'lgic',	# 
}

_Enum_vwby = {
	'conflicts' : 'cflc',	# 
	'existing_items' : 'exsi',	# 
	'small_icon' : 'smic',	# 
	'icon' : 'iimg',	# 
	'name' : 'pnam',	# 
	'modification_date' : 'asmo',	# 
	'size' : 'ptsz',	# 
	'kind' : 'kind',	# 
	'comment' : 'comt',	# 
	'label' : 'labi',	# 
	'version' : 'vers',	# 
	'creation_date' : 'ascd',	# 
	'small_button' : 'smbu',	# 
	'large_button' : 'lgbu',	# 
	'grid' : 'grid',	# 
	'all' : 'kyal',	# 
}

_Enum_gsen = {
	'CPU' : 'proc',	# 
	'FPU' : 'fpu ',	# 
	'MMU' : 'mmu ',	# 
	'hardware' : 'hdwr',	# 
	'operating_system' : 'os  ',	# 
	'sound_system' : 'snd ',	# 
	'memory_available' : 'lram',	# 
	'memory_installed' : 'ram ',	# 
}

_Enum_ese0 = {
	'starting_up' : 'ese2',	# 
	'running' : 'ese3',	# 
	'rebuilding_desktop' : 'ese5',	# 
	'copying' : 'ese4',	# 
	'restarting' : 'ese6',	# 
	'quitting' : 'ese7',	# 
}


#
# Indices of types declared in this module
#
_classdeclarations = {
}

_propdeclarations = {
}

_compdeclarations = {
}

_enumdeclarations = {
	'sodr' : _Enum_sodr,
	'ipnl' : _Enum_ipnl,
	'ese0' : _Enum_ese0,
	'vwby' : _Enum_vwby,
	'gsen' : _Enum_gsen,
	'isiz' : _Enum_isiz,
	'earr' : _Enum_earr,
	'pple' : _Enum_pple,
}
