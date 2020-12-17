#! python
#
# This module implements a loop back connection receiving itself what it sent.
#
# The purpose of this module is.. well... You can run the unit tests with it.
# and it was so easy to implement ;-)
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2020 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause
#
# URL format:    loop://[option[/option...]]
# options:
# - "debug" print diagnostic messages
from __future__ import absolute_import

import logging
import numbers
import time
try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse
try:
    import queue
except ImportError:
    import Queue as queue

from serial.serialutil import SerialBase, SerialException, to_bytes, iterbytes, SerialTimeoutException, PortNotOpenError

# map log level names to constants. used in from_url()
LOGGER_LEVELS = {
    'debug': logging.DEBUG,
    'info': logging.INFO,
    'warning': logging.WARNING,
    'error': logging.ERROR,
}


class Serial(SerialBase):
    """Serial port implementation that simulates a loop back connection in plain software."""

    BAUDRATES = (50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
                 9600, 19200, 38400, 57600, 115200)

    def __init__(self, *args, **kwargs):
        self.buffer_size = 4096
        self.queue = None
        self.logger = None
        self._cancel_write = False
        super(Serial, self).__init__(*args, **kwargs)

    def open(self):
        """\
        Open port with current settings. This may throw a SerialException
        if the port cannot be opened.
        """
        if self.is_open:
            raise SerialException("Port is already open.")
        self.logger = None
        self.queue = queue.Queue(self.buffer_size)

        if self._port is None:
            raise SerialException("Port must be configured before it can be used.")
        # not that there is anything to open, but the function applies the
        # options found in the URL
        self.from_url(self.port)

        # not that there anything to configure...
        self._reconfigure_port()
        # all things set up get, now a clean start
        self.is_open = True
        if not self._dsrdtr:
            self._update_dtr_state()
        if not self._rtscts:
            self._update_rts_state()
        self.reset_input_buffer()
        self.reset_output_buffer()

    def close(self):
        if self.is_open:
            self.is_open = False
            try:
                self.queue.put_nowait(None)
            except queue.Full:
                pass
        super(Serial, self).close()

    def _reconfigure_port(self):
        """\
        Set communication parameters on opened port. For the loop://
        protocol all settings are ignored!
        """
        # not that's it of any real use, but it helps in the unit tests
        if not isinstance(self._baudrate, numbers.Integral) or not 0 < self._baudrate < 2 ** 32:
            raise ValueError("invalid baudrate: {!r}".format(self._baudrate))
        if self.logger:
            self.logger.info('_reconfigure_port()')

    def from_url(self, url):
        """extract host and port from an URL string"""
        parts = urlparse.urlsplit(url)
        if parts.scheme != "loop":
            raise SerialException(
                'expected a string in the form '
                '"loop://[?logging={debug|info|warning|error}]": not starting '
                'with loop:// ({!r})'.format(parts.scheme))
        try:
            # process options now, directly altering self
            for option, values in urlparse.parse_qs(parts.query, True).items():
                if option == 'logging':
                    logging.basicConfig()   # XXX is that good to call it here?
                    self.logger = logging.getLogger('pySerial.loop')
                    self.logger.setLevel(LOGGER_LEVELS[values[0]])
                    self.logger.debug('enabled logging')
                else:
                    raise ValueError('unknown option: {!r}'.format(option))
        except ValueError as e:
            raise SerialException(
                'expected a string in the form '
                '"loop://[?logging={debug|info|warning|error}]": {}'.format(e))

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    @property
    def in_waiting(self):
        """Return the number of bytes currently in the input buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            # attention the logged value can differ from return value in
            # threaded environments...
            self.logger.debug('in_waiting -> {:d}'.format(self.queue.qsize()))
        return self.queue.qsize()

    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if not self.is_open:
            raise PortNotOpenError()
        if self._timeout is not None and self._timeout != 0:
            timeout = time.time() + self._timeout
        else:
            timeout = None
        data = bytearray()
        while size > 0 and self.is_open:
            try:
                b = self.queue.get(timeout=self._timeout)  # XXX inter char timeout
            except queue.Empty:
                if self._timeout == 0:
                    break
            else:
                if b is not None:
                    data += b
                    size -= 1
                else:
                    break
            # check for timeout now, after data has been read.
            # useful for timeout = 0 (non blocking) read
            if timeout and time.time() > timeout:
                if self.logger:
                    self.logger.info('read timeout')
                break
        return bytes(data)

    def cancel_read(self):
        self.queue.put_nowait(None)

    def cancel_write(self):
        self._cancel_write = True

    def write(self, data):
        """\
        Output the given byte string over the serial port. Can block if the
        connection is blocked. May raise SerialException if the connection is
        closed.
        """
        self._cancel_write = False
        if not self.is_open:
            raise PortNotOpenError()
        data = to_bytes(data)
        # calculate aprox time that would be used to send the data
        time_used_to_send = 10.0 * len(data) / self._baudrate
        # when a write timeout is configured check if we would be successful
        # (not sending anything, not even the part that would have time)
        if self._write_timeout is not None and time_used_to_send > self._write_timeout:
            # must wait so that unit test succeeds
            time_left = self._write_timeout
            while time_left > 0 and not self._cancel_write:
                time.sleep(min(time_left, 0.5))
                time_left -= 0.5
            if self._cancel_write:
                return 0  # XXX
            raise SerialTimeoutException('Write timeout')
        for byte in iterbytes(data):
            self.queue.put(byte, timeout=self._write_timeout)
        return len(data)

    def reset_input_buffer(self):
        """Clear input buffer, discarding all that is in the buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('reset_input_buffer()')
        try:
            while self.queue.qsize():
                self.queue.get_nowait()
        except queue.Empty:
            pass

    def reset_output_buffer(self):
        """\
        Clear output buffer, aborting the current output and
        discarding all that is in the buffer.
        """
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('reset_output_buffer()')
        try:
            while self.queue.qsize():
                self.queue.get_nowait()
        except queue.Empty:
            pass

    @property
    def out_waiting(self):
        """Return how many bytes the in the outgoing buffer"""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            # attention the logged value can differ from return value in
            # threaded environments...
            self.logger.debug('out_waiting -> {:d}'.format(self.queue.qsize()))
        return self.queue.qsize()

    def _update_break_state(self):
        """\
        Set break: Controls TXD. When active, to transmitting is
        possible.
        """
        if self.logger:
            self.logger.info('_update_break_state({!r})'.format(self._break_state))

    def _update_rts_state(self):
        """Set terminal status line: Request To Send"""
        if self.logger:
            self.logger.info('_update_rts_state({!r}) -> state of CTS'.format(self._rts_state))

    def _update_dtr_state(self):
        """Set terminal status line: Data Terminal Ready"""
        if self.logger:
            self.logger.info('_update_dtr_state({!r}) -> state of DSR'.format(self._dtr_state))

    @property
    def cts(self):
        """Read terminal status line: Clear To Send"""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('CTS -> state of RTS ({!r})'.format(self._rts_state))
        return self._rts_state

    @property
    def dsr(self):
        """Read terminal status line: Data Set Ready"""
        if self.logger:
            self.logger.info('DSR -> state of DTR ({!r})'.format(self._dtr_state))
        return self._dtr_state

    @property
    def ri(self):
        """Read terminal status line: Ring Indicator"""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('returning dummy for RI')
        return False

    @property
    def cd(self):
        """Read terminal status line: Carrier Detect"""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('returning dummy for CD')
        return True

    # - - - platform specific - - -
    # None so far


# simple client test
if __name__ == '__main__':
    import sys
    s = Serial('loop://')
    sys.stdout.write('{}\n'.format(s))

    sys.stdout.write("write...\n")
    s.write("hello\n")
    s.flush()
    sys.stdout.write("read: {!r}\n".format(s.read(5)))

    s.close()
