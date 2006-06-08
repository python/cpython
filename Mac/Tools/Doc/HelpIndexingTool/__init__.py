"""
Package generated from /Developer/Applications/Apple Help Indexing Tool.app
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Help_Indexing_Tool_Suite
import odds_and_ends
import Miscellaneous_Standards
import Required_Suite


_code_to_module = {
    'CoRe' : Standard_Suite,
    'HIT ' : Help_Indexing_Tool_Suite,
    'Odds' : odds_and_ends,
    'misc' : Miscellaneous_Standards,
    'reqd' : Required_Suite,
}



_code_to_fullname = {
    'CoRe' : ('HelpIndexingTool.Standard_Suite', 'Standard_Suite'),
    'HIT ' : ('HelpIndexingTool.Help_Indexing_Tool_Suite', 'Help_Indexing_Tool_Suite'),
    'Odds' : ('HelpIndexingTool.odds_and_ends', 'odds_and_ends'),
    'misc' : ('HelpIndexingTool.Miscellaneous_Standards', 'Miscellaneous_Standards'),
    'reqd' : ('HelpIndexingTool.Required_Suite', 'Required_Suite'),
}

from Standard_Suite import *
from Help_Indexing_Tool_Suite import *
from odds_and_ends import *
from Miscellaneous_Standards import *
from Required_Suite import *

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
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(document)
getbaseclasses(application)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'cwin' : window,
    'capp' : application,
    'docu' : document,
    'capp' : application,
}


class HelpIndexingTool(Standard_Suite_Events,
        Help_Indexing_Tool_Suite_Events,
        odds_and_ends_Events,
        Miscellaneous_Standards_Events,
        Required_Suite_Events,
        aetools.TalkTo):
    _signature = 'hiti'

    _moduleName = 'HelpIndexingTool'
