#!/usr/bin/env python
#
# backend for serial IO for POSIX compatible systems, like Linux, OSX
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2020 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause
#
# parts based on code from Grant B. Edwards  <grante@visi.com>:
#  ftp://ftp.visi.com/users/grante/python/PosixSerial.py
#
# references: http://www.easysw.com/~mike/serial/serial.html

# Collection of port names (was previously used by number_to_device which was
# removed.
# - Linux                   /dev/ttyS%d (confirmed)
# - cygwin/win32            /dev/com%d (confirmed)
# - openbsd (OpenBSD)       /dev/cua%02d
# - bsd*, freebsd*          /dev/cuad%d
# - darwin (OS X)           /dev/cuad%d
# - netbsd                  /dev/dty%02d (NetBSD 1.6 testing by Erk)
# - irix (IRIX)             /dev/ttyf%d (partially tested) names depending on flow control
# - hp (HP-UX)              /dev/tty%dp0 (not tested)
# - sunos (Solaris/SunOS)   /dev/tty%c (letters, 'a'..'z') (confirmed)
# - aix (AIX)               /dev/tty%d


from __future__ import absolute_import

# pylint: disable=abstract-method
import errno
import fcntl
import os
import select
import struct
import sys
import termios

import serial
from serial.serialutil import SerialBase, SerialException, to_bytes, \
    PortNotOpenError, SerialTimeoutException, Timeout


class PlatformSpecificBase(object):
    BAUDRATE_CONSTANTS = {}

    def _set_special_baudrate(self, baudrate):
        raise NotImplementedError('non-standard baudrates are not supported on this platform')

    def _set_rs485_mode(self, rs485_settings):
        raise NotImplementedError('RS485 not supported on this platform')

    def set_low_latency_mode(self, low_latency_settings):
        raise NotImplementedError('Low latency not supported on this platform')

    def _update_break_state(self):
        """\
        Set break: Controls TXD. When active, no transmitting is possible.
        """
        if self._break_state:
            fcntl.ioctl(self.fd, TIOCSBRK)
        else:
            fcntl.ioctl(self.fd, TIOCCBRK)
    

# some systems support an extra flag to enable the two in POSIX unsupported
# paritiy settings for MARK and SPACE
CMSPAR = 0  # default, for unsupported platforms, override below

# try to detect the OS so that a device can be selected...
# this code block should supply a device() and set_special_baudrate() function
# for the platform
plat = sys.platform.lower()

