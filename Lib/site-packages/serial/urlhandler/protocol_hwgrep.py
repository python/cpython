#! python
#
# This module implements a special URL handler that uses the port listing to
# find ports by searching the string descriptions.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2011-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause
#
# URL format:    hwgrep://<regexp>&<option>
#
# where <regexp> is a Python regexp according to the re module
#
# violating the normal definition for URLs, the charachter `&` is used to
# separate parameters from the arguments (instead of `?`, but the question mark
# is heavily used in regexp'es)
#
# options:
# n=<N>     pick the N'th entry instead of the first one (numbering starts at 1)
# skip_busy tries to open port to check if it is busy, fails on posix as ports are not locked!

from __future__ import absolute_import

import serial
import serial.tools.list_ports

try:
    basestring
except NameError:
    basestring = str    # python 3  pylint: disable=redefined-builtin


class Serial(serial.Serial):
    """Just inherit the native Serial port implementation and patch the port property."""
    # pylint: disable=no-member

    @serial.Serial.port.setter
    def port(self, value):
        """translate port name before storing it"""
        if isinstance(value, basestring) and value.startswith('hwgrep://'):
            serial.Serial.port.__set__(self, self.from_url(value))
        else:
            serial.Serial.port.__set__(self, value)

    def from_url(self, url):
        """extract host and port from an URL string"""
        if url.lower().startswith("hwgrep://"):
            url = url[9:]
        n = 0
        test_open = False
        args = url.split('&')
        regexp = args.pop(0)
        for arg in args:
            if '=' in arg:
                option, value = arg.split('=', 1)
            else:
                option = arg
                value = None
            if option == 'n':
                # pick n'th element
                n = int(value) - 1
                if n < 1:
                    raise ValueError('option "n" expects a positive integer larger than 1: {!r}'.format(value))
            elif option == 'skip_busy':
                # open to test if port is available. not the nicest way..
                test_open = True
            else:
                raise ValueError('unknown option: {!r}'.format(option))
        # use a for loop to get the 1st element from the generator
        for port, desc, hwid in sorted(serial.tools.list_ports.grep(regexp)):
            if test_open:
                try:
                    s = serial.Serial(port)
                except serial.SerialException:
                    # it has some error, skip this one
                    continue
                else:
                    s.close()
            if n:
                n -= 1
                continue
            return port
        else:
            raise serial.SerialException('no ports found matching regexp {!r}'.format(url))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if __name__ == '__main__':
    s = Serial(None)
    s.port = 'hwgrep://ttyS0'
    print(s)
