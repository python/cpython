"""
Package generated from /Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Standard_URL_suite
import Mozilla_suite
import Text
import WorldWideWeb_suite
import PowerPlant
import Required_suite


_code_to_module = {
    'CoRe' : Standard_Suite,
    'GURL' : Standard_URL_suite,
    'MOSS' : Mozilla_suite,
    'TEXT' : Text,
    'WWW!' : WorldWideWeb_suite,
    'ppnt' : PowerPlant,
    'reqd' : Required_suite,
}



_code_to_fullname = {
    'CoRe' : ('Netscape.Standard_Suite', 'Standard_Suite'),
    'GURL' : ('Netscape.Standard_URL_suite', 'Standard_URL_suite'),
    'MOSS' : ('Netscape.Mozilla_suite', 'Mozilla_suite'),
    'TEXT' : ('Netscape.Text', 'Text'),
    'WWW!' : ('Netscape.WorldWideWeb_suite', 'WorldWideWeb_suite'),
    'ppnt' : ('Netscape.PowerPlant', 'PowerPlant'),
    'reqd' : ('Netscape.Required_suite', 'Required_suite'),
}

from Standard_Suite import *
from Standard_URL_suite import *
from Mozilla_suite import *
from Text import *
from WorldWideWeb_suite import *
from PowerPlant import *
from Required_suite import *

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
getbaseclasses(text)
getbaseclasses(styleset)
getbaseclasses(StdSuites.Text_Suite.character)
getbaseclasses(StdSuites.Text_Suite.text_flow)
getbaseclasses(StdSuites.Text_Suite.word)
getbaseclasses(StdSuites.Text_Suite.paragraph)
getbaseclasses(StdSuites.Text_Suite.text_style_info)
getbaseclasses(StdSuites.Text_Suite.line)
getbaseclasses(StdSuites.Text_Suite.text)
getbaseclasses(window)
getbaseclasses(application)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'ctxt' : text,
    'stys' : styleset,
    'cha ' : StdSuites.Text_Suite.character,
    'cflo' : StdSuites.Text_Suite.text_flow,
    'cwor' : StdSuites.Text_Suite.word,
    'cpar' : StdSuites.Text_Suite.paragraph,
    'tsty' : StdSuites.Text_Suite.text_style_info,
    'clin' : StdSuites.Text_Suite.line,
    'ctxt' : StdSuites.Text_Suite.text,
    'cwin' : window,
    'capp' : application,
}


class Netscape(Standard_Suite_Events,
        Standard_URL_suite_Events,
        Mozilla_suite_Events,
        Text_Events,
        WorldWideWeb_suite_Events,
        PowerPlant_Events,
        Required_suite_Events,
        aetools.TalkTo):
    _signature = 'MOSS'

    _moduleName = 'Netscape'

    _elemdict = application._elemdict
    _propdict = application._propdict
