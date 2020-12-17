#!/usr/bin/env python
#
# This is a module that gathers a list of serial ports including details on OSX
#
# code originally from https://github.com/makerbot/pyserial/tree/master/serial/tools
# with contributions from cibomahto, dgs3, FarMcKon, tedbrandston
# and modifications by cliechti, hoihu, hardkrash
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2013-2020
#
# SPDX-License-Identifier:    BSD-3-Clause


# List all of the callout devices in OS/X by querying IOKit.

# See the following for a reference of how to do this:
# http://developer.apple.com/library/mac/#documentation/DeviceDrivers/Conceptual/WorkingWSerial/WWSerial_SerialDevs/SerialDevices.html#//apple_ref/doc/uid/TP30000384-CIHGEAFD

# More help from darwin_hid.py

# Also see the 'IORegistryExplorer' for an idea of what we are actually searching

from __future__ import absolute_import

import ctypes

from serial.tools import list_ports_common

iokit = ctypes.cdll.LoadLibrary('/System/Library/Frameworks/IOKit.framework/IOKit')
cf = ctypes.cdll.LoadLibrary('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation')

# kIOMasterPortDefault is no longer exported in BigSur but no biggie, using NULL works just the same
kIOMasterPortDefault = 0 # WAS: ctypes.c_void_p.in_dll(iokit, "kIOMasterPortDefault")
kCFAllocatorDefault = ctypes.c_void_p.in_dll(cf, "kCFAllocatorDefault")

kCFStringEncodingMacRoman = 0
kCFStringEncodingUTF8 = 0x08000100

# defined in `IOKit/usb/USBSpec.h`
kUSBVendorString = 'USB Vendor Name'
kUSBSerialNumberString = 'USB Serial Number'

# `io_name_t` defined as `typedef char io_name_t[128];`
# in `device/device_types.h`
io_name_size = 128

# defined in `mach/kern_return.h`
KERN_SUCCESS = 0
# kern_return_t defined as `typedef int kern_return_t;` in `mach/i386/kern_return.h`
kern_return_t = ctypes.c_int

iokit.IOServiceMatching.restype = ctypes.c_void_p

iokit.IOServiceGetMatchingServices.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
iokit.IOServiceGetMatchingServices.restype = kern_return_t

iokit.IORegistryEntryGetParentEntry.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
iokit.IOServiceGetMatchingServices.restype = kern_return_t

iokit.IORegistryEntryCreateCFProperty.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32]
iokit.IORegistryEntryCreateCFProperty.restype = ctypes.c_void_p

iokit.IORegistryEntryGetPath.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
iokit.IORegistryEntryGetPath.restype = kern_return_t

iokit.IORegistryEntryGetName.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
iokit.IORegistryEntryGetName.restype = kern_return_t

iokit.IOObjectGetClass.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
iokit.IOObjectGetClass.restype = kern_return_t

iokit.IOObjectRelease.argtypes = [ctypes.c_void_p]


cf.CFStringCreateWithCString.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int32]
cf.CFStringCreateWithCString.restype = ctypes.c_void_p

cf.CFStringGetCStringPtr.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
cf.CFStringGetCStringPtr.restype = ctypes.c_char_p

cf.CFStringGetCString.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_long, ctypes.c_uint32]
cf.CFStringGetCString.restype = ctypes.c_bool

cf.CFNumberGetValue.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p]
cf.CFNumberGetValue.restype = ctypes.c_void_p

# void CFRelease ( CFTypeRef cf );
cf.CFRelease.argtypes = [ctypes.c_void_p]
cf.CFRelease.restype = None

# CFNumber type defines
kCFNumberSInt8Type = 1
kCFNumberSInt16Type = 2
kCFNumberSInt32Type = 3
kCFNumberSInt64Type = 4


def get_string_property(device_type, property):
    """
    Search the given device for the specified string property

    @param device_type Type of Device
    @param property String to search for
    @return Python string containing the value, or None if not found.
    """
    key = cf.CFStringCreateWithCString(
            kCFAllocatorDefault,
            property.encode("utf-8"),
            kCFStringEncodingUTF8)

    CFContainer = iokit.IORegistryEntryCreateCFProperty(
            device_type,
            key,
            kCFAllocatorDefault,
            0)
    output = None

    if CFContainer:
        output = cf.CFStringGetCStringPtr(CFContainer, 0)
        if output is not None:
            output = output.decode('utf-8')
        else:
            buffer = ctypes.create_string_buffer(io_name_size);
            success = cf.CFStringGetCString(CFContainer, ctypes.byref(buffer), io_name_size, kCFStringEncodingUTF8)
            if success:
                output = buffer.value.decode('utf-8')
        cf.CFRelease(CFContainer)
    return output


def get_int_property(device_type, property, cf_number_type):
    """
    Search the given device for the specified string property

    @param device_type Device to search
    @param property String to search for
    @param cf_number_type CFType number

    @return Python string containing the value, or None if not found.
    """
    key = cf.CFStringCreateWithCString(
            kCFAllocatorDefault,
            property.encode("utf-8"),
            kCFStringEncodingUTF8)

    CFContainer = iokit.IORegistryEntryCreateCFProperty(
            device_type,
            key,
            kCFAllocatorDefault,
            0)

    if CFContainer:
        if (cf_number_type == kCFNumberSInt32Type):
            number = ctypes.c_uint32()
        elif (cf_number_type == kCFNumberSInt16Type):
            number = ctypes.c_uint16()
        cf.CFNumberGetValue(CFContainer, cf_number_type, ctypes.byref(number))
        cf.CFRelease(CFContainer)
        return number.value
    return None

