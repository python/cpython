"""Suite Table Suite: Classes for manipulating tables
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Extensies:AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'tbls'

class Table_Suite_Events:

	pass


class cell(aetools.ComponentItem):
	"""cell - A cell """
	want = 'ccel'
class formula(aetools.NProperty):
	"""formula - the formula of the cell """
	which = 'pfor'
	want = 'ctxt'
class protection(aetools.NProperty):
	"""protection - Indicates whether value or formula in the cell can be changed """
	which = 'ppro'
	want = 'prtn'

cells = cell

class column(aetools.ComponentItem):
	"""column - A column """
	want = 'ccol'
class name(aetools.NProperty):
	"""name - the name of the column """
	which = 'pnam'
	want = 'itxt'

columns = column

class row(aetools.ComponentItem):
	"""row - A row """
	want = 'crow'

rows = row

class table(aetools.ComponentItem):
	"""table - A table """
	want = 'ctbl'

tables = table
cell._propdict = {
	'formula' : formula,
	'protection' : protection,
}
cell._elemdict = {
}
column._propdict = {
	'name' : name,
}
column._elemdict = {
}
row._propdict = {
}
row._elemdict = {
}
table._propdict = {
}
table._elemdict = {
}
_Enum_prtn = {
	'read_only' : 'nmod',	# Can’t change values or formulas
	'formulas_protected' : 'fpro',	# Can changes values but not formulas
	'read_2f_write' : 'modf',	# Can change values and formulas
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'ccel' : cell,
	'ctbl' : table,
	'ccol' : column,
	'crow' : row,
}

_propdeclarations = {
	'ppro' : protection,
	'pfor' : formula,
	'pnam' : name,
}

_compdeclarations = {
}

_enumdeclarations = {
	'prtn' : _Enum_prtn,
}
