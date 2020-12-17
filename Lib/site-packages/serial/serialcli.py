#! python
#
# Backend for .NET/Mono (IronPython), .NET >= 2
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2008-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

from __future__ import absolute_import

import System
import System.IO.Ports
from serial.serialutil import *

# must invoke function with byte array, make a helper to convert strings
# to byte arrays
sab = System.Array[System.Byte]


def as_byte_array(string):
    return sab([ord(x) for x in string])  # XXX will require adaption when run with a 3.x compatible IronPython


class Serial(SerialBase):
    """Serial port implementation for .NET/Mono."""

    BAUDRATES = (50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
                 9600, 19200, 38400, 57600, 115200)

    def open(self):
        """\
        Open port with current settings. This may throw a SerialException
        if the port cannot be opened.
        """
        if self._port is None:
            raise SerialException("Port must be configured before it can be used.")
        if self.is_open:
            raise SerialException("Port is already open.")
        try:
            self._port_handle = System.IO.Ports.SerialPort(self.portstr)
        except Exception as msg:
            self._port_handle = None
            raise SerialException("could not open port %s: %s" % (self.portstr, msg))

        # if RTS and/or DTR are not set before open, they default to True
        if self._rts_state is None:
            self._rts_state = True
        if self._dtr_state is None:
            self._dtr_state = True

        self._reconfigure_port()
        self._port_handle.Open()
        self.is_open = True
        if not self._dsrdtr:
            self._update_dtr_state()
        if not self._rtscts:
            self._update_rts_state()
        self.reset_input_buffer()

    def _reconfigure_port(self):
        """Set communication parameters on opened port."""
        if not self._port_handle:
            raise SerialException("Can only operate on a valid port handle")

        #~ self._port_handle.ReceivedBytesThreshold = 1

        if self._timeout is None:
            self._port_handle.ReadTimeout = System.IO.Ports.SerialPort.InfiniteTimeout
        else:
            self._port_handle.ReadTimeout = int(self._timeout * 1000)

        # if self._timeout != 0 and self._interCharTimeout is not None:
            # timeouts = (int(self._interCharTimeout * 1000),) + timeouts[1:]

        if self._write_timeout is None:
            self._port_handle.WriteTimeout = System.IO.Ports.SerialPort.InfiniteTimeout
        else:
            self._port_handle.WriteTimeout = int(self._write_timeout * 1000)

        # Setup the connection info.
        try:
            self._port_handle.BaudRate = self._baudrate
        except IOError as e:
            # catch errors from illegal baudrate settings
            raise ValueError(str(e))

        if self._bytesize == FIVEBITS:
            self._port_handle.DataBits = 5
        elif self._bytesize == SIXBITS:
            self._port_handle.DataBits = 6
        elif self._bytesize == SEVENBITS:
            self._port_handle.DataBits = 7
        elif self._bytesize == EIGHTBITS:
            self._port_handle.DataBits = 8
        else:
            raise ValueError("Unsupported number of data bits: %r" % self._bytesize)

        if self._parity == PARITY_NONE:
            self._port_handle.Parity = getattr(System.IO.Ports.Parity, 'None')  # reserved keyword in Py3k
        elif self._parity == PARITY_EVEN:
            self._port_handle.Parity = System.IO.Ports.Parity.Even
        elif self._parity == PARITY_ODD:
            self._port_handle.Parity = System.IO.Ports.Parity.Odd
        elif self._parity == PARITY_MARK:
            self._port_handle.Parity = System.IO.Ports.Parity.Mark
        elif self._parity == PARITY_SPACE:
            self._port_handle.Parity = System.IO.Ports.Parity.Space
        else:
            raise ValueError("Unsupported parity mode: %r" % self._parity)

        if self._stopbits == STOPBITS_ONE:
            self._port_handle.StopBits = System.IO.Ports.StopBits.One
        elif self._stopbits == STOPBITS_ONE_POINT_FIVE:
            self._port_handle.StopBits = System.IO.Ports.StopBits.OnePointFive
        elif self._stopbits == STOPBITS_TWO:
            self._port_handle.StopBits = System.IO.Ports.StopBits.Two
        else:
            raise ValueError("Unsupported number of stop bits: %r" % self._stopbits)

        if self._rtscts and self._xonxoff:
            self._port_handle.Handshake = System.IO.Ports.Handshake.RequestToSendXOnXOff
        elif self._rtscts:
            self._port_handle.Handshake = System.IO.Ports.Handshake.RequestToSend
        elif self._xonxoff:
            self._port_handle.Handshake = System.IO.Ports.Handshake.XOnXOff
        else:
            self._port_handle.Handshake = getattr(System.IO.Ports.Handshake, 'None')   # reserved keyword in Py3k

    #~ def __del__(self):
        #~ self.close()

    def close(self):
        """Close port"""
        if self.is_open:
            if self._port_handle:
                try:
                    self._port_handle.Close()
                except System.IO.Ports.InvalidOperationException:
                    # ignore errors. can happen for unplugged USB serial devices
                    pass
                self._port_handle = None
            self.is_open = False

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    @property
    def in_waiting(self):
        """Return the number of characters currently in the input buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        return self._port_handle.BytesToRead

    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if not self.is_open:
            raise PortNotOpenError()
        # must use single byte reads as this is the only way to read
        # without applying encodings
        data = bytearray()
        while size:
            try:
                data.append(self._port_handle.ReadByte())
            except System.TimeoutException:
                break
            else:
                size -= 1
        return bytes(data)

    def write(self, data):
        """Output the given string over the serial port."""
        if not self.is_open:
            raise PortNotOpenError()
        #~ if not isinstance(data, (bytes, bytearray)):
            #~ raise TypeError('expected %s or bytearray, got %s' % (bytes, type(data)))
        try:
            # must call overloaded method with byte array argument
            # as this is the only one not applying encodings
            self._port_handle.Write(as_byte_array(data), 0, len(data))
        except System.TimeoutException:
            raise SerialTimeoutException('Write timeout')
        return len(data)

    def reset_input_buffer(self):
        """Clear input buffer, discarding all that is in the buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        self._port_handle.DiscardInBuffer()

    def reset_output_buffer(self):
        """\
        Clear output buffer, aborting the current output and
        discarding all that is in the buffer.
        """
        if not self.is_open:
            raise PortNotOpenError()
        self._port_handle.DiscardOutBuffer()

    def _update_break_state(self):
        """
        Set break: Controls TXD. When active, to transmitting is possible.
        """
        if not self.is_open:
            raise PortNotOpenError()
        self._port_handle.BreakState = bool(self._break_state)

    def _update_rts_state(self):
        """Set terminal status line: Request To Send"""
        if not self.is_open:
            raise PortNotOpenError()
        self._port_handle.RtsEnable = bool(self._rts_state)

    def _update_dtr_state(self):
        """Set terminal status line: Data Terminal Ready"""
        if not self.is_open:
            raise PortNotOpenError()
        self._port_handle.DtrEnable = bool(self._dtr_state)

    @property
    def cts(self):
        """Read terminal status line: Clear To Send"""
        if not self.is_open:
            raise PortNotOpenError()
        return self._port_handle.CtsHolding

    @property
    def dsr(self):
        """Read terminal status line: Data Set Ready"""
        if not self.is_open:
            raise PortNotOpenError()
        return self._port_handle.DsrHolding

    @property
    def ri(self):
        """Read terminal status line: Ring Indicator"""
        if not self.is_open:
            raise PortNotOpenError()
        #~ return self._port_handle.XXX
        return False  # XXX an error would be better

    @property
    def cd(self):
        """Read terminal status line: Carrier Detect"""
        if not self.is_open:
            raise PortNotOpenError()
        return self._port_handle.CDHolding

    # - - platform specific - - - -
    # none