def IORegistryEntryGetName(device):
    devicename = ctypes.create_string_buffer(io_name_size);
    res = iokit.IORegistryEntryGetName(device, ctypes.byref(devicename))
    if res != KERN_SUCCESS:
        return None
    # this works in python2 but may not be valid. Also I don't know if
    # this encoding is guaranteed. It may be dependent on system locale.
    return devicename.value.decode('utf-8')

def IOObjectGetClass(device):
    classname = ctypes.create_string_buffer(io_name_size)
    iokit.IOObjectGetClass(device, ctypes.byref(classname))
    return classname.value

def GetParentDeviceByType(device, parent_type):
    """ Find the first parent of a device that implements the parent_type
        @param IOService Service to inspect
        @return Pointer to the parent type, or None if it was not found.
    """
    # First, try to walk up the IOService tree to find a parent of this device that is a IOUSBDevice.
    parent_type = parent_type.encode('utf-8')
    while IOObjectGetClass(device) != parent_type:
        parent = ctypes.c_void_p()
        response = iokit.IORegistryEntryGetParentEntry(
                device,
                "IOService".encode("utf-8"),
                ctypes.byref(parent))
        # If we weren't able to find a parent for the device, we're done.
        if response != KERN_SUCCESS:
            return None
        device = parent
    return device


def GetIOServicesByType(service_type):
    """
    returns iterator over specified service_type
    """
    serial_port_iterator = ctypes.c_void_p()

    iokit.IOServiceGetMatchingServices(
            kIOMasterPortDefault,
            iokit.IOServiceMatching(service_type.encode('utf-8')),
            ctypes.byref(serial_port_iterator))

    services = []
    while iokit.IOIteratorIsValid(serial_port_iterator):
        service = iokit.IOIteratorNext(serial_port_iterator)
        if not service:
            break
        services.append(service)
    iokit.IOObjectRelease(serial_port_iterator)
    return services


def location_to_string(locationID):
    """
    helper to calculate port and bus number from locationID
    """
    loc = ['{}-'.format(locationID >> 24)]
    while locationID & 0xf00000:
        if len(loc) > 1:
            loc.append('.')
        loc.append('{}'.format((locationID >> 20) & 0xf))
        locationID <<= 4
    return ''.join(loc)


class SuitableSerialInterface(object):
    pass


def scan_interfaces():
    """
    helper function to scan USB interfaces
    returns a list of SuitableSerialInterface objects with name and id attributes
    """
    interfaces = []
    for service in GetIOServicesByType('IOSerialBSDClient'):
        device = get_string_property(service, "IOCalloutDevice")
        if device:
            usb_device = GetParentDeviceByType(service, "IOUSBInterface")
            if usb_device:
                name = get_string_property(usb_device, "USB Interface Name") or None
                locationID = get_int_property(usb_device, "locationID", kCFNumberSInt32Type) or ''
                i = SuitableSerialInterface()
                i.id = locationID
                i.name = name
                interfaces.append(i)
    return interfaces


def search_for_locationID_in_interfaces(serial_interfaces, locationID):
    for interface in serial_interfaces:
        if (interface.id == locationID):
            return interface.name
    return None


def comports(include_links=False):
    # XXX include_links is currently ignored. are links in /dev even supported here?
    # Scan for all iokit serial ports
    services = GetIOServicesByType('IOSerialBSDClient')
    ports = []
    serial_interfaces = scan_interfaces()
    for service in services:
        # First, add the callout device file.
        device = get_string_property(service, "IOCalloutDevice")
        if device:
            info = list_ports_common.ListPortInfo(device)
            # If the serial port is implemented by IOUSBDevice
            # NOTE IOUSBDevice was deprecated as of 10.11 and finally on Apple Silicon
            # devices has been completely removed.  Thanks to @oskay for this patch.
            usb_device = GetParentDeviceByType(service, "IOUSBHostDevice")
            if not usb_device:
                usb_device = GetParentDeviceByType(service, "IOUSBDevice")
            if usb_device:
                # fetch some useful informations from properties
                info.vid = get_int_property(usb_device, "idVendor", kCFNumberSInt16Type)
                info.pid = get_int_property(usb_device, "idProduct", kCFNumberSInt16Type)
                info.serial_number = get_string_property(usb_device, kUSBSerialNumberString)
                # We know this is a usb device, so the
                # IORegistryEntryName should always be aliased to the
                # usb product name string descriptor.
                info.product = IORegistryEntryGetName(usb_device) or 'n/a'
                info.manufacturer = get_string_property(usb_device, kUSBVendorString)
                locationID = get_int_property(usb_device, "locationID", kCFNumberSInt32Type)
                info.location = location_to_string(locationID)
                info.interface = search_for_locationID_in_interfaces(serial_interfaces, locationID)
                info.apply_usb_info()
            ports.append(info)
    return ports

# test
if __name__ == '__main__':
    for port, desc, hwid in sorted(comports()):
        print("{}: {} [{}]".format(port, desc, hwid))
