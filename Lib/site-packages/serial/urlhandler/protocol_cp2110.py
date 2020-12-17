#! python
#
# Backend for Silicon Labs CP2110/4 HID-to-UART devices.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2015 Chris Liechti <cliechti@gmx.net>
# (C) 2019 Google LLC
#
# SPDX-License-Identifier:    BSD-3-Clause

# This backend implements support for HID-to-UART devices manufactured
# by Silicon Labs and marketed as CP2110 and CP2114. The
# implementation is (mostly) OS-independent and in userland. It relies
# on cython-hidapi (https://github.com/trezor/cython-hidapi).

# The HID-to-UART protocol implemented by CP2110/4 is described in the
# AN434 document from Silicon Labs:
# https://www.silabs.com/documents/public/application-notes/AN434-CP2110-4-Interface-Specification.pdf

# TODO items:

# - rtscts support is configured for hardware flow control, but the
#   signaling is missing (AN434 suggests this is done through GPIO).
# - Cancelling reads and writes is not supported.
# - Baudrate validation is not implemented, as it depends on model and configuration.

import struct
import threading

try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse

try:
    import Queue
except ImportError:
    import queue as Queue

import hid  # hidapi

import serial
from serial.serialutil import SerialBase, SerialException, PortNotOpenError, to_bytes, Timeout


# Report IDs and related constant
_REPORT_GETSET_UART_ENABLE = 0x41
_DISABLE_UART = 0x00
_ENABLE_UART = 0x01

_REPORT_SET_PURGE_FIFOS = 0x43
_PURGE_TX_FIFO = 0x01
_PURGE_RX_FIFO = 0x02

_REPORT_GETSET_UART_CONFIG = 0x50

_REPORT_SET_TRANSMIT_LINE_BREAK = 0x51
_REPORT_SET_STOP_LINE_BREAK = 0x52


