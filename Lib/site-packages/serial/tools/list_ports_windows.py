#! python
#
# Enumerate serial ports on Windows including a human readable description
# and hardware information.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2016 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

from __future__ import absolute_import

# pylint: disable=invalid-name,too-few-public-methods
import re
import ctypes
from ctypes.wintypes import BOOL
from ctypes.wintypes import HWND
from ctypes.wintypes import DWORD
from ctypes.wintypes import WORD
from ctypes.wintypes import LONG
from ctypes.wintypes import ULONG
from ctypes.wintypes import HKEY
from ctypes.wintypes import BYTE
import serial
from serial.win32 import ULONG_PTR
from serial.tools import list_ports_common


def ValidHandle(value, func, arguments):
    if value == 0:
        raise ctypes.WinError()
    return value


NULL = 0
HDEVINFO = ctypes.c_void_p
LPCTSTR = ctypes.c_wchar_p
PCTSTR = ctypes.c_wchar_p
PTSTR = ctypes.c_wchar_p
LPDWORD = PDWORD = ctypes.POINTER(DWORD)
#~ LPBYTE = PBYTE = ctypes.POINTER(BYTE)
LPBYTE = PBYTE = ctypes.c_void_p        # XXX avoids error about types

ACCESS_MASK = DWORD
REGSAM = ACCESS_MASK


class GUID(ctypes.Structure):
    _fields_ = [
        ('Data1', DWORD),
        ('Data2', WORD),
        ('Data3', WORD),
        ('Data4', BYTE * 8),
    ]

    def __str__(self):
        return "{{{:08x}-{:04x}-{:04x}-{}-{}}}".format(
            self.Data1,
            self.Data2,
            self.Data3,
            ''.join(["{:02x}".format(d) for d in self.Data4[:2]]),
            ''.join(["{:02x}".format(d) for d in self.Data4[2:]]),
        )


class SP_DEVINFO_DATA(ctypes.Structure):
    _fields_ = [
        ('cbSize', DWORD),
        ('ClassGuid', GUID),
        ('DevInst', DWORD),
        ('Reserved', ULONG_PTR),
    ]

    def __str__(self):
        return "ClassGuid:{} DevInst:{}".format(self.ClassGuid, self.DevInst)


PSP_DEVINFO_DATA = ctypes.POINTER(SP_DEVINFO_DATA)

PSP_DEVICE_INTERFACE_DETAIL_DATA = ctypes.c_void_p

setupapi = ctypes.windll.LoadLibrary("setupapi")
SetupDiDestroyDeviceInfoList = setupapi.SetupDiDestroyDeviceInfoList
SetupDiDestroyDeviceInfoList.argtypes = [HDEVINFO]
SetupDiDestroyDeviceInfoList.restype = BOOL

SetupDiClassGuidsFromName = setupapi.SetupDiClassGuidsFromNameW
SetupDiClassGuidsFromName.argtypes = [PCTSTR, ctypes.POINTER(GUID), DWORD, PDWORD]
SetupDiClassGuidsFromName.restype = BOOL

SetupDiEnumDeviceInfo = setupapi.SetupDiEnumDeviceInfo
SetupDiEnumDeviceInfo.argtypes = [HDEVINFO, DWORD, PSP_DEVINFO_DATA]
SetupDiEnumDeviceInfo.restype = BOOL

SetupDiGetClassDevs = setupapi.SetupDiGetClassDevsW
SetupDiGetClassDevs.argtypes = [ctypes.POINTER(GUID), PCTSTR, HWND, DWORD]
SetupDiGetClassDevs.restype = HDEVINFO
SetupDiGetClassDevs.errcheck = ValidHandle

SetupDiGetDeviceRegistryProperty = setupapi.SetupDiGetDeviceRegistryPropertyW
SetupDiGetDeviceRegistryProperty.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD]
SetupDiGetDeviceRegistryProperty.restype = BOOL

SetupDiGetDeviceInstanceId = setupapi.SetupDiGetDeviceInstanceIdW
SetupDiGetDeviceInstanceId.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, PTSTR, DWORD, PDWORD]
SetupDiGetDeviceInstanceId.restype = BOOL

