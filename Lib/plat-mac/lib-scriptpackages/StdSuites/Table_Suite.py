"""Suite Table Suite: Classes for manipulating tables
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Extensies/AppleScript
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

class rows(aetools.ComponentItem):
	"""rows -  """
	want = 'crow'

row = rows

class tables(aetools.ComponentItem):
	"""tables -  """
	want = 'ctbl'

table = tables
cell._superclassnames = []
cell._privpropdict = {
	'formula' : formula,
	'protection' : protection,
}
cell._privelemdict = {
}
column._superclassnames = []
column._privpropdict = {
	'name' : name,
}
column._privelemdict = {
}
rows._superclassnames = []
rows._privpropdict = {
}
rows._privelemdict = {
}
tables._superclassnames = []
tables._privpropdict = {
}
tables._privelemdict = {
}
_Enum_prtn = {
	'read_only' : 'nmod',	# Can\xd5t change values or formulas
	'formulas_protected' : 'fpro',	# Can changes values but not formulas
	'read_2f_write' : 'modf',	# Can change values and formulas
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'ccel' : cell,
	'ccol' : column,
	'crow' : rows,
	'ctbl' : tables,
}

_propdeclarations = {
	'pfor' : formula,
	'pnam' : name,
	'ppro' : protection,
}

_compdeclarations = {
}

_enumdeclarations = {
	'prtn' : _Enum_prtn,
}
