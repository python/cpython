"""
Package generated from /Applications/Utilities/Terminal.app
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Text_Suite
import Terminal_Suite


_code_to_module = {
    '????' : Standard_Suite,
    '????' : Text_Suite,
    'trmx' : Terminal_Suite,
}



_code_to_fullname = {
    '????' : ('Terminal.Standard_Suite', 'Standard_Suite'),
    '????' : ('Terminal.Text_Suite', 'Text_Suite'),
    'trmx' : ('Terminal.Terminal_Suite', 'Terminal_Suite'),
}

from Standard_Suite import *
from Text_Suite import *
from Terminal_Suite import *

def getbaseclasses(v):
    if not getattr(v, '_propdict', None):
        v._propdict = {}
        v._elemdict = {}
        for superclassname in getattr(v, '_superclassnames', []):
            superclass = eval(superclassname)
            getbaseclasses(superclass)
            v._propdict.update(getattr(superclass, '_propdict', {}))
            v._elemdict.update(getattr(superclass, '_elemdict', {}))
        v._propdict.update(getattr(v, '_privpropdict', {}))
        v._elemdict.update(getattr(v, '_privelemdict', {}))

import StdSuites

#
# Set property and element dictionaries now that all classes have been defined
#
getbaseclasses(color)
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(item)
getbaseclasses(document)
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(character)
getbaseclasses(attachment)
getbaseclasses(paragraph)
getbaseclasses(word)
getbaseclasses(attribute_run)
getbaseclasses(text)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'colr' : color,
    'cwin' : window,
    'capp' : application,
    'cobj' : item,
    'docu' : document,
    'cwin' : window,
    'capp' : application,
    'cha ' : character,
    'atts' : attachment,
    'cpar' : paragraph,
    'cwor' : word,
    'catr' : attribute_run,
    'ctxt' : text,
}


class Terminal(Standard_Suite_Events,
        Text_Suite_Events,
        Terminal_Suite_Events,
        aetools.TalkTo):
    _signature = 'trmx'

    _moduleName = 'Terminal'

    _elemdict = application._elemdict
    _propdict = application._propdict