SetupDiOpenDevRegKey = setupapi.SetupDiOpenDevRegKey
SetupDiOpenDevRegKey.argtypes = [HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM]
SetupDiOpenDevRegKey.restype = HKEY

advapi32 = ctypes.windll.LoadLibrary("Advapi32")
RegCloseKey = advapi32.RegCloseKey
RegCloseKey.argtypes = [HKEY]
RegCloseKey.restype = LONG

RegQueryValueEx = advapi32.RegQueryValueExW
RegQueryValueEx.argtypes = [HKEY, LPCTSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD]
RegQueryValueEx.restype = LONG

cfgmgr32 = ctypes.windll.LoadLibrary("Cfgmgr32")
CM_Get_Parent = cfgmgr32.CM_Get_Parent
CM_Get_Parent.argtypes = [PDWORD, DWORD, ULONG]
CM_Get_Parent.restype = LONG

CM_Get_Device_IDW = cfgmgr32.CM_Get_Device_IDW
CM_Get_Device_IDW.argtypes = [DWORD, PTSTR, ULONG, ULONG]
CM_Get_Device_IDW.restype = LONG

CM_MapCrToWin32Err = cfgmgr32.CM_MapCrToWin32Err
CM_MapCrToWin32Err.argtypes = [DWORD, DWORD]
CM_MapCrToWin32Err.restype = DWORD


DIGCF_PRESENT = 2
DIGCF_DEVICEINTERFACE = 16
INVALID_HANDLE_VALUE = 0
ERROR_INSUFFICIENT_BUFFER = 122
ERROR_NOT_FOUND = 1168
SPDRP_HARDWAREID = 1
SPDRP_FRIENDLYNAME = 12
SPDRP_LOCATION_PATHS = 35
SPDRP_MFG = 11
DICS_FLAG_GLOBAL = 1
DIREG_DEV = 0x00000001
KEY_READ = 0x20019


MAX_USB_DEVICE_TREE_TRAVERSAL_DEPTH = 5


def get_parent_serial_number(child_devinst, child_vid, child_pid, depth=0, last_serial_number=None):
    """ Get the serial number of the parent of a device.

    Args:
        child_devinst: The device instance handle to get the parent serial number of.
        child_vid: The vendor ID of the child device.
        child_pid: The product ID of the child device.
        depth: The current iteration depth of the USB device tree.
    """

    # If the traversal depth is beyond the max, abandon attempting to find the serial number.
    if depth > MAX_USB_DEVICE_TREE_TRAVERSAL_DEPTH:
        return '' if not last_serial_number else last_serial_number

    # Get the parent device instance.
    devinst = DWORD()
    ret = CM_Get_Parent(ctypes.byref(devinst), child_devinst, 0)

    if ret:
        win_error = CM_MapCrToWin32Err(DWORD(ret), DWORD(0))

        # If there is no parent available, the child was the root device. We cannot traverse
        # further.
        if win_error == ERROR_NOT_FOUND:
            return '' if not last_serial_number else last_serial_number

        raise ctypes.WinError(win_error)

    # Get the ID of the parent device and parse it for vendor ID, product ID, and serial number.
    parentHardwareID = ctypes.create_unicode_buffer(250)

    ret = CM_Get_Device_IDW(
        devinst,
        parentHardwareID,
        ctypes.sizeof(parentHardwareID) - 1,
        0)

    if ret:
        raise ctypes.WinError(CM_MapCrToWin32Err(DWORD(ret), DWORD(0)))

    parentHardwareID_str = parentHardwareID.value
    m = re.search(r'VID_([0-9a-f]{4})(&PID_([0-9a-f]{4}))?(&MI_(\d{2}))?(\\(.*))?',
                  parentHardwareID_str,
                  re.I)

    # return early if we have no matches (likely malformed serial, traversed too far)
    if not m:
        return '' if not last_serial_number else last_serial_number

    vid = None
    pid = None
    serial_number = None
    if m.group(1):
        vid = int(m.group(1), 16)
    if m.group(3):
        pid = int(m.group(3), 16)
    if m.group(7):
        serial_number = m.group(7)

    # store what we found as a fallback for malformed serial values up the chain
    found_serial_number = serial_number

    # Check that the USB serial number only contains alpha-numeric characters. It may be a windows
    # device ID (ephemeral ID).
    if serial_number and not re.match(r'^\w+$', serial_number):
        serial_number = None

    if not vid or not pid:
        # If pid and vid are not available at this device level, continue to the parent.
        return get_parent_serial_number(devinst, child_vid, child_pid, depth + 1, found_serial_number)

    if pid != child_pid or vid != child_vid:
        # If the VID or PID has changed, we are no longer looking at the same physical device. The
        # serial number is unknown.
        return '' if not last_serial_number else last_serial_number

    # In this case, the vid and pid of the parent device are identical to the child. However, if
    # there still isn't a serial number available, continue to the next parent.
    if not serial_number:
        return get_parent_serial_number(devinst, child_vid, child_pid, depth + 1, found_serial_number)

    # Finally, the VID and PID are identical to the child and a serial number is present, so return
    # it.
    return serial_number