if plat[:5] == 'linux':    # Linux (confirmed)  # noqa
    import array

    # extra termios flags
    CMSPAR = 0o10000000000  # Use "stick" (mark/space) parity

    # baudrate ioctls
    TCGETS2 = 0x802C542A
    TCSETS2 = 0x402C542B
    BOTHER = 0o010000

    # RS485 ioctls
    TIOCGRS485 = 0x542E
    TIOCSRS485 = 0x542F
    SER_RS485_ENABLED = 0b00000001
    SER_RS485_RTS_ON_SEND = 0b00000010
    SER_RS485_RTS_AFTER_SEND = 0b00000100
    SER_RS485_RX_DURING_TX = 0b00010000

    class PlatformSpecific(PlatformSpecificBase):
        BAUDRATE_CONSTANTS = {
            0:       0o000000,  # hang up
            50:      0o000001,
            75:      0o000002,
            110:     0o000003,
            134:     0o000004,
            150:     0o000005,
            200:     0o000006,
            300:     0o000007,
            600:     0o000010,
            1200:    0o000011,
            1800:    0o000012,
            2400:    0o000013,
            4800:    0o000014,
            9600:    0o000015,
            19200:   0o000016,
            38400:   0o000017,
            57600:   0o010001,
            115200:  0o010002,
            230400:  0o010003,
            460800:  0o010004,
            500000:  0o010005,
            576000:  0o010006,
            921600:  0o010007,
            1000000: 0o010010,
            1152000: 0o010011,
            1500000: 0o010012,
            2000000: 0o010013,
            2500000: 0o010014,
            3000000: 0o010015,
            3500000: 0o010016,
            4000000: 0o010017
        }

        def set_low_latency_mode(self, low_latency_settings):
            buf = array.array('i', [0] * 32)

            try:
                # get serial_struct
                fcntl.ioctl(self.fd, termios.TIOCGSERIAL, buf)

                # set or unset ASYNC_LOW_LATENCY flag
                if low_latency_settings:
                    buf[4] |= 0x2000
                else:
                    buf[4] &= ~0x2000

                # set serial_struct
                fcntl.ioctl(self.fd, termios.TIOCSSERIAL, buf)
            except IOError as e:
                raise ValueError('Failed to update ASYNC_LOW_LATENCY flag to {}: {}'.format(low_latency_settings, e))

        def _set_special_baudrate(self, baudrate):
            # right size is 44 on x86_64, allow for some growth
            buf = array.array('i', [0] * 64)
            try:
                # get serial_struct
                fcntl.ioctl(self.fd, TCGETS2, buf)
                # set custom speed
                buf[2] &= ~termios.CBAUD
                buf[2] |= BOTHER
                buf[9] = buf[10] = baudrate

                # set serial_struct
                fcntl.ioctl(self.fd, TCSETS2, buf)
            except IOError as e:
                raise ValueError('Failed to set custom baud rate ({}): {}'.format(baudrate, e))

        def _set_rs485_mode(self, rs485_settings):
            buf = array.array('i', [0] * 8)  # flags, delaytx, delayrx, padding
            try:
                fcntl.ioctl(self.fd, TIOCGRS485, buf)
                buf[0] |= SER_RS485_ENABLED
                if rs485_settings is not None:
                    if rs485_settings.loopback:
                        buf[0] |= SER_RS485_RX_DURING_TX
                    else:
                        buf[0] &= ~SER_RS485_RX_DURING_TX
                    if rs485_settings.rts_level_for_tx:
                        buf[0] |= SER_RS485_RTS_ON_SEND
                    else:
                        buf[0] &= ~SER_RS485_RTS_ON_SEND
                    if rs485_settings.rts_level_for_rx:
                        buf[0] |= SER_RS485_RTS_AFTER_SEND
                    else:
                        buf[0] &= ~SER_RS485_RTS_AFTER_SEND
                    if rs485_settings.delay_before_tx is not None:
                        buf[1] = int(rs485_settings.delay_before_tx * 1000)
                    if rs485_settings.delay_before_rx is not None:
                        buf[2] = int(rs485_settings.delay_before_rx * 1000)
                else:
                    buf[0] = 0  # clear SER_RS485_ENABLED
                fcntl.ioctl(self.fd, TIOCSRS485, buf)
            except IOError as e:
                raise ValueError('Failed to set RS485 mode: {}'.format(e))


elif plat == 'cygwin':       # cygwin/win32 (confirmed)

    class PlatformSpecific(PlatformSpecificBase):
        BAUDRATE_CONSTANTS = {
            128000: 0x01003,
            256000: 0x01005,
            500000: 0x01007,
            576000: 0x01008,
            921600: 0x01009,
            1000000: 0x0100a,
            1152000: 0x0100b,
            1500000: 0x0100c,
            2000000: 0x0100d,
            2500000: 0x0100e,
            3000000: 0x0100f
        }


elif plat[:6] == 'darwin':   # OS X
    import array
    IOSSIOSPEED = 0x80045402  # _IOW('T', 2, speed_t)

    class PlatformSpecific(PlatformSpecificBase):
        osx_version = os.uname()[2].split('.')
        TIOCSBRK = 0x2000747B # _IO('t', 123)
        TIOCCBRK = 0x2000747A # _IO('t', 122)

        # Tiger or above can support arbitrary serial speeds
        if int(osx_version[0]) >= 8:
            def _set_special_baudrate(self, baudrate):
                # use IOKit-specific call to set up high speeds
                buf = array.array('i', [baudrate])
                fcntl.ioctl(self.fd, IOSSIOSPEED, buf, 1)

        def _update_break_state(self):
            """\
            Set break: Controls TXD. When active, no transmitting is possible.
            """
            if self._break_state:
                fcntl.ioctl(self.fd, PlatformSpecific.TIOCSBRK)
            else:
                fcntl.ioctl(self.fd, PlatformSpecific.TIOCCBRK)