class Serial(SerialBase):
    # This is not quite correct. AN343 specifies that the minimum
    # baudrate is different between CP2110 and CP2114, and it's halved
    # when using non-8-bit symbols.
    BAUDRATES = (300, 375, 600, 1200, 1800, 2400, 4800, 9600, 19200,
                 38400, 57600, 115200, 230400, 460800, 500000, 576000,
                 921600, 1000000)

    def __init__(self, *args, **kwargs):
        self._hid_handle = None
        self._read_buffer = None
        self._thread = None
        super(Serial, self).__init__(*args, **kwargs)

    def open(self):
        if self._port is None:
            raise SerialException("Port must be configured before it can be used.")
        if self.is_open:
            raise SerialException("Port is already open.")

        self._read_buffer = Queue.Queue()

        self._hid_handle = hid.device()
        try:
            portpath = self.from_url(self.portstr)
            self._hid_handle.open_path(portpath)
        except OSError as msg:
            raise SerialException(msg.errno, "could not open port {}: {}".format(self._port, msg))

        try:
            self._reconfigure_port()
        except:
            try:
                self._hid_handle.close()
            except:
                pass
            self._hid_handle = None
            raise
        else:
            self.is_open = True
            self._thread = threading.Thread(target=self._hid_read_loop)
            self._thread.setDaemon(True)
            self._thread.setName('pySerial CP2110 reader thread for {}'.format(self._port))
            self._thread.start()

    def from_url(self, url):
        parts = urlparse.urlsplit(url)
        if parts.scheme != "cp2110":
            raise SerialException(
                'expected a string in the forms '
                '"cp2110:///dev/hidraw9" or "cp2110://0001:0023:00": '
                'not starting with cp2110:// {{!r}}'.format(parts.scheme))
        if parts.netloc:  # cp2100://BUS:DEVICE:ENDPOINT, for libusb
            return parts.netloc.encode('utf-8')
        return parts.path.encode('utf-8')

    def close(self):
        self.is_open = False
        if self._thread:
            self._thread.join(1)  # read timeout is 0.1
            self._thread = None
        self._hid_handle.close()
        self._hid_handle = None

    def _reconfigure_port(self):
        parity_value = None
        if self._parity == serial.PARITY_NONE:
            parity_value = 0x00
        elif self._parity == serial.PARITY_ODD:
            parity_value = 0x01
        elif self._parity == serial.PARITY_EVEN:
            parity_value = 0x02
        elif self._parity == serial.PARITY_MARK:
            parity_value = 0x03
        elif self._parity == serial.PARITY_SPACE:
            parity_value = 0x04
        else:
            raise ValueError('Invalid parity: {!r}'.format(self._parity))

        if self.rtscts:
            flow_control_value = 0x01
        else:
            flow_control_value = 0x00

        data_bits_value = None
        if self._bytesize == 5:
            data_bits_value = 0x00
        elif self._bytesize == 6:
            data_bits_value = 0x01
        elif self._bytesize == 7:
            data_bits_value = 0x02
        elif self._bytesize == 8:
            data_bits_value = 0x03
        else:
            raise ValueError('Invalid char len: {!r}'.format(self._bytesize))

        stop_bits_value = None
        if self._stopbits == serial.STOPBITS_ONE:
            stop_bits_value = 0x00
        elif self._stopbits == serial.STOPBITS_ONE_POINT_FIVE:
            stop_bits_value = 0x01
        elif self._stopbits == serial.STOPBITS_TWO:
            stop_bits_value = 0x01
        else:
            raise ValueError('Invalid stop bit specification: {!r}'.format(self._stopbits))

        configuration_report = struct.pack(
            '>BLBBBB',
            _REPORT_GETSET_UART_CONFIG,
            self._baudrate,
            parity_value,
            flow_control_value,
            data_bits_value,
            stop_bits_value)

        self._hid_handle.send_feature_report(configuration_report)

        self._hid_handle.send_feature_report(
            bytes((_REPORT_GETSET_UART_ENABLE, _ENABLE_UART)))
        self._update_break_state()

    @property
    def in_waiting(self):
        return self._read_buffer.qsize()

    def reset_input_buffer(self):
        if not self.is_open:
            raise PortNotOpenError()
        self._hid_handle.send_feature_report(
            bytes((_REPORT_SET_PURGE_FIFOS, _PURGE_RX_FIFO)))
        # empty read buffer
        while self._read_buffer.qsize():
            self._read_buffer.get(False)

    def reset_output_buffer(self):
        if not self.is_open:
            raise PortNotOpenError()
        self._hid_handle.send_feature_report(
            bytes((_REPORT_SET_PURGE_FIFOS, _PURGE_TX_FIFO)))

    def _update_break_state(self):
        if not self._hid_handle:
            raise PortNotOpenError()

        if self._break_state:
            self._hid_handle.send_feature_report(
                bytes((_REPORT_SET_TRANSMIT_LINE_BREAK, 0)))
        else:
            # Note that while AN434 states "There are no data bytes in
            # the payload other than the Report ID", either hidapi or
            # Linux does not seem to send the report otherwise.
            self._hid_handle.send_feature_report(
                bytes((_REPORT_SET_STOP_LINE_BREAK, 0)))

    def read(self, size=1):
        if not self.is_open:
            raise PortNotOpenError()

        data = bytearray()
        try:
            timeout = Timeout(self._timeout)
            while len(data) < size:
                if self._thread is None:
                    raise SerialException('connection failed (reader thread died)')
                buf = self._read_buffer.get(True, timeout.time_left())
                if buf is None:
                    return bytes(data)
                data += buf
                if timeout.expired():
                    break
        except Queue.Empty:  # -> timeout
            pass
        return bytes(data)

    def write(self, data):
        if not self.is_open:
            raise PortNotOpenError()
        data = to_bytes(data)
        tx_len = len(data)
        while tx_len > 0:
            to_be_sent = min(tx_len, 0x3F)
            report = to_bytes([to_be_sent]) + data[:to_be_sent]
            self._hid_handle.write(report)

            data = data[to_be_sent:]
            tx_len = len(data)

    def _hid_read_loop(self):
        try:
            while self.is_open:
                data = self._hid_handle.read(64, timeout_ms=100)
                if not data:
                    continue
                data_len = data.pop(0)
                assert data_len == len(data)
                self._read_buffer.put(bytearray(data))
        finally:
            self._thread = None
