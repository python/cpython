"""Suite Macintosh Connectivity Classes: Classes relating to Apple Macintosh personal computer connectivity
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Extensies:AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'macc'

class Macintosh_Connectivity_Clas_Events:

	pass


class device_specification(aetools.ComponentItem):
	"""device specification - A device connected to a computer """
	want = 'cdev'
class properties(aetools.NProperty):
	"""properties - property that allows getting and setting of multiple properties """
	which = 'pALL'
	want = 'reco'
class device_type(aetools.NProperty):
	"""device type - the kind of device """
	which = 'pdvt'
	want = 'edvt'
class device_address(aetools.NProperty):
	"""device address - the address of the device """
	which = 'pdva'
	want = 'cadr'

device_specifications = device_specification

class address_specification(aetools.ComponentItem):
	"""address specification - Unique designation of a device or service connected to this computer """
	want = 'cadr'
# repeated property properties property that allows getting and setting of multiple properties
class conduit(aetools.NProperty):
	"""conduit - How the addressee is physically connected """
	which = 'pcon'
	want = 'econ'
class protocol(aetools.NProperty):
	"""protocol - How to talk to this addressee """
	which = 'pprt'
	want = 'epro'

address_specifications = address_specification

class ADB_address(aetools.ComponentItem):
	"""ADB address - Addresses a device connected via Apple Desktop Bus """
	want = 'cadb'
class _3c_inheritance_3e_(aetools.NProperty):
	"""<inheritance> - inherits some of its properties from this class """
	which = 'c@#^'
	want = 'cadr'
class ID(aetools.NProperty):
	"""ID - the Apple Desktop Bus device ID """
	which = 'ID  '
	want = 'shor'

ADB_addresses = ADB_address

class AppleTalk_address(aetools.ComponentItem):
	"""AppleTalk address - Addresses a device or service connected via the AppleTalk protocol """
	want = 'cat '
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
class AppleTalk_machine(aetools.NProperty):
	"""AppleTalk machine - the machine name part of the address """
	which = 'patm'
	want = 'TEXT'
class AppleTalk_zone(aetools.NProperty):
	"""AppleTalk zone - the zone part of the address """
	which = 'patz'
	want = 'TEXT'
class AppleTalk_type(aetools.NProperty):
	"""AppleTalk type - the type part of the AppleTalk address """
	which = 'patt'
	want = 'TEXT'

AppleTalk_addresses = AppleTalk_address

class bus_slot(aetools.ComponentItem):
	"""bus slot - Addresses a PC, PCI, or NuBus card """
	want = 'cbus'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
# repeated property ID the slot number

bus_slots = bus_slot

class Ethernet_address(aetools.ComponentItem):
	"""Ethernet address - Addresses a device by its Ethernet address """
	want = 'cen '
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
# repeated property ID the Ethernet address

Ethernet_addresses = Ethernet_address

class FireWire_address(aetools.ComponentItem):
	"""FireWire address - Addresses a device on the FireWire bus """
	want = 'cfw '
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
# repeated property ID the FireWire device ID

FireWire_addresses = FireWire_address

class IP_address(aetools.ComponentItem):
	"""IP address - Addresses a device or service via the Internet Protocol (IP) """
	want = 'cip '
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
# repeated property ID the address in the form "127.201.0.1"
class DNS_form(aetools.NProperty):
	"""DNS form - the address in the form "apple.com" """
	which = 'pdns'
	want = 'TEXT'
class port(aetools.NProperty):
	"""port - the port number of the service or client being addressed """
	which = 'ppor'
	want = 'TEXT'

IP_addresses = IP_address

class LocalTalk_address(aetools.ComponentItem):
	"""LocalTalk address - Addresses a device by its LocalTalk address """
	want = 'clt '
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
class network(aetools.NProperty):
	"""network - the LocalTalk network number """
	which = 'pnet'
	want = 'shor'
class node(aetools.NProperty):
	"""node - the LocalTalk node number """
	which = 'pnod'
	want = 'shor'
class socket(aetools.NProperty):
	"""socket - the LocalTalk socket number """
	which = 'psoc'
	want = 'shor'

LocalTalk_addresses = LocalTalk_address

class SCSI_address(aetools.ComponentItem):
	"""SCSI address - Addresses a SCSI device """
	want = 'cscs'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
class SCSI_bus(aetools.NProperty):
	"""SCSI bus - the SCSI bus number """
	which = 'pscb'
	want = 'shor'
# repeated property ID the SCSI ID
class LUN(aetools.NProperty):
	"""LUN - the SCSI logical unit number """
	which = 'pslu'
	want = 'shor'

SCSI_addresses = SCSI_address

class Token_Ring_address(aetools.ComponentItem):
	"""Token Ring address - Addresses a device or service via the Token Ring protocol """
	want = 'ctok'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
# repeated property ID the Token Ring ID

Token_Ring_addresses = Token_Ring_address

class USB_address(aetools.ComponentItem):
	"""USB address - Addresses a device on the Universal Serial Bus """
	want = 'cusb'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
class name(aetools.NProperty):
	"""name - the USB device name """
	which = 'pnam'
	want = 'TEXT'

USB_Addresses = USB_address
device_specification._propdict = {
	'properties' : properties,
	'device_type' : device_type,
	'device_address' : device_address,
}
device_specification._elemdict = {
}
address_specification._propdict = {
	'properties' : properties,
	'conduit' : conduit,
	'protocol' : protocol,
}
address_specification._elemdict = {
}
ADB_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'ID' : ID,
}
ADB_address._elemdict = {
}
AppleTalk_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'AppleTalk_machine' : AppleTalk_machine,
	'AppleTalk_zone' : AppleTalk_zone,
	'AppleTalk_type' : AppleTalk_type,
}
AppleTalk_address._elemdict = {
}
bus_slot._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'ID' : ID,
}
bus_slot._elemdict = {
}
Ethernet_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'ID' : ID,
}
Ethernet_address._elemdict = {
}
FireWire_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'ID' : ID,
}
FireWire_address._elemdict = {
}
IP_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'ID' : ID,
	'DNS_form' : DNS_form,
	'port' : port,
}
IP_address._elemdict = {
}
LocalTalk_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'network' : network,
	'node' : node,
	'socket' : socket,
}
LocalTalk_address._elemdict = {
}
SCSI_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'SCSI_bus' : SCSI_bus,
	'ID' : ID,
	'LUN' : LUN,
}
SCSI_address._elemdict = {
}
Token_Ring_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'ID' : ID,
}
Token_Ring_address._elemdict = {
}
USB_address._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'name' : name,
}
USB_address._elemdict = {
}
_Enum_edvt = {
	'hard_disk_drive' : 'ehd ',	# 
	'floppy_disk_drive' : 'efd ',	# 
	'CD_ROM_drive' : 'ecd ',	# 
	'DVD_drive' : 'edvd',	# 
	'storage_device' : 'edst',	# 
	'keyboard' : 'ekbd',	# 
	'mouse' : 'emou',	# 
	'trackball' : 'etrk',	# 
	'trackpad' : 'edtp',	# 
	'pointing_device' : 'edpd',	# 
	'video_monitor' : 'edvm',	# 
	'LCD_display' : 'edlc',	# 
	'display' : 'edds',	# 
	'modem' : 'edmm',	# 
	'PC_card' : 'ecpc',	# 
	'PCI_card' : 'edpi',	# 
	'NuBus_card' : 'ednb',	# 
	'printer' : 'edpr',	# 
	'speakers' : 'edsp',	# 
	'microphone' : 'ecmi',	# 
}

_Enum_econ = {
	'ADB' : 'eadb',	# 
	'printer_port' : 'ecpp',	# 
	'modem_port' : 'ecmp',	# 
	'modem_printer_port' : 'empp',	# 
	'LocalTalk' : 'eclt',	# 
	'Ethernet' : 'ecen',	# 
	'Token_Ring' : 'etok',	# 
	'SCSI' : 'ecsc',	# 
	'USB' : 'ecus',	# 
	'FireWire' : 'ecfw',	# 
	'infrared' : 'ecir',	# 
	'PC_card' : 'ecpc',	# 
	'PCI_bus' : 'ecpi',	# 
	'NuBus' : 'enub',	# 
	'PDS_slot' : 'ecpd',	# 
	'Comm_slot' : 'eccm',	# 
	'monitor_out' : 'ecmn',	# 
	'video_out' : 'ecvo',	# 
	'video_in' : 'ecvi',	# 
	'audio_out' : 'ecao',	# 
	'audio_line_in' : 'ecai',	# 
	'audio_line_out' : 'ecal',	# 
	'microphone' : 'ecmi',	# 
}

_Enum_epro = {
	'serial' : 'epsr',	# 
	'AppleTalk' : 'epat',	# 
	'IP' : 'epip',	# 
	'SCSI' : 'ecsc',	# 
	'ADB' : 'eadb',	# 
	'FireWire' : 'ecfw',	# 
	'IrDA' : 'epir',	# 
	'IRTalk' : 'epit',	# 
	'USB' : 'ecus',	# 
	'PC_card' : 'ecpc',	# 
	'PCI_bus' : 'ecpi',	# 
	'NuBus' : 'enub',	# 
	'bus' : 'ebus',	# 
	'Macintosh_video' : 'epmv',	# 
	'SVGA' : 'epsg',	# 
	'S_video' : 'epsv',	# 
	'analog_audio' : 'epau',	# 
	'digital_audio' : 'epda',	# 
	'PostScript' : 'epps',	# 
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'cen ' : Ethernet_address,
	'clt ' : LocalTalk_address,
	'cip ' : IP_address,
	'cusb' : USB_address,
	'cadb' : ADB_address,
	'cscs' : SCSI_address,
	'cbus' : bus_slot,
	'cdev' : device_specification,
	'ctok' : Token_Ring_address,
	'cfw ' : FireWire_address,
	'cadr' : address_specification,
	'cat ' : AppleTalk_address,
}

_propdeclarations = {
	'pnod' : node,
	'pslu' : LUN,
	'patm' : AppleTalk_machine,
	'pdva' : device_address,
	'pscb' : SCSI_bus,
	'ppor' : port,
	'pALL' : properties,
	'ID  ' : ID,
	'c@#^' : _3c_inheritance_3e_,
	'pdvt' : device_type,
	'pnet' : network,
	'patz' : AppleTalk_zone,
	'pnam' : name,
	'pcon' : conduit,
	'pprt' : protocol,
	'patt' : AppleTalk_type,
	'psoc' : socket,
	'pdns' : DNS_form,
}

_compdeclarations = {
}

_enumdeclarations = {
	'econ' : _Enum_econ,
	'epro' : _Enum_epro,
	'edvt' : _Enum_edvt,
}
