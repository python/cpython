#! python
#
# This module implements a special URL handler that wraps an other port,
# print the traffic for debugging purposes. With this, it is possible
# to debug the serial port traffic on every application that uses
# serial_for_url.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause
#
# URL format:    spy://port[?option[=value][&option[=value]]]
# options:
# - dev=X   a file or device to write to
# - color   use escape code to colorize output
# - raw     forward raw bytes instead of hexdump
#
# example:
#   redirect output to an other terminal window on Posix (Linux):
#   python -m serial.tools.miniterm spy:///dev/ttyUSB0?dev=/dev/pts/14\&color

from __future__ import absolute_import

import sys
import time

import serial
from serial.serialutil import  to_bytes

try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse


def sixteen(data):
    """\
    yield tuples of hex and ASCII display in multiples of 16. Includes a
    space after 8 bytes and (None, None) after 16 bytes and at the end.
    """
    n = 0
    for b in serial.iterbytes(data):
        yield ('{:02X} '.format(ord(b)), b.decode('ascii') if b' ' <= b < b'\x7f' else '.')
        n += 1
        if n == 8:
            yield (' ', '')
        elif n >= 16:
            yield (None, None)
            n = 0
    if n > 0:
        while n < 16:
            n += 1
            if n == 8:
                yield (' ', '')
            yield ('   ', ' ')
        yield (None, None)


def hexdump(data):
    """yield lines with hexdump of data"""
    values = []
    ascii = []
    offset = 0
    for h, a in sixteen(data):
        if h is None:
            yield (offset, ' '.join([''.join(values), ''.join(ascii)]))
            del values[:]
            del ascii[:]
            offset += 0x10
        else:
            values.append(h)
            ascii.append(a)


class FormatRaw(object):
    """Forward only RX and TX data to output."""

    def __init__(self, output, color):
        self.output = output
        self.color = color
        self.rx_color = '\x1b[32m'
        self.tx_color = '\x1b[31m'

    def rx(self, data):
        """show received data"""
        if self.color:
            self.output.write(self.rx_color)
        self.output.write(data)
        self.output.flush()

    def tx(self, data):
        """show transmitted data"""
        if self.color:
            self.output.write(self.tx_color)
        self.output.write(data)
        self.output.flush()

    def control(self, name, value):
        """(do not) show control calls"""
        pass


class FormatHexdump(object):
    """\
    Create a hex dump of RX ad TX data, show when control lines are read or
    written.

    output example::

        000000.000 Q-RX flushInput
        000002.469 RTS  inactive
        000002.773 RTS  active
        000003.001 TX   48 45 4C 4C 4F                                    HELLO
        000003.102 RX   48 45 4C 4C 4F                                    HELLO

    """

    def __init__(self, output, color):
        self.start_time = time.time()
        self.output = output
        self.color = color
        self.rx_color = '\x1b[32m'
        self.tx_color = '\x1b[31m'
        self.control_color = '\x1b[37m'

    def write_line(self, timestamp, label, value, value2=''):
        self.output.write('{:010.3f} {:4} {}{}\n'.format(timestamp, label, value, value2))
        self.output.flush()

    def rx(self, data):
        """show received data as hex dump"""
        if self.color:
            self.output.write(self.rx_color)
        if data:
            for offset, row in hexdump(data):
                self.write_line(time.time() - self.start_time, 'RX', '{:04X}  '.format(offset), row)
        else:
            self.write_line(time.time() - self.start_time, 'RX', '<empty>')

    def tx(self, data):
        """show transmitted data as hex dump"""
        if self.color:
            self.output.write(self.tx_color)
        for offset, row in hexdump(data):
            self.write_line(time.time() - self.start_time, 'TX', '{:04X}  '.format(offset), row)

    def control(self, name, value):
        """show control calls"""
        if self.color:
            self.output.write(self.control_color)
        self.write_line(time.time() - self.start_time, name, value)


