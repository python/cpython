"""
Package generated from Macintosh HD:Hulpprogramma's:Disk Copy
Resource aete resid 0
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Special_Events
import Utility_Events


_code_to_module = {
        'Core' : Standard_Suite,
        'ddsk' : Special_Events,
        'ddsk' : Utility_Events,
}



_code_to_fullname = {
        'Core' : ('Disk_Copy.Standard_Suite', 'Standard_Suite'),
        'ddsk' : ('Disk_Copy.Special_Events', 'Special_Events'),
        'ddsk' : ('Disk_Copy.Utility_Events', 'Utility_Events'),
}

from Standard_Suite import *
from Special_Events import *
from Utility_Events import *


class Disk_Copy(Standard_Suite_Events,
                Special_Events_Events,
                Utility_Events_Events,
                aetools.TalkTo):
    _signature = 'ddsk'