def iterate_comports():
    """Return a generator that yields descriptions for serial ports"""
    PortsGUIDs = (GUID * 8)()  # so far only seen one used, so hope 8 are enough...
    ports_guids_size = DWORD()
    if not SetupDiClassGuidsFromName(
            "Ports",
            PortsGUIDs,
            ctypes.sizeof(PortsGUIDs),
            ctypes.byref(ports_guids_size)):
        raise ctypes.WinError()

    ModemsGUIDs = (GUID * 8)()  # so far only seen one used, so hope 8 are enough...
    modems_guids_size = DWORD()
    if not SetupDiClassGuidsFromName(
            "Modem",
            ModemsGUIDs,
            ctypes.sizeof(ModemsGUIDs),
            ctypes.byref(modems_guids_size)):
        raise ctypes.WinError()

    GUIDs = PortsGUIDs[:ports_guids_size.value] + ModemsGUIDs[:modems_guids_size.value]

    # repeat for all possible GUIDs
    for index in range(len(GUIDs)):
        bInterfaceNumber = None
        g_hdi = SetupDiGetClassDevs(
            ctypes.byref(GUIDs[index]),
            None,
            NULL,
            DIGCF_PRESENT)  # was DIGCF_PRESENT|DIGCF_DEVICEINTERFACE which misses CDC ports

        devinfo = SP_DEVINFO_DATA()
        devinfo.cbSize = ctypes.sizeof(devinfo)
        index = 0
        while SetupDiEnumDeviceInfo(g_hdi, index, ctypes.byref(devinfo)):
            index += 1

            # get the real com port name
            hkey = SetupDiOpenDevRegKey(
                g_hdi,
                ctypes.byref(devinfo),
                DICS_FLAG_GLOBAL,
                0,
                DIREG_DEV,  # DIREG_DRV for SW info
                KEY_READ)
            port_name_buffer = ctypes.create_unicode_buffer(250)
            port_name_length = ULONG(ctypes.sizeof(port_name_buffer))
            RegQueryValueEx(
                hkey,
                "PortName",
                None,
                None,
                ctypes.byref(port_name_buffer),
                ctypes.byref(port_name_length))
            RegCloseKey(hkey)

            # unfortunately does this method also include parallel ports.
            # we could check for names starting with COM or just exclude LPT
            # and hope that other "unknown" names are serial ports...
            if port_name_buffer.value.startswith('LPT'):
                continue

            # hardware ID
            szHardwareID = ctypes.create_unicode_buffer(250)
            # try to get ID that includes serial number
            if not SetupDiGetDeviceInstanceId(
                    g_hdi,
                    ctypes.byref(devinfo),
                    #~ ctypes.byref(szHardwareID),
                    szHardwareID,
                    ctypes.sizeof(szHardwareID) - 1,
                    None):
                # fall back to more generic hardware ID if that would fail
                if not SetupDiGetDeviceRegistryProperty(
                        g_hdi,
                        ctypes.byref(devinfo),
                        SPDRP_HARDWAREID,
                        None,
                        ctypes.byref(szHardwareID),
                        ctypes.sizeof(szHardwareID) - 1,
                        None):
                    # Ignore ERROR_INSUFFICIENT_BUFFER
                    if ctypes.GetLastError() != ERROR_INSUFFICIENT_BUFFER:
                        raise ctypes.WinError()
            # stringify
            szHardwareID_str = szHardwareID.value

            info = list_ports_common.ListPortInfo(port_name_buffer.value, skip_link_detection=True)

            # in case of USB, make a more readable string, similar to that form
            # that we also generate on other platforms
            if szHardwareID_str.startswith('USB'):
                m = re.search(r'VID_([0-9a-f]{4})(&PID_([0-9a-f]{4}))?(&MI_(\d{2}))?(\\(.*))?', szHardwareID_str, re.I)
                if m:
                    info.vid = int(m.group(1), 16)
                    if m.group(3):
                        info.pid = int(m.group(3), 16)
                    if m.group(5):
                        bInterfaceNumber = int(m.group(5))

                    # Check that the USB serial number only contains alpha-numeric characters. It
                    # may be a windows device ID (ephemeral ID) for composite devices.
                    if m.group(7) and re.match(r'^\w+$', m.group(7)):
                        info.serial_number = m.group(7)
                    else:
                        info.serial_number = get_parent_serial_number(devinfo.DevInst, info.vid, info.pid)

                # calculate a location string
                loc_path_str = ctypes.create_unicode_buffer(250)
                if SetupDiGetDeviceRegistryProperty(
                        g_hdi,
                        ctypes.byref(devinfo),
                        SPDRP_LOCATION_PATHS,
                        None,
                        ctypes.byref(loc_path_str),
                        ctypes.sizeof(loc_path_str) - 1,
                        None):
                    m = re.finditer(r'USBROOT\((\w+)\)|#USB\((\w+)\)', loc_path_str.value)
                    location = []
                    for g in m:
                        if g.group(1):
                            location.append('{:d}'.format(int(g.group(1)) + 1))
                        else:
                            if len(location) > 1:
                                location.append('.')
                            else:
                                location.append('-')
                            location.append(g.group(2))
                    if bInterfaceNumber is not None:
                        location.append(':{}.{}'.format(
                            'x',  # XXX how to determine correct bConfigurationValue?
                            bInterfaceNumber))
                    if location:
                        info.location = ''.join(location)
                info.hwid = info.usb_info()
            elif szHardwareID_str.startswith('FTDIBUS'):
                m = re.search(r'VID_([0-9a-f]{4})\+PID_([0-9a-f]{4})(\+(\w+))?', szHardwareID_str, re.I)
                if m:
                    info.vid = int(m.group(1), 16)
                    info.pid = int(m.group(2), 16)
                    if m.group(4):
                        info.serial_number = m.group(4)
                # USB location is hidden by FDTI driver :(
                info.hwid = info.usb_info()
            else:
                info.hwid = szHardwareID_str

            # friendly name
            szFriendlyName = ctypes.create_unicode_buffer(250)
            if SetupDiGetDeviceRegistryProperty(
                    g_hdi,
                    ctypes.byref(devinfo),
                    SPDRP_FRIENDLYNAME,
                    #~ SPDRP_DEVICEDESC,
                    None,
                    ctypes.byref(szFriendlyName),
                    ctypes.sizeof(szFriendlyName) - 1,
                    None):
                info.description = szFriendlyName.value
            #~ else:
                # Ignore ERROR_INSUFFICIENT_BUFFER
                #~ if ctypes.GetLastError() != ERROR_INSUFFICIENT_BUFFER:
                    #~ raise IOError("failed to get details for %s (%s)" % (devinfo, szHardwareID.value))
                # ignore errors and still include the port in the list, friendly name will be same as port name

            # manufacturer
            szManufacturer = ctypes.create_unicode_buffer(250)
            if SetupDiGetDeviceRegistryProperty(
                    g_hdi,
                    ctypes.byref(devinfo),
                    SPDRP_MFG,
                    #~ SPDRP_DEVICEDESC,
                    None,
                    ctypes.byref(szManufacturer),
                    ctypes.sizeof(szManufacturer) - 1,
                    None):
                info.manufacturer = szManufacturer.value
            yield info
        SetupDiDestroyDeviceInfoList(g_hdi)


def comports(include_links=False):
    """Return a list of info objects about serial ports"""
    return list(iterate_comports())

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# test
if __name__ == '__main__':
    for port, desc, hwid in sorted(comports()):
        print("{}: {} [{}]".format(port, desc, hwid))
