"""
Package generated from /System/Library/CoreServices/System Events.app
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Text_Suite
import Disk_Folder_File_Suite
import Folder_Actions_Suite
import Login_Items_Suite
import Power_Suite
import Processes_Suite
import System_Events_Suite


_code_to_module = {
    '????' : Standard_Suite,
    '????' : Text_Suite,
    'cdis' : Disk_Folder_File_Suite,
    'faco' : Folder_Actions_Suite,
    'logi' : Login_Items_Suite,
    'powr' : Power_Suite,
    'prcs' : Processes_Suite,
    'sevs' : System_Events_Suite,
}



_code_to_fullname = {
    '????' : ('SystemEvents.Standard_Suite', 'Standard_Suite'),
    '????' : ('SystemEvents.Text_Suite', 'Text_Suite'),
    'cdis' : ('SystemEvents.Disk_Folder_File_Suite', 'Disk_Folder_File_Suite'),
    'faco' : ('SystemEvents.Folder_Actions_Suite', 'Folder_Actions_Suite'),
    'logi' : ('SystemEvents.Login_Items_Suite', 'Login_Items_Suite'),
    'powr' : ('SystemEvents.Power_Suite', 'Power_Suite'),
    'prcs' : ('SystemEvents.Processes_Suite', 'Processes_Suite'),
    'sevs' : ('SystemEvents.System_Events_Suite', 'System_Events_Suite'),
}

from Standard_Suite import *
from Text_Suite import *
from Disk_Folder_File_Suite import *
from Folder_Actions_Suite import *
from Login_Items_Suite import *
from Power_Suite import *
from Processes_Suite import *
from System_Events_Suite import *

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
getbaseclasses(login_item)
getbaseclasses(color)
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(item)
getbaseclasses(document)
getbaseclasses(character)
getbaseclasses(attachment)
getbaseclasses(paragraph)
getbaseclasses(word)
getbaseclasses(attribute_run)
getbaseclasses(text)
getbaseclasses(file)
getbaseclasses(application)
getbaseclasses(item)
getbaseclasses(folder)
getbaseclasses(disk)
getbaseclasses(script)
getbaseclasses(application)
getbaseclasses(folder_action)
getbaseclasses(application)
getbaseclasses(application)
getbaseclasses(process)
getbaseclasses(application_process)
getbaseclasses(desk_accessory_process)
getbaseclasses(application)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'logi' : login_item,
    'colr' : color,
    'cwin' : window,
    'capp' : application,
    'cobj' : item,
    'docu' : document,
    'cha ' : character,
    'atts' : attachment,
    'cpar' : paragraph,
    'cwor' : word,
    'catr' : attribute_run,
    'ctxt' : text,
    'file' : file,
    'capp' : application,
    'cobj' : item,
    'cfol' : folder,
    'cdis' : disk,
    'scpt' : script,
    'capp' : application,
    'foac' : folder_action,
    'capp' : application,
    'capp' : application,
    'prcs' : process,
    'pcap' : application_process,
    'pcda' : desk_accessory_process,
    'capp' : application,
}


class SystemEvents(Standard_Suite_Events,
        Text_Suite_Events,
        Disk_Folder_File_Suite_Events,
        Folder_Actions_Suite_Events,
        Login_Items_Suite_Events,
        Power_Suite_Events,
        Processes_Suite_Events,
        System_Events_Suite_Events,
        aetools.TalkTo):
    _signature = 'sevs'

    _moduleName = 'SystemEvents'

    _elemdict = application._elemdict
    _propdict = application._propdict
