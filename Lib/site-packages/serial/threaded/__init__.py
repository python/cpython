#!/usr/bin/env python3
#
# Working with threading and pySerial
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2015-2016 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause
"""\
Support threading with serial ports.
"""
from __future__ import absolute_import

import serial
import threading


class Protocol(object):
    """\
    Protocol as used by the ReaderThread. This base class provides empty
    implementations of all methods.
    """

    def connection_made(self, transport):
        """Called when reader thread is started"""

    def data_received(self, data):
        """Called with snippets received from the serial port"""

    def connection_lost(self, exc):
        """\
        Called when the serial port is closed or the reader loop terminated
        otherwise.
        """
        if isinstance(exc, Exception):
            raise exc


class Packetizer(Protocol):
    """
    Read binary packets from serial port. Packets are expected to be terminated
    with a TERMINATOR byte (null byte by default).

    The class also keeps track of the transport.
    """

    TERMINATOR = b'\0'

    def __init__(self):
        self.buffer = bytearray()
        self.transport = None

    def connection_made(self, transport):
        """Store transport"""
        self.transport = transport

    def connection_lost(self, exc):
        """Forget transport"""
        self.transport = None
        super(Packetizer, self).connection_lost(exc)

    def data_received(self, data):
        """Buffer received data, find TERMINATOR, call handle_packet"""
        self.buffer.extend(data)
        while self.TERMINATOR in self.buffer:
            packet, self.buffer = self.buffer.split(self.TERMINATOR, 1)
            self.handle_packet(packet)

    def handle_packet(self, packet):
        """Process packets - to be overridden by subclassing"""
        raise NotImplementedError('please implement functionality in handle_packet')


class FramedPacket(Protocol):
    """
    Read binary packets. Packets are expected to have a start and stop marker.

    The class also keeps track of the transport.
    """

    START = b'('
    STOP = b')'

    def __init__(self):
        self.packet = bytearray()
        self.in_packet = False
        self.transport = None

    def connection_made(self, transport):
        """Store transport"""
        self.transport = transport

    def connection_lost(self, exc):
        """Forget transport"""
        self.transport = None
        self.in_packet = False
        del self.packet[:]
        super(FramedPacket, self).connection_lost(exc)

    def data_received(self, data):
        """Find data enclosed in START/STOP, call handle_packet"""
        for byte in serial.iterbytes(data):
            if byte == self.START:
                self.in_packet = True
            elif byte == self.STOP:
                self.in_packet = False
                self.handle_packet(bytes(self.packet)) # make read-only copy
                del self.packet[:]
            elif self.in_packet:
                self.packet.extend(byte)
            else:
                self.handle_out_of_packet_data(byte)

    def handle_packet(self, packet):
        """Process packets - to be overridden by subclassing"""
        raise NotImplementedError('please implement functionality in handle_packet')

    def handle_out_of_packet_data(self, data):
        """Process data that is received outside of packets"""
        pass


class LineReader(Packetizer):
    """
    Read and write (Unicode) lines from/to serial port.
    The encoding is applied.
    """

    TERMINATOR = b'\r\n'
    ENCODING = 'utf-8'
    UNICODE_HANDLING = 'replace'

    def handle_packet(self, packet):
        self.handle_line(packet.decode(self.ENCODING, self.UNICODE_HANDLING))

    def handle_line(self, line):
        """Process one line - to be overridden by subclassing"""
        raise NotImplementedError('please implement functionality in handle_line')

    def write_line(self, text):
        """
        Write text to the transport. ``text`` is a Unicode string and the encoding
        is applied before sending ans also the newline is append.
        """
        # + is not the best choice but bytes does not support % or .format in py3 and we want a single write call
        self.transport.write(text.encode(self.ENCODING, self.UNICODE_HANDLING) + self.TERMINATOR)


class ReaderThread(threading.Thread):
    """\
    Implement a serial port read loop and dispatch to a Protocol instance (like
    the asyncio.Protocol) but do it with threads.

    Calls to close() will close the serial port but it is also possible to just
    stop() this thread and continue the serial port instance otherwise.
    """

    def __init__(self, serial_instance, protocol_factory):
        """\
        Initialize thread.

        Note that the serial_instance' timeout is set to one second!
        Other settings are not changed.
        """
        super(ReaderThread, self).__init__()
        self.daemon = True
        self.serial = serial_instance
        self.protocol_factory = protocol_factory
        self.alive = True
        self._lock = threading.Lock()
        self._connection_made = threading.Event()
        self.protocol = None

    def stop(self):
        """Stop the reader thread"""
        self.alive = False
        if hasattr(self.serial, 'cancel_read'):
            self.serial.cancel_read()
        self.join(2)

    def run(self):
        """Reader loop"""
        if not hasattr(self.serial, 'cancel_read'):
            self.serial.timeout = 1
        self.protocol = self.protocol_factory()
        try:
            self.protocol.connection_made(self)
        except Exception as e:
            self.alive = False
            self.protocol.connection_lost(e)
            self._connection_made.set()
            return
        error = None
        self._connection_made.set()
        while self.alive and self.serial.is_open:
            try:
                # read all that is there or wait for one byte (blocking)
                data = self.serial.read(self.serial.in_waiting or 1)
            except serial.SerialException as e:
                # probably some I/O problem such as disconnected USB serial
                # adapters -> exit
                error = e
                break
            else:
                if data:
                    # make a separated try-except for called user code
                    try:
                        self.protocol.data_received(data)
                    except Exception as e:
                        error = e
                        break
        self.alive = False
        self.protocol.connection_lost(error)
        self.protocol = None

    def write(self, data):
        """Thread safe writing (uses lock)"""
        with self._lock:
            return self.serial.write(data)

    def close(self):
        """Close the serial port and exit reader thread (uses lock)"""
        # use the lock to let other threads finish writing
        with self._lock:
            # first stop reading, so that closing can be done on idle port
            self.stop()
            self.serial.close()

    def connect(self):
        """
        Wait until connection is set up and return the transport and protocol
        instances.
        """
        if self.alive:
            self._connection_made.wait()
            if not self.alive:
                raise RuntimeError('connection_lost already called')
            return (self, self.protocol)
        else:
            raise RuntimeError('already stopped')

    # - -  context manager, returns protocol

    def __enter__(self):
        """\
        Enter context handler. May raise RuntimeError in case the connection
        could not be created.
        """
        self.start()
        self._connection_made.wait()
        if not self.alive:
            raise RuntimeError('connection_lost already called')
        return self.protocol

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Leave context: close port"""
        self.close()


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# test
if __name__ == '__main__':
    # pylint: disable=wrong-import-position
    import sys
    import time
    import traceback

    #~ PORT = 'spy:///dev/ttyUSB0'
    PORT = 'loop://'

    class PrintLines(LineReader):
        def connection_made(self, transport):
            super(PrintLines, self).connection_made(transport)
            sys.stdout.write('port opened\n')
            self.write_line('hello world')

        def handle_line(self, data):
            sys.stdout.write('line received: {!r}\n'.format(data))

        def connection_lost(self, exc):
            if exc:
                traceback.print_exc(exc)
            sys.stdout.write('port closed\n')

    ser = serial.serial_for_url(PORT, baudrate=115200, timeout=1)
    with ReaderThread(ser, PrintLines) as protocol:
        protocol.write_line('hello')
        time.sleep(2)

    # alternative usage
    ser = serial.serial_for_url(PORT, baudrate=115200, timeout=1)
    t = ReaderThread(ser, PrintLines)
    t.start()
    transport, protocol = t.connect()
    protocol.write_line('hello')
    time.sleep(2)
    t.close()