class Serial(serial.Serial):
    """\
    Inherit the native Serial port implementation and wrap all the methods and
    attributes.
    """
    # pylint: disable=no-member

    def __init__(self, *args, **kwargs):
        super(Serial, self).__init__(*args, **kwargs)
        self.formatter = None
        self.show_all = False

    @serial.Serial.port.setter
    def port(self, value):
        if value is not None:
            serial.Serial.port.__set__(self, self.from_url(value))

    def from_url(self, url):
        """extract host and port from an URL string"""
        parts = urlparse.urlsplit(url)
        if parts.scheme != 'spy':
            raise serial.SerialException(
                'expected a string in the form '
                '"spy://port[?option[=value][&option[=value]]]": '
                'not starting with spy:// ({!r})'.format(parts.scheme))
        # process options now, directly altering self
        formatter = FormatHexdump
        color = False
        output = sys.stderr
        try:
            for option, values in urlparse.parse_qs(parts.query, True).items():
                if option == 'file':
                    output = open(values[0], 'w')
                elif option == 'color':
                    color = True
                elif option == 'raw':
                    formatter = FormatRaw
                elif option == 'all':
                    self.show_all = True
                else:
                    raise ValueError('unknown option: {!r}'.format(option))
        except ValueError as e:
            raise serial.SerialException(
                'expected a string in the form '
                '"spy://port[?option[=value][&option[=value]]]": {}'.format(e))
        self.formatter = formatter(output, color)
        return ''.join([parts.netloc, parts.path])

    def write(self, tx):
        tx = to_bytes(tx)
        self.formatter.tx(tx)
        return super(Serial, self).write(tx)

    def read(self, size=1):
        rx = super(Serial, self).read(size)
        if rx or self.show_all:
            self.formatter.rx(rx)
        return rx

    if hasattr(serial.Serial, 'cancel_read'):
        def cancel_read(self):
            self.formatter.control('Q-RX', 'cancel_read')
            super(Serial, self).cancel_read()

    if hasattr(serial.Serial, 'cancel_write'):
        def cancel_write(self):
            self.formatter.control('Q-TX', 'cancel_write')
            super(Serial, self).cancel_write()

    @property
    def in_waiting(self):
        n = super(Serial, self).in_waiting
        if self.show_all:
            self.formatter.control('Q-RX', 'in_waiting -> {}'.format(n))
        return n

    def flush(self):
        self.formatter.control('Q-TX', 'flush')
        super(Serial, self).flush()

    def reset_input_buffer(self):
        self.formatter.control('Q-RX', 'reset_input_buffer')
        super(Serial, self).reset_input_buffer()

    def reset_output_buffer(self):
        self.formatter.control('Q-TX', 'reset_output_buffer')
        super(Serial, self).reset_output_buffer()

    def send_break(self, duration=0.25):
        self.formatter.control('BRK', 'send_break {}s'.format(duration))
        super(Serial, self).send_break(duration)

    @serial.Serial.break_condition.setter
    def break_condition(self, level):
        self.formatter.control('BRK', 'active' if level else 'inactive')
        serial.Serial.break_condition.__set__(self, level)

    @serial.Serial.rts.setter
    def rts(self, level):
        self.formatter.control('RTS', 'active' if level else 'inactive')
        serial.Serial.rts.__set__(self, level)

    @serial.Serial.dtr.setter
    def dtr(self, level):
        self.formatter.control('DTR', 'active' if level else 'inactive')
        serial.Serial.dtr.__set__(self, level)

    @serial.Serial.cts.getter
    def cts(self):
        level = super(Serial, self).cts
        self.formatter.control('CTS', 'active' if level else 'inactive')
        return level

    @serial.Serial.dsr.getter
    def dsr(self):
        level = super(Serial, self).dsr
        self.formatter.control('DSR', 'active' if level else 'inactive')
        return level

    @serial.Serial.ri.getter
    def ri(self):
        level = super(Serial, self).ri
        self.formatter.control('RI', 'active' if level else 'inactive')
        return level

    @serial.Serial.cd.getter
    def cd(self):
        level = super(Serial, self).cd
        self.formatter.control('CD', 'active' if level else 'inactive')
        return level

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if __name__ == '__main__':
    ser = Serial(None)
    ser.port = 'spy:///dev/ttyS0'
    print(ser)
