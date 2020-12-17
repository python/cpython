#!/usr/bin/env python
#
# This is a module that gathers a list of serial ports on POSIXy systems.
# For some specific implementations, see also list_ports_linux, list_ports_osx
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2011-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

"""\
The ``comports`` function is expected to return an iterable that yields tuples
of 3 strings: port name, human readable description and a hardware ID.

As currently no method is known to get the second two strings easily, they are
currently just identical to the port name.
"""

from __future__ import absolute_import

import glob
import sys
import os
from serial.tools import list_ports_common

# try to detect the OS so that a device can be selected...
plat = sys.platform.lower()

if plat[:5] == 'linux':    # Linux (confirmed)  # noqa
    from serial.tools.list_ports_linux import comports

elif plat[:6] == 'darwin':   # OS X (confirmed)
    from serial.tools.list_ports_osx import comports

elif plat == 'cygwin':       # cygwin/win32
    # cygwin accepts /dev/com* in many contexts
    # (such as 'open' call, explicit 'ls'), but 'glob.glob'
    # and bare 'ls' do not; so use /dev/ttyS* instead
    def comports(include_links=False):
        devices = glob.glob('/dev/ttyS*')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:7] == 'openbsd':    # OpenBSD
    def comports(include_links=False):
        devices = glob.glob('/dev/cua*')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:3] == 'bsd' or plat[:7] == 'freebsd':
    def comports(include_links=False):
        devices = glob.glob('/dev/cua*[!.init][!.lock]')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:6] == 'netbsd':   # NetBSD
    def comports(include_links=False):
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/dty*')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:4] == 'irix':     # IRIX
    def comports(include_links=False):
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/ttyf*')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:2] == 'hp':       # HP-UX (not tested)
    def comports(include_links=False):
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/tty*p0')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:5] == 'sunos':    # Solaris/SunOS
    def comports(include_links=False):
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/tty*c')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

elif plat[:3] == 'aix':      # AIX
    def comports(include_links=False):
        """scan for available ports. return a list of device names."""
        devices = glob.glob('/dev/tty*')
        if include_links:
            devices.extend(list_ports_common.list_links(devices))
        return [list_ports_common.ListPortInfo(d) for d in devices]

else:
    # platform detection has failed...
    import serial
    sys.stderr.write("""\
don't know how to enumerate ttys on this system.
! I you know how the serial ports are named send this information to
! the author of this module:

sys.platform = {!r}
os.name = {!r}
pySerial version = {}

also add the naming scheme of the serial ports and with a bit luck you can get
this module running...
""".format(sys.platform, os.name, serial.VERSION))
    raise ImportError("Sorry: no implementation for your platform ('{}') available".format(os.name))

# test
if __name__ == '__main__':
    for port, desc, hwid in sorted(comports()):
        print("{}: {} [{}]".format(port, desc, hwid))