elif plat[:3] == 'bsd' or \
     plat[:7] == 'freebsd' or \
     plat[:6] == 'netbsd' or \
     plat[:7] == 'openbsd':

    class ReturnBaudrate(object):
        def __getitem__(self, key):
            return key

    class PlatformSpecific(PlatformSpecificBase):
        # Only tested on FreeBSD:
        # The baud rate may be passed in as
        # a literal value.
        BAUDRATE_CONSTANTS = ReturnBaudrate()

        TIOCSBRK = 0x2000747B # _IO('t', 123)
        TIOCCBRK = 0x2000747A # _IO('t', 122)

        
        def _update_break_state(self):
            """\
            Set break: Controls TXD. When active, no transmitting is possible.
            """
            if self._break_state:
                fcntl.ioctl(self.fd, PlatformSpecific.TIOCSBRK)
            else:
                fcntl.ioctl(self.fd, PlatformSpecific.TIOCCBRK)

else:
    class PlatformSpecific(PlatformSpecificBase):
        pass


# load some constants for later use.
# try to use values from termios, use defaults from linux otherwise
TIOCMGET = getattr(termios, 'TIOCMGET', 0x5415)
TIOCMBIS = getattr(termios, 'TIOCMBIS', 0x5416)
TIOCMBIC = getattr(termios, 'TIOCMBIC', 0x5417)
TIOCMSET = getattr(termios, 'TIOCMSET', 0x5418)

# TIOCM_LE = getattr(termios, 'TIOCM_LE', 0x001)
TIOCM_DTR = getattr(termios, 'TIOCM_DTR', 0x002)
TIOCM_RTS = getattr(termios, 'TIOCM_RTS', 0x004)
# TIOCM_ST = getattr(termios, 'TIOCM_ST', 0x008)
# TIOCM_SR = getattr(termios, 'TIOCM_SR', 0x010)

TIOCM_CTS = getattr(termios, 'TIOCM_CTS', 0x020)
TIOCM_CAR = getattr(termios, 'TIOCM_CAR', 0x040)
TIOCM_RNG = getattr(termios, 'TIOCM_RNG', 0x080)
TIOCM_DSR = getattr(termios, 'TIOCM_DSR', 0x100)
TIOCM_CD = getattr(termios, 'TIOCM_CD', TIOCM_CAR)
TIOCM_RI = getattr(termios, 'TIOCM_RI', TIOCM_RNG)
# TIOCM_OUT1 = getattr(termios, 'TIOCM_OUT1', 0x2000)
# TIOCM_OUT2 = getattr(termios, 'TIOCM_OUT2', 0x4000)
if hasattr(termios, 'TIOCINQ'):
    TIOCINQ = termios.TIOCINQ
else:
    TIOCINQ = getattr(termios, 'FIONREAD', 0x541B)
TIOCOUTQ = getattr(termios, 'TIOCOUTQ', 0x5411)

TIOCM_zero_str = struct.pack('I', 0)
TIOCM_RTS_str = struct.pack('I', TIOCM_RTS)
TIOCM_DTR_str = struct.pack('I', TIOCM_DTR)

TIOCSBRK = getattr(termios, 'TIOCSBRK', 0x5427)
TIOCCBRK = getattr(termios, 'TIOCCBRK', 0x5428)


