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
class _Prop_rotation(aetools.NProperty):
    """rotation - the default rotation for objects in the drawing area """
    which = 'prot'
    want = 'trot'
class _Prop_scale(aetools.NProperty):
    """scale - the default scaling for objects in the drawing area """
    which = 'pscl'
    want = 'fixd'
class _Prop_translation(aetools.NProperty):
    """translation - the default repositioning for objects in the drawing area """
    which = 'ptrs'
    want = 'QDpt'

drawing_areas = drawing_area

class graphic_groups(aetools.ComponentItem):
    """graphic groups -  """
    want = 'cpic'

graphic_group = graphic_groups
drawing_area._superclassnames = []
drawing_area._privpropdict = {
    'rotation' : _Prop_rotation,
    'scale' : _Prop_scale,
    'translation' : _Prop_translation,
}
drawing_area._privelemdict = {
}
graphic_groups._superclassnames = []
graphic_groups._privpropdict = {
}
graphic_groups._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'cdrw' : drawing_area,
    'cpic' : graphic_groups,
}

_propdeclarations = {
    'prot' : _Prop_rotation,
    'pscl' : _Prop_scale,
    'ptrs' : _Prop_translation,
}

_compdeclarations = {
}

_enumdeclarations = {
}
