"""Suite QuickDraw Graphics Supplemental Suite: Defines transformations of graphic objects
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Extensions/AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'qdsp'

class QuickDraw_Graphics_Suppleme_Events:

	pass


class drawing_area(aetools.ComponentItem):
	"""drawing area - Container for graphics and supporting information """
	want = 'cdrw'
class rotation(aetools.NProperty):
	"""rotation - the default rotation for objects in the drawing area """
	which = 'prot'
	want = 'trot'
class scale(aetools.NProperty):
	"""scale - the default scaling for objects in the drawing area """
	which = 'pscl'
	want = 'fixd'
class translation(aetools.NProperty):
	"""translation - the default repositioning for objects in the drawing area """
	which = 'ptrs'
	want = 'QDpt'

drawing_areas = drawing_area

class graphic_group(aetools.ComponentItem):
	"""graphic group - Group of graphics """
	want = 'cpic'

graphic_groups = graphic_group
drawing_area._superclassnames = []
drawing_area._privpropdict = {
	'rotation' : rotation,
	'scale' : scale,
	'translation' : translation,
}
drawing_area._privelemdict = {
}
graphic_group._superclassnames = []
graphic_group._privpropdict = {
}
graphic_group._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cpic' : graphic_group,
	'cdrw' : drawing_area,
}

_propdeclarations = {
	'prot' : rotation,
	'ptrs' : translation,
	'pscl' : scale,
}

_compdeclarations = {
}

_enumdeclarations = {
}
