"""Suite Macintosh Connectivity Classes: Classes relating to Apple Macintosh personal computer connectivity
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Extensions/AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'macc'

class Macintosh_Connectivity_Clas_Events:

    pass


class ADB_address(aetools.ComponentItem):
    """ADB address - Addresses a device connected via Apple Desktop Bus """
    want = 'cadb'
class _Prop__3c_inheritance_3e_(aetools.NProperty):
    """<inheritance> - inherits some of its properties from this class """
    which = 'c@#^'
    want = 'cadr'
class _Prop_ID(aetools.NProperty):
    """ID - the Apple Desktop Bus device ID """
    which = 'ID  '
    want = 'shor'

ADB_addresses = ADB_address

class address_specification(aetools.ComponentItem):
    """address specification - Unique designation of a device or service connected to this computer """
    want = 'cadr'
class _Prop_conduit(aetools.NProperty):
    """conduit - How the addressee is physically connected """
    which = 'pcon'
    want = 'econ'
class _Prop_properties(aetools.NProperty):
    """properties - property that allows getting and setting of multiple properties """
    which = 'pALL'
    want = 'reco'
class _Prop_protocol(aetools.NProperty):
    """protocol - How to talk to this addressee """
    which = 'pprt'
    want = 'epro'

address_specifications = address_specification

class AppleTalk_address(aetools.ComponentItem):
    """AppleTalk address - Addresses a device or service connected via the AppleTalk protocol """
    want = 'cat '
class _Prop_AppleTalk_machine(aetools.NProperty):
    """AppleTalk machine - the machine name part of the address """
    which = 'patm'
    want = 'TEXT'
class _Prop_AppleTalk_type(aetools.NProperty):
    """AppleTalk type - the type part of the AppleTalk address """
    which = 'patt'
    want = 'TEXT'
class _Prop_AppleTalk_zone(aetools.NProperty):
    """AppleTalk zone - the zone part of the address """
    which = 'patz'
    want = 'TEXT'

AppleTalk_addresses = AppleTalk_address

class bus_slot(aetools.ComponentItem):
    """bus slot - Addresses a PC, PCI, or NuBus card """
    want = 'cbus'

bus_slots = bus_slot

class device_specification(aetools.ComponentItem):
    """device specification - A device connected to a computer """
    want = 'cdev'
class _Prop_device_address(aetools.NProperty):
    """device address - the address of the device """
    which = 'pdva'
    want = 'cadr'
class _Prop_device_type(aetools.NProperty):
    """device type - the kind of device """
    which = 'pdvt'
    want = 'edvt'

device_specifications = device_specification

class Ethernet_address(aetools.ComponentItem):
    """Ethernet address - Addresses a device by its Ethernet address """
    want = 'cen '

Ethernet_addresses = Ethernet_address

class FireWire_address(aetools.ComponentItem):
    """FireWire address - Addresses a device on the FireWire bus """
    want = 'cfw '

FireWire_addresses = FireWire_address

class IP_address(aetools.ComponentItem):
    """IP address - Addresses a device or service via the Internet Protocol (IP) """
    want = 'cip '
class _Prop_DNS_form(aetools.NProperty):
    """DNS form - the address in the form "apple.com" """
    which = 'pdns'
    want = 'TEXT'
class _Prop_port(aetools.NProperty):
    """port - the port number of the service or client being addressed """
    which = 'ppor'
    want = 'TEXT'

IP_addresses = IP_address

class LocalTalk_address(aetools.ComponentItem):
    """LocalTalk address - Addresses a device by its LocalTalk address """
    want = 'clt '
class _Prop_network(aetools.NProperty):
    """network - the LocalTalk network number """
    which = 'pnet'
    want = 'shor'
class _Prop_node(aetools.NProperty):
    """node - the LocalTalk node number """
    which = 'pnod'
    want = 'shor'
class _Prop_socket(aetools.NProperty):
    """socket - the LocalTalk socket number """
    which = 'psoc'
    want = 'shor'

LocalTalk_addresses = LocalTalk_address

class SCSI_address(aetools.ComponentItem):
    """SCSI address - Addresses a SCSI device """
    want = 'cscs'
class _Prop_LUN(aetools.NProperty):
    """LUN - the SCSI logical unit number """
    which = 'pslu'
    want = 'shor'
class _Prop_SCSI_bus(aetools.NProperty):
    """SCSI bus - the SCSI bus number """
    which = 'pscb'
    want = 'shor'

SCSI_addresses = SCSI_address

class Token_Ring_address(aetools.ComponentItem):
    """Token Ring address - Addresses a device or service via the Token Ring protocol """
    want = 'ctok'

Token_Ring_addresses = Token_Ring_address

class USB_address(aetools.ComponentItem):
    """USB address - Addresses a device on the Universal Serial Bus """
    want = 'cusb'
class _Prop_name(aetools.NProperty):
    """name - the USB device name """
    which = 'pnam'
    want = 'TEXT'

USB_Addresses = USB_address
ADB_address._superclassnames = ['address_specification']
ADB_address._privpropdict = {
    'ID' : _Prop_ID,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
ADB_address._privelemdict = {
}
address_specification._superclassnames = []
address_specification._privpropdict = {
    'conduit' : _Prop_conduit,
    'properties' : _Prop_properties,
    'protocol' : _Prop_protocol,
}
address_specification._privelemdict = {
}
AppleTalk_address._superclassnames = ['address_specification']
AppleTalk_address._privpropdict = {
    'AppleTalk_machine' : _Prop_AppleTalk_machine,
    'AppleTalk_type' : _Prop_AppleTalk_type,
    'AppleTalk_zone' : _Prop_AppleTalk_zone,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
AppleTalk_address._privelemdict = {
}
bus_slot._superclassnames = ['address_specification']
bus_slot._privpropdict = {
    'ID' : _Prop_ID,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
bus_slot._privelemdict = {
}
device_specification._superclassnames = []
device_specification._privpropdict = {
    'device_address' : _Prop_device_address,
    'device_type' : _Prop_device_type,
    'properties' : _Prop_properties,
}
device_specification._privelemdict = {
}
Ethernet_address._superclassnames = ['address_specification']
Ethernet_address._privpropdict = {
    'ID' : _Prop_ID,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
Ethernet_address._privelemdict = {
}
FireWire_address._superclassnames = ['address_specification']
FireWire_address._privpropdict = {
    'ID' : _Prop_ID,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
FireWire_address._privelemdict = {
}
IP_address._superclassnames = ['address_specification']
IP_address._privpropdict = {
    'DNS_form' : _Prop_DNS_form,
    'ID' : _Prop_ID,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
    'port' : _Prop_port,
}
IP_address._privelemdict = {
}
LocalTalk_address._superclassnames = ['address_specification']
LocalTalk_address._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
    'network' : _Prop_network,
    'node' : _Prop_node,
    'socket' : _Prop_socket,
}
LocalTalk_address._privelemdict = {
}
SCSI_address._superclassnames = ['address_specification']
SCSI_address._privpropdict = {
    'ID' : _Prop_ID,
    'LUN' : _Prop_LUN,
    'SCSI_bus' : _Prop_SCSI_bus,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
SCSI_address._privelemdict = {
}
Token_Ring_address._superclassnames = ['address_specification']
Token_Ring_address._privpropdict = {
    'ID' : _Prop_ID,
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
Token_Ring_address._privelemdict = {
}
USB_address._superclassnames = ['address_specification']
USB_address._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
    'name' : _Prop_name,
}
USB_address._privelemdict = {
}
_Enum_econ = {
    'ADB' : 'eadb',     #
    'printer_port' : 'ecpp',    #
    'modem_port' : 'ecmp',      #
    'modem_printer_port' : 'empp',      #
    'LocalTalk' : 'eclt',       #
    'Ethernet' : 'ecen',        #
    'Token_Ring' : 'etok',      #
    'SCSI' : 'ecsc',    #
    'USB' : 'ecus',     #
    'FireWire' : 'ecfw',        #
    'infrared' : 'ecir',        #
    'PC_card' : 'ecpc', #
    'PCI_bus' : 'ecpi', #
    'NuBus' : 'enub',   #
    'PDS_slot' : 'ecpd',        #
    'Comm_slot' : 'eccm',       #
    'monitor_out' : 'ecmn',     #
    'video_out' : 'ecvo',       #
    'video_in' : 'ecvi',        #
    'audio_out' : 'ecao',       #
    'audio_line_in' : 'ecai',   #
    'audio_line_out' : 'ecal',  #
    'microphone' : 'ecmi',      #
}

_Enum_edvt = {
    'hard_disk_drive' : 'ehd ', #
    'floppy_disk_drive' : 'efd ',       #
    'CD_ROM_drive' : 'ecd ',    #
    'DVD_drive' : 'edvd',       #
    'storage_device' : 'edst',  #
    'keyboard' : 'ekbd',        #
    'mouse' : 'emou',   #
    'trackball' : 'etrk',       #
    'trackpad' : 'edtp',        #
    'pointing_device' : 'edpd', #
    'video_monitor' : 'edvm',   #
    'LCD_display' : 'edlc',     #
    'display' : 'edds', #
    'modem' : 'edmm',   #
    'PC_card' : 'ecpc', #
    'PCI_card' : 'edpi',        #
    'NuBus_card' : 'ednb',      #
    'printer' : 'edpr', #
    'speakers' : 'edsp',        #
    'microphone' : 'ecmi',      #
}

_Enum_epro = {
    'serial' : 'epsr',  #
    'AppleTalk' : 'epat',       #
    'IP' : 'epip',      #
    'SCSI' : 'ecsc',    #
    'ADB' : 'eadb',     #
    'FireWire' : 'ecfw',        #
    'IrDA' : 'epir',    #
    'IRTalk' : 'epit',  #
    'USB' : 'ecus',     #
    'PC_card' : 'ecpc', #
    'PCI_bus' : 'ecpi', #
    'NuBus' : 'enub',   #
    'bus' : 'ebus',     #
    'Macintosh_video' : 'epmv', #
    'SVGA' : 'epsg',    #
    'S_video' : 'epsv', #
    'analog_audio' : 'epau',    #
    'digital_audio' : 'epda',   #
    'PostScript' : 'epps',      #
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'cadb' : ADB_address,
    'cadr' : address_specification,
    'cat ' : AppleTalk_address,
    'cbus' : bus_slot,
    'cdev' : device_specification,
    'cen ' : Ethernet_address,
    'cfw ' : FireWire_address,
    'cip ' : IP_address,
    'clt ' : LocalTalk_address,
    'cscs' : SCSI_address,
    'ctok' : Token_Ring_address,
    'cusb' : USB_address,
}

_propdeclarations = {
    'ID  ' : _Prop_ID,
    'c@#^' : _Prop__3c_inheritance_3e_,
    'pALL' : _Prop_properties,
    'patm' : _Prop_AppleTalk_machine,
    'patt' : _Prop_AppleTalk_type,
    'patz' : _Prop_AppleTalk_zone,
    'pcon' : _Prop_conduit,
    'pdns' : _Prop_DNS_form,
    'pdva' : _Prop_device_address,
    'pdvt' : _Prop_device_type,
    'pnam' : _Prop_name,
    'pnet' : _Prop_network,
    'pnod' : _Prop_node,
    'ppor' : _Prop_port,
    'pprt' : _Prop_protocol,
    'pscb' : _Prop_SCSI_bus,
    'pslu' : _Prop_LUN,
    'psoc' : _Prop_socket,
}

_compdeclarations = {
}

_enumdeclarations = {
    'econ' : _Enum_econ,
    'edvt' : _Enum_edvt,
    'epro' : _Enum_epro,
}