class Serial(SerialBase, PlatformSpecific):
    """\
    Serial port class POSIX implementation. Serial port configuration is
    done with termios and fcntl. Runs on Linux and many other Un*x like
    systems.
    """

    def open(self):
        """\
        Open port with current settings. This may throw a SerialException
        if the port cannot be opened."""
        if self._port is None:
            raise SerialException("Port must be configured before it can be used.")
        if self.is_open:
            raise SerialException("Port is already open.")
        self.fd = None
        # open
        try:
            self.fd = os.open(self.portstr, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        except OSError as msg:
            self.fd = None
            raise SerialException(msg.errno, "could not open port {}: {}".format(self._port, msg))
        #~ fcntl.fcntl(self.fd, fcntl.F_SETFL, 0)  # set blocking

        self.pipe_abort_read_r, self.pipe_abort_read_w = None, None
        self.pipe_abort_write_r, self.pipe_abort_write_w = None, None

        try:
            self._reconfigure_port(force_update=True)

            try:
                if not self._dsrdtr:
                    self._update_dtr_state()
                if not self._rtscts:
                    self._update_rts_state()
            except IOError as e:
                # ignore Invalid argument and Inappropriate ioctl
                if e.errno not in (errno.EINVAL, errno.ENOTTY):
                    raise

            self._reset_input_buffer()

            self.pipe_abort_read_r, self.pipe_abort_read_w = os.pipe()
            self.pipe_abort_write_r, self.pipe_abort_write_w = os.pipe()
            fcntl.fcntl(self.pipe_abort_read_r, fcntl.F_SETFL, os.O_NONBLOCK)
            fcntl.fcntl(self.pipe_abort_write_r, fcntl.F_SETFL, os.O_NONBLOCK)
        except BaseException:
            try:
                os.close(self.fd)
            except Exception:
                # ignore any exception when closing the port
                # also to keep original exception that happened when setting up
                pass
            self.fd = None

            if self.pipe_abort_read_w is not None:
                os.close(self.pipe_abort_read_w)
                self.pipe_abort_read_w = None
            if self.pipe_abort_read_r is not None:
                os.close(self.pipe_abort_read_r)
                self.pipe_abort_read_r = None
            if self.pipe_abort_write_w is not None:
                os.close(self.pipe_abort_write_w)
                self.pipe_abort_write_w = None
            if self.pipe_abort_write_r is not None:
                os.close(self.pipe_abort_write_r)
                self.pipe_abort_write_r = None

            raise

        self.is_open = True

    def _reconfigure_port(self, force_update=False):
        """Set communication parameters on opened port."""
        if self.fd is None:
            raise SerialException("Can only operate on a valid file descriptor")

        # if exclusive lock is requested, create it before we modify anything else
        if self._exclusive is not None:
            if self._exclusive:
                try:
                    fcntl.flock(self.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                except IOError as msg:
                    raise SerialException(msg.errno, "Could not exclusively lock port {}: {}".format(self._port, msg))
            else:
                fcntl.flock(self.fd, fcntl.LOCK_UN)

        custom_baud = None

        vmin = vtime = 0                # timeout is done via select
        if self._inter_byte_timeout is not None:
            vmin = 1
            vtime = int(self._inter_byte_timeout * 10)
        try:
            orig_attr = termios.tcgetattr(self.fd)
            iflag, oflag, cflag, lflag, ispeed, ospeed, cc = orig_attr
        except termios.error as msg:      # if a port is nonexistent but has a /dev file, it'll fail here
            raise SerialException("Could not configure port: {}".format(msg))
        # set up raw mode / no echo / binary
        cflag |= (termios.CLOCAL | termios.CREAD)
        lflag &= ~(termios.ICANON | termios.ECHO | termios.ECHOE |
                   termios.ECHOK | termios.ECHONL |
                   termios.ISIG | termios.IEXTEN)  # |termios.ECHOPRT
        for flag in ('ECHOCTL', 'ECHOKE'):  # netbsd workaround for Erk
            if hasattr(termios, flag):
                lflag &= ~getattr(termios, flag)

        oflag &= ~(termios.OPOST | termios.ONLCR | termios.OCRNL)
        iflag &= ~(termios.INLCR | termios.IGNCR | termios.ICRNL | termios.IGNBRK)
        if hasattr(termios, 'IUCLC'):
            iflag &= ~termios.IUCLC
        if hasattr(termios, 'PARMRK'):
            iflag &= ~termios.PARMRK

        # setup baud rate
        try:
            ispeed = ospeed = getattr(termios, 'B{}'.format(self._baudrate))
        except AttributeError:
            try:
                ispeed = ospeed = self.BAUDRATE_CONSTANTS[self._baudrate]
            except KeyError:
                #~ raise ValueError('Invalid baud rate: %r' % self._baudrate)

                # See if BOTHER is defined for this platform; if it is, use
                # this for a speed not defined in the baudrate constants list.
                try:
                    ispeed = ospeed = BOTHER
                except NameError:
                    # may need custom baud rate, it isn't in our list.
                    ispeed = ospeed = getattr(termios, 'B38400')

                try:
                    custom_baud = int(self._baudrate)  # store for later
                except ValueError:
                    raise ValueError('Invalid baud rate: {!r}'.format(self._baudrate))
                else:
                    if custom_baud < 0:
                        raise ValueError('Invalid baud rate: {!r}'.format(self._baudrate))

        # setup char len
        cflag &= ~termios.CSIZE
        if self._bytesize == 8:
            cflag |= termios.CS8
        elif self._bytesize == 7:
            cflag |= termios.CS7
        elif self._bytesize == 6:
            cflag |= termios.CS6
        elif self._bytesize == 5:
            cflag |= termios.CS5
        else:
            raise ValueError('Invalid char len: {!r}'.format(self._bytesize))
        # setup stop bits
        if self._stopbits == serial.STOPBITS_ONE:
            cflag &= ~(termios.CSTOPB)
        elif self._stopbits == serial.STOPBITS_ONE_POINT_FIVE:
            cflag |= (termios.CSTOPB)  # XXX same as TWO.. there is no POSIX support for 1.5
        elif self._stopbits == serial.STOPBITS_TWO:
            cflag |= (termios.CSTOPB)
        else:
            raise ValueError('Invalid stop bit specification: {!r}'.format(self._stopbits))
        # setup parity
        iflag &= ~(termios.INPCK | termios.ISTRIP)
        if self._parity == serial.PARITY_NONE:
            cflag &= ~(termios.PARENB | termios.PARODD | CMSPAR)
        elif self._parity == serial.PARITY_EVEN:
            cflag &= ~(termios.PARODD | CMSPAR)
            cflag |= (termios.PARENB)
        elif self._parity == serial.PARITY_ODD:
            cflag &= ~CMSPAR
            cflag |= (termios.PARENB | termios.PARODD)
        elif self._parity == serial.PARITY_MARK and CMSPAR:
            cflag |= (termios.PARENB | CMSPAR | termios.PARODD)
        elif self._parity == serial.PARITY_SPACE and CMSPAR:
            cflag |= (termios.PARENB | CMSPAR)
            cflag &= ~(termios.PARODD)
        else:
            raise ValueError('Invalid parity: {!r}'.format(self._parity))
        # setup flow control
        # xonxoff
        if hasattr(termios, 'IXANY'):
            if self._xonxoff:
                iflag |= (termios.IXON | termios.IXOFF)  # |termios.IXANY)
            else:
                iflag &= ~(termios.IXON | termios.IXOFF | termios.IXANY)
        else:
            if self._xonxoff:
                iflag |= (termios.IXON | termios.IXOFF)
            else:
                iflag &= ~(termios.IXON | termios.IXOFF)
        # rtscts
        if hasattr(termios, 'CRTSCTS'):
            if self._rtscts:
                cflag |= (termios.CRTSCTS)
            else:
                cflag &= ~(termios.CRTSCTS)
        elif hasattr(termios, 'CNEW_RTSCTS'):   # try it with alternate constant name
            if self._rtscts:
                cflag |= (termios.CNEW_RTSCTS)
            else:
                cflag &= ~(termios.CNEW_RTSCTS)
        # XXX should there be a warning if setting up rtscts (and xonxoff etc) fails??

        # buffer
        # vmin "minimal number of characters to be read. 0 for non blocking"
        if vmin < 0 or vmin > 255:
            raise ValueError('Invalid vmin: {!r}'.format(vmin))
        cc[termios.VMIN] = vmin
        # vtime
        if vtime < 0 or vtime > 255:
            raise ValueError('Invalid vtime: {!r}'.format(vtime))
        cc[termios.VTIME] = vtime
        # activate settings
        if force_update or [iflag, oflag, cflag, lflag, ispeed, ospeed, cc] != orig_attr:
            termios.tcsetattr(
                self.fd,
                termios.TCSANOW,
                [iflag, oflag, cflag, lflag, ispeed, ospeed, cc])

        # apply custom baud rate, if any
        if custom_baud is not None:
            self._set_special_baudrate(custom_baud)

        if self._rs485_mode is not None:
            self._set_rs485_mode(self._rs485_mode)

    def close(self):
        """Close port"""
        if self.is_open:
            if self.fd is not None:
                os.close(self.fd)
                self.fd = None
                os.close(self.pipe_abort_read_w)
                os.close(self.pipe_abort_read_r)
                os.close(self.pipe_abort_write_w)
                os.close(self.pipe_abort_write_r)
                self.pipe_abort_read_r, self.pipe_abort_read_w = None, None
                self.pipe_abort_write_r, self.pipe_abort_write_w = None, None
            self.is_open = False

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    @property
    def in_waiting(self):
        """Return the number of bytes currently in the input buffer."""
        #~ s = fcntl.ioctl(self.fd, termios.FIONREAD, TIOCM_zero_str)
        s = fcntl.ioctl(self.fd, TIOCINQ, TIOCM_zero_str)
        return struct.unpack('I', s)[0]

    # select based implementation, proved to work on many systems
    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if not self.is_open:
            raise PortNotOpenError()
        read = bytearray()
        timeout = Timeout(self._timeout)
        while len(read) < size:
            try:
                ready, _, _ = select.select([self.fd, self.pipe_abort_read_r], [], [], timeout.time_left())
                if self.pipe_abort_read_r in ready:
                    os.read(self.pipe_abort_read_r, 1000)
                    break
                # If select was used with a timeout, and the timeout occurs, it
                # returns with empty lists -> thus abort read operation.
                # For timeout == 0 (non-blocking operation) also abort when
                # there is nothing to read.
                if not ready:
                    break   # timeout
                buf = os.read(self.fd, size - len(read))
            except OSError as e:
                # this is for Python 3.x where select.error is a subclass of
                # OSError ignore BlockingIOErrors and EINTR. other errors are shown
                # https://www.python.org/dev/peps/pep-0475.
                if e.errno not in (errno.EAGAIN, errno.EALREADY, errno.EWOULDBLOCK, errno.EINPROGRESS, errno.EINTR):
                    raise SerialException('read failed: {}'.format(e))
            except select.error as e:
                # this is for Python 2.x
                # ignore BlockingIOErrors and EINTR. all errors are shown
                # see also http://www.python.org/dev/peps/pep-3151/#select
                if e[0] not in (errno.EAGAIN, errno.EALREADY, errno.EWOULDBLOCK, errno.EINPROGRESS, errno.EINTR):
                    raise SerialException('read failed: {}'.format(e))
            else:
                # read should always return some data as select reported it was
                # ready to read when we get to this point.
                if not buf:
                    # Disconnected devices, at least on Linux, show the
                    # behavior that they are always ready to read immediately
                    # but reading returns nothing.
                    raise SerialException(
                        'device reports readiness to read but returned no data '
                        '(device disconnected or multiple access on port?)')
                read.extend(buf)

            if timeout.expired():
                break
        return bytes(read)

    def cancel_read(self):
        if self.is_open:
            os.write(self.pipe_abort_read_w, b"x")

    def cancel_write(self):
        if self.is_open:
            os.write(self.pipe_abort_write_w, b"x")

    def write(self, data):
        """Output the given byte string over the serial port."""
        if not self.is_open:
            raise PortNotOpenError()
        d = to_bytes(data)
        tx_len = length = len(d)
        timeout = Timeout(self._write_timeout)
        while tx_len > 0:
            try:
                n = os.write(self.fd, d)
                if timeout.is_non_blocking:
                    # Zero timeout indicates non-blocking - simply return the
                    # number of bytes of data actually written
                    return n
                elif not timeout.is_infinite:
                    # when timeout is set, use select to wait for being ready
                    # with the time left as timeout
                    if timeout.expired():
                        raise SerialTimeoutException('Write timeout')
                    abort, ready, _ = select.select([self.pipe_abort_write_r], [self.fd], [], timeout.time_left())
                    if abort:
                        os.read(self.pipe_abort_write_r, 1000)
                        break
                    if not ready:
                        raise SerialTimeoutException('Write timeout')
                else:
                    assert timeout.time_left() is None
                    # wait for write operation
                    abort, ready, _ = select.select([self.pipe_abort_write_r], [self.fd], [], None)
                    if abort:
                        os.read(self.pipe_abort_write_r, 1)
                        break
                    if not ready:
                        raise SerialException('write failed (select)')
                d = d[n:]
                tx_len -= n
            except SerialException:
                raise
            except OSError as e:
                # this is for Python 3.x where select.error is a subclass of
                # OSError ignore BlockingIOErrors and EINTR. other errors are shown
                # https://www.python.org/dev/peps/pep-0475.
                if e.errno not in (errno.EAGAIN, errno.EALREADY, errno.EWOULDBLOCK, errno.EINPROGRESS, errno.EINTR):
                    raise SerialException('write failed: {}'.format(e))
            except select.error as e:
                # this is for Python 2.x
                # ignore BlockingIOErrors and EINTR. all errors are shown
                # see also http://www.python.org/dev/peps/pep-3151/#select
                if e[0] not in (errno.EAGAIN, errno.EALREADY, errno.EWOULDBLOCK, errno.EINPROGRESS, errno.EINTR):
                    raise SerialException('write failed: {}'.format(e))
            if not timeout.is_non_blocking and timeout.expired():
                raise SerialTimeoutException('Write timeout')
        return length - len(d)

    def flush(self):
        """\
        Flush of file like objects. In this case, wait until all data
        is written.
        """
        if not self.is_open:
            raise PortNotOpenError()
        termios.tcdrain(self.fd)

    def _reset_input_buffer(self):
        """Clear input buffer, discarding all that is in the buffer."""
        termios.tcflush(self.fd, termios.TCIFLUSH)

    def reset_input_buffer(self):
        """Clear input buffer, discarding all that is in the buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        self._reset_input_buffer()

    def reset_output_buffer(self):
        """\
        Clear output buffer, aborting the current output and discarding all
        that is in the buffer.
        """
        if not self.is_open:
            raise PortNotOpenError()
        termios.tcflush(self.fd, termios.TCOFLUSH)

    def send_break(self, duration=0.25):
        """\
        Send break condition. Timed, returns to idle state after given
        duration.
        """
        if not self.is_open:
            raise PortNotOpenError()
        termios.tcsendbreak(self.fd, int(duration / 0.25))

    def _update_rts_state(self):
        """Set terminal status line: Request To Send"""
        if self._rts_state:
            fcntl.ioctl(self.fd, TIOCMBIS, TIOCM_RTS_str)
        else:
            fcntl.ioctl(self.fd, TIOCMBIC, TIOCM_RTS_str)

    def _update_dtr_state(self):
        """Set terminal status line: Data Terminal Ready"""
        if self._dtr_state:
            fcntl.ioctl(self.fd, TIOCMBIS, TIOCM_DTR_str)
        else:
            fcntl.ioctl(self.fd, TIOCMBIC, TIOCM_DTR_str)

    @property
    def cts(self):
        """Read terminal status line: Clear To Send"""
        if not self.is_open:
            raise PortNotOpenError()
        s = fcntl.ioctl(self.fd, TIOCMGET, TIOCM_zero_str)
        return struct.unpack('I', s)[0] & TIOCM_CTS != 0

    @property
    def dsr(self):
        """Read terminal status line: Data Set Ready"""
        if not self.is_open:
            raise PortNotOpenError()
        s = fcntl.ioctl(self.fd, TIOCMGET, TIOCM_zero_str)
        return struct.unpack('I', s)[0] & TIOCM_DSR != 0

    @property
    def ri(self):
        """Read terminal status line: Ring Indicator"""
        if not self.is_open:
            raise PortNotOpenError()
        s = fcntl.ioctl(self.fd, TIOCMGET, TIOCM_zero_str)
        return struct.unpack('I', s)[0] & TIOCM_RI != 0

    @property
    def cd(self):
        """Read terminal status line: Carrier Detect"""
        if not self.is_open:
            raise PortNotOpenError()
        s = fcntl.ioctl(self.fd, TIOCMGET, TIOCM_zero_str)
        return struct.unpack('I', s)[0] & TIOCM_CD != 0

    # - - platform specific - - - -

    @property
    def out_waiting(self):
        """Return the number of bytes currently in the output buffer."""
        #~ s = fcntl.ioctl(self.fd, termios.FIONREAD, TIOCM_zero_str)
        s = fcntl.ioctl(self.fd, TIOCOUTQ, TIOCM_zero_str)
        return struct.unpack('I', s)[0]

    def fileno(self):
        """\
        For easier use of the serial port instance with select.
        WARNING: this function is not portable to different platforms!
        """
        if not self.is_open:
            raise PortNotOpenError()
        return self.fd

    def set_input_flow_control(self, enable=True):
        """\
        Manually control flow - when software flow control is enabled.
        This will send XON (true) or XOFF (false) to the other device.
        WARNING: this function is not portable to different platforms!
        """
        if not self.is_open:
            raise PortNotOpenError()
        if enable:
            termios.tcflow(self.fd, termios.TCION)
        else:
            termios.tcflow(self.fd, termios.TCIOFF)

    def set_output_flow_control(self, enable=True):
        """\
        Manually control flow of outgoing data - when hardware or software flow
        control is enabled.
        WARNING: this function is not portable to different platforms!
        """
        if not self.is_open:
            raise PortNotOpenError()
        if enable:
            termios.tcflow(self.fd, termios.TCOON)
        else:
            termios.tcflow(self.fd, termios.TCOOFF)

    def nonblocking(self):
        """DEPRECATED - has no use"""
        import warnings
        warnings.warn("nonblocking() has no effect, already nonblocking", DeprecationWarning)


class PosixPollSerial(Serial):
    """\
    Poll based read implementation. Not all systems support poll properly.
    However this one has better handling of errors, such as a device
    disconnecting while it's in use (e.g. USB-serial unplugged).
    """

    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if not self.is_open:
            raise PortNotOpenError()
        read = bytearray()
        timeout = Timeout(self._timeout)
        poll = select.poll()
        poll.register(self.fd, select.POLLIN | select.POLLERR | select.POLLHUP | select.POLLNVAL)
        poll.register(self.pipe_abort_read_r, select.POLLIN | select.POLLERR | select.POLLHUP | select.POLLNVAL)
        if size > 0:
            while len(read) < size:
                # print "\tread(): size",size, "have", len(read)    #debug
                # wait until device becomes ready to read (or something fails)
                for fd, event in poll.poll(None if timeout.is_infinite else (timeout.time_left() * 1000)):
                    if fd == self.pipe_abort_read_r:
                        break
                    if event & (select.POLLERR | select.POLLHUP | select.POLLNVAL):
                        raise SerialException('device reports error (poll)')
                    #  we don't care if it is select.POLLIN or timeout, that's
                    #  handled below
                if fd == self.pipe_abort_read_r:
                    os.read(self.pipe_abort_read_r, 1000)
                    break
                buf = os.read(self.fd, size - len(read))
                read.extend(buf)
                if timeout.expired() \
                        or (self._inter_byte_timeout is not None and self._inter_byte_timeout > 0) and not buf:
                    break   # early abort on timeout
        return bytes(read)


class VTIMESerial(Serial):
    """\
    Implement timeout using vtime of tty device instead of using select.
    This means that no inter character timeout can be specified and that
    the error handling is degraded.

    Overall timeout is disabled when inter-character timeout is used.

    Note that this implementation does NOT support cancel_read(), it will
    just ignore that.
    """

    def _reconfigure_port(self, force_update=True):
        """Set communication parameters on opened port."""
        super(VTIMESerial, self)._reconfigure_port()
        fcntl.fcntl(self.fd, fcntl.F_SETFL, 0)  # clear O_NONBLOCK

        if self._inter_byte_timeout is not None:
            vmin = 1
            vtime = int(self._inter_byte_timeout * 10)
        elif self._timeout is None:
            vmin = 1
            vtime = 0
        else:
            vmin = 0
            vtime = int(self._timeout * 10)
        try:
            orig_attr = termios.tcgetattr(self.fd)
            iflag, oflag, cflag, lflag, ispeed, ospeed, cc = orig_attr
        except termios.error as msg:      # if a port is nonexistent but has a /dev file, it'll fail here
            raise serial.SerialException("Could not configure port: {}".format(msg))

        if vtime < 0 or vtime > 255:
            raise ValueError('Invalid vtime: {!r}'.format(vtime))
        cc[termios.VTIME] = vtime
        cc[termios.VMIN] = vmin

        termios.tcsetattr(
                self.fd,
                termios.TCSANOW,
                [iflag, oflag, cflag, lflag, ispeed, ospeed, cc])

    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if not self.is_open:
            raise PortNotOpenError()
        read = bytearray()
        while len(read) < size:
            buf = os.read(self.fd, size - len(read))
            if not buf:
                break
            read.extend(buf)
        return bytes(read)

    # hack to make hasattr return false
    cancel_read = property()
