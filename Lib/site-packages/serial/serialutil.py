#! python
#
# Base class and support functions used by various backends.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2020 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

from __future__ import absolute_import

import io
import time

# ``memoryview`` was introduced in Python 2.7 and ``bytes(some_memoryview)``
# isn't returning the contents (very unfortunate). Therefore we need special
# cases and test for it. Ensure that there is a ``memoryview`` object for older
# Python versions. This is easier than making every test dependent on its
# existence.
try:
    memoryview
except (NameError, AttributeError):
    # implementation does not matter as we do not really use it.
    # it just must not inherit from something else we might care for.
    class memoryview(object):   # pylint: disable=redefined-builtin,invalid-name
        pass

try:
    unicode
except (NameError, AttributeError):
    unicode = str       # for Python 3, pylint: disable=redefined-builtin,invalid-name

try:
    basestring
except (NameError, AttributeError):
    basestring = (str,)    # for Python 3, pylint: disable=redefined-builtin,invalid-name


# "for byte in data" fails for python3 as it returns ints instead of bytes
def iterbytes(b):
    """Iterate over bytes, returning bytes instead of ints (python3)"""
    if isinstance(b, memoryview):
        b = b.tobytes()
    i = 0
    while True:
        a = b[i:i + 1]
        i += 1
        if a:
            yield a
        else:
            break


# all Python versions prior 3.x convert ``str([17])`` to '[17]' instead of '\x11'
# so a simple ``bytes(sequence)`` doesn't work for all versions
def to_bytes(seq):
    """convert a sequence to a bytes type"""
    if isinstance(seq, bytes):
        return seq
    elif isinstance(seq, bytearray):
        return bytes(seq)
    elif isinstance(seq, memoryview):
        return seq.tobytes()
    elif isinstance(seq, unicode):
        raise TypeError('unicode strings are not supported, please encode to bytes: {!r}'.format(seq))
    else:
        # handle list of integers and bytes (one or more items) for Python 2 and 3
        return bytes(bytearray(seq))


# create control bytes
XON = to_bytes([17])
XOFF = to_bytes([19])

CR = to_bytes([13])
LF = to_bytes([10])


PARITY_NONE, PARITY_EVEN, PARITY_ODD, PARITY_MARK, PARITY_SPACE = 'N', 'E', 'O', 'M', 'S'
STOPBITS_ONE, STOPBITS_ONE_POINT_FIVE, STOPBITS_TWO = (1, 1.5, 2)
FIVEBITS, SIXBITS, SEVENBITS, EIGHTBITS = (5, 6, 7, 8)

PARITY_NAMES = {
    PARITY_NONE: 'None',
    PARITY_EVEN: 'Even',
    PARITY_ODD: 'Odd',
    PARITY_MARK: 'Mark',
    PARITY_SPACE: 'Space',
}


class SerialException(IOError):
    """Base class for serial port related exceptions."""


class SerialTimeoutException(SerialException):
    """Write timeouts give an exception"""


class PortNotOpenError(SerialException):
    """Port is not open"""
    def __init__(self):
        super(PortNotOpenError, self).__init__('Attempting to use a port that is not open')


class Timeout(object):
    """\
    Abstraction for timeout operations. Using time.monotonic() if available
    or time.time() in all other cases.

    The class can also be initialized with 0 or None, in order to support
    non-blocking and fully blocking I/O operations. The attributes
    is_non_blocking and is_infinite are set accordingly.
    """
    if hasattr(time, 'monotonic'):
        # Timeout implementation with time.monotonic(). This function is only
        # supported by Python 3.3 and above. It returns a time in seconds
        # (float) just as time.time(), but is not affected by system clock
        # adjustments.
        TIME = time.monotonic
    else:
        # Timeout implementation with time.time(). This is compatible with all
        # Python versions but has issues if the clock is adjusted while the
        # timeout is running.
        TIME = time.time

    def __init__(self, duration):
        """Initialize a timeout with given duration"""
        self.is_infinite = (duration is None)
        self.is_non_blocking = (duration == 0)
        self.duration = duration
        if duration is not None:
            self.target_time = self.TIME() + duration
        else:
            self.target_time = None

    def expired(self):
        """Return a boolean, telling if the timeout has expired"""
        return self.target_time is not None and self.time_left() <= 0

    def time_left(self):
        """Return how many seconds are left until the timeout expires"""
        if self.is_non_blocking:
            return 0
        elif self.is_infinite:
            return None
        else:
            delta = self.target_time - self.TIME()
            if delta > self.duration:
                # clock jumped, recalculate
                self.target_time = self.TIME() + self.duration
                return self.duration
            else:
                return max(0, delta)

    def restart(self, duration):
        """\
        Restart a timeout, only supported if a timeout was already set up
        before.
        """
        self.duration = duration
        self.target_time = self.TIME() + duration


class SerialBase(io.RawIOBase):
    """\
    Serial port base class. Provides __init__ function and properties to
    get/set port settings.
    """

    # default values, may be overridden in subclasses that do not support all values
    BAUDRATES = (50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
                 9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000,
                 576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000,
                 3000000, 3500000, 4000000)
    BYTESIZES = (FIVEBITS, SIXBITS, SEVENBITS, EIGHTBITS)
    PARITIES = (PARITY_NONE, PARITY_EVEN, PARITY_ODD, PARITY_MARK, PARITY_SPACE)
    STOPBITS = (STOPBITS_ONE, STOPBITS_ONE_POINT_FIVE, STOPBITS_TWO)

    def __init__(self,
                 port=None,
                 baudrate=9600,
                 bytesize=EIGHTBITS,
                 parity=PARITY_NONE,
                 stopbits=STOPBITS_ONE,
                 timeout=None,
                 xonxoff=False,
                 rtscts=False,
                 write_timeout=None,
                 dsrdtr=False,
                 inter_byte_timeout=None,
                 exclusive=None,
                 **kwargs):
        """\
        Initialize comm port object. If a "port" is given, then the port will be
        opened immediately. Otherwise a Serial port object in closed state
        is returned.
        """

        self.is_open = False
        self.portstr = None
        self.name = None
        # correct values are assigned below through properties
        self._port = None
        self._baudrate = None
        self._bytesize = None
        self._parity = None
        self._stopbits = None
        self._timeout = None
        self._write_timeout = None
        self._xonxoff = None
        self._rtscts = None
        self._dsrdtr = None
        self._inter_byte_timeout = None
        self._rs485_mode = None  # disabled by default
        self._rts_state = True
        self._dtr_state = True
        self._break_state = False
        self._exclusive = None

        # assign values using get/set methods using the properties feature
        self.port = port
        self.baudrate = baudrate
        self.bytesize = bytesize
        self.parity = parity
        self.stopbits = stopbits
        self.timeout = timeout
        self.write_timeout = write_timeout
        self.xonxoff = xonxoff
        self.rtscts = rtscts
        self.dsrdtr = dsrdtr
        self.inter_byte_timeout = inter_byte_timeout
        self.exclusive = exclusive

        # watch for backward compatible kwargs
        if 'writeTimeout' in kwargs:
            self.write_timeout = kwargs.pop('writeTimeout')
        if 'interCharTimeout' in kwargs:
            self.inter_byte_timeout = kwargs.pop('interCharTimeout')
        if kwargs:
            raise ValueError('unexpected keyword arguments: {!r}'.format(kwargs))

        if port is not None:
            self.open()

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    # to be implemented by subclasses:
    # def open(self):
    # def close(self):

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    @property
    def port(self):
        """\
        Get the current port setting. The value that was passed on init or using
        setPort() is passed back.
        """
        return self._port

    @port.setter
    def port(self, port):
        """\
        Change the port.
        """
        if port is not None and not isinstance(port, basestring):
            raise ValueError('"port" must be None or a string, not {}'.format(type(port)))
        was_open = self.is_open
        if was_open:
            self.close()
        self.portstr = port
        self._port = port
        self.name = self.portstr
        if was_open:
            self.open()

    @property
    def baudrate(self):
        """Get the current baud rate setting."""
        return self._baudrate

    @baudrate.setter
    def baudrate(self, baudrate):
        """\
        Change baud rate. It raises a ValueError if the port is open and the
        baud rate is not possible. If the port is closed, then the value is
        accepted and the exception is raised when the port is opened.
        """
        try:
            b = int(baudrate)
        except TypeError:
            raise ValueError("Not a valid baudrate: {!r}".format(baudrate))
        else:
            if b < 0:
                raise ValueError("Not a valid baudrate: {!r}".format(baudrate))
            self._baudrate = b
            if self.is_open:
                self._reconfigure_port()

    @property
    def bytesize(self):
        """Get the current byte size setting."""
        return self._bytesize

    @bytesize.setter
    def bytesize(self, bytesize):
        """Change byte size."""
        if bytesize not in self.BYTESIZES:
            raise ValueError("Not a valid byte size: {!r}".format(bytesize))
        self._bytesize = bytesize
        if self.is_open:
            self._reconfigure_port()

    @property
    def exclusive(self):
        """Get the current exclusive access setting."""
        return self._exclusive

    @exclusive.setter
    def exclusive(self, exclusive):
        """Change the exclusive access setting."""
        self._exclusive = exclusive
        if self.is_open:
            self._reconfigure_port()

    @property
    def parity(self):
        """Get the current parity setting."""
        return self._parity

    @parity.setter
    def parity(self, parity):
        """Change parity setting."""
        if parity not in self.PARITIES:
            raise ValueError("Not a valid parity: {!r}".format(parity))
        self._parity = parity
        if self.is_open:
            self._reconfigure_port()

    @property
    def stopbits(self):
        """Get the current stop bits setting."""
        return self._stopbits

    @stopbits.setter
    def stopbits(self, stopbits):
        """Change stop bits size."""
        if stopbits not in self.STOPBITS:
            raise ValueError("Not a valid stop bit size: {!r}".format(stopbits))
        self._stopbits = stopbits
        if self.is_open:
            self._reconfigure_port()

    @property
    def timeout(self):
        """Get the current timeout setting."""
        return self._timeout

    @timeout.setter
    def timeout(self, timeout):
        """Change timeout setting."""
        if timeout is not None:
            try:
                timeout + 1     # test if it's a number, will throw a TypeError if not...
            except TypeError:
                raise ValueError("Not a valid timeout: {!r}".format(timeout))
            if timeout < 0:
                raise ValueError("Not a valid timeout: {!r}".format(timeout))
        self._timeout = timeout
        if self.is_open:
            self._reconfigure_port()

    @property
    def write_timeout(self):
        """Get the current timeout setting."""
        return self._write_timeout

    @write_timeout.setter
    def write_timeout(self, timeout):
        """Change timeout setting."""
        if timeout is not None:
            if timeout < 0:
                raise ValueError("Not a valid timeout: {!r}".format(timeout))
            try:
                timeout + 1     # test if it's a number, will throw a TypeError if not...
            except TypeError:
                raise ValueError("Not a valid timeout: {!r}".format(timeout))

        self._write_timeout = timeout
        if self.is_open:
            self._reconfigure_port()

    @property
    def inter_byte_timeout(self):
        """Get the current inter-character timeout setting."""
        return self._inter_byte_timeout

    @inter_byte_timeout.setter
    def inter_byte_timeout(self, ic_timeout):
        """Change inter-byte timeout setting."""
        if ic_timeout is not None:
            if ic_timeout < 0:
                raise ValueError("Not a valid timeout: {!r}".format(ic_timeout))
            try:
                ic_timeout + 1     # test if it's a number, will throw a TypeError if not...
            except TypeError:
                raise ValueError("Not a valid timeout: {!r}".format(ic_timeout))

        self._inter_byte_timeout = ic_timeout
        if self.is_open:
            self._reconfigure_port()

    @property
    def xonxoff(self):
        """Get the current XON/XOFF setting."""
        return self._xonxoff

    @xonxoff.setter
    def xonxoff(self, xonxoff):
        """Change XON/XOFF setting."""
        self._xonxoff = xonxoff
        if self.is_open:
            self._reconfigure_port()

    @property
    def rtscts(self):
        """Get the current RTS/CTS flow control setting."""
        return self._rtscts

    @rtscts.setter
    def rtscts(self, rtscts):
        """Change RTS/CTS flow control setting."""
        self._rtscts = rtscts
        if self.is_open:
            self._reconfigure_port()

    @property
    def dsrdtr(self):
        """Get the current DSR/DTR flow control setting."""
        return self._dsrdtr

    @dsrdtr.setter
    def dsrdtr(self, dsrdtr=None):
        """Change DsrDtr flow control setting."""
        if dsrdtr is None:
            # if not set, keep backwards compatibility and follow rtscts setting
            self._dsrdtr = self._rtscts
        else:
            # if defined independently, follow its value
            self._dsrdtr = dsrdtr
        if self.is_open:
            self._reconfigure_port()

    @property
    def rts(self):
        return self._rts_state

    @rts.setter
    def rts(self, value):
        self._rts_state = value
        if self.is_open:
            self._update_rts_state()

    @property
    def dtr(self):
        return self._dtr_state

    @dtr.setter
    def dtr(self, value):
        self._dtr_state = value
        if self.is_open:
            self._update_dtr_state()

    @property
    def break_condition(self):
        return self._break_state

    @break_condition.setter
    def break_condition(self, value):
        self._break_state = value
        if self.is_open:
            self._update_break_state()

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    # functions useful for RS-485 adapters

    @property
    def rs485_mode(self):
        """\
        Enable RS485 mode and apply new settings, set to None to disable.
        See serial.rs485.RS485Settings for more info about the value.
        """
        return self._rs485_mode

    @rs485_mode.setter
    def rs485_mode(self, rs485_settings):
        self._rs485_mode = rs485_settings
        if self.is_open:
            self._reconfigure_port()

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    _SAVED_SETTINGS = ('baudrate', 'bytesize', 'parity', 'stopbits', 'xonxoff',
                       'dsrdtr', 'rtscts', 'timeout', 'write_timeout',
                       'inter_byte_timeout')

    def get_settings(self):
        """\
        Get current port settings as a dictionary. For use with
        apply_settings().
        """
        return dict([(key, getattr(self, '_' + key)) for key in self._SAVED_SETTINGS])

    def apply_settings(self, d):
        """\
        Apply stored settings from a dictionary returned from
        get_settings(). It's allowed to delete keys from the dictionary. These
        values will simply left unchanged.
        """
        for key in self._SAVED_SETTINGS:
            if key in d and d[key] != getattr(self, '_' + key):   # check against internal "_" value
                setattr(self, key, d[key])          # set non "_" value to use properties write function

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    def __repr__(self):
        """String representation of the current port settings and its state."""
        return '{name}<id=0x{id:x}, open={p.is_open}>(port={p.portstr!r}, ' \
               'baudrate={p.baudrate!r}, bytesize={p.bytesize!r}, parity={p.parity!r}, ' \
               'stopbits={p.stopbits!r}, timeout={p.timeout!r}, xonxoff={p.xonxoff!r}, ' \
               'rtscts={p.rtscts!r}, dsrdtr={p.dsrdtr!r})'.format(
                   name=self.__class__.__name__, id=id(self), p=self)

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    # compatibility with io library
    # pylint: disable=invalid-name,missing-docstring

    def readable(self):
        return True

    def writable(self):
        return True

    def seekable(self):
        return False

    def readinto(self, b):
        data = self.read(len(b))
        n = len(data)
        try:
            b[:n] = data
        except TypeError as err:
            import array
            if not isinstance(b, array.array):
                raise err
            b[:n] = array.array('b', data)
        return n

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    # context manager

    def __enter__(self):
        if self._port is not None and not self.is_open:
            self.open()
        return self

    def __exit__(self, *args, **kwargs):
        self.close()

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    def send_break(self, duration=0.25):
        """\
        Send break condition. Timed, returns to idle state after given
        duration.
        """
        if not self.is_open:
            raise PortNotOpenError()
        self.break_condition = True
        time.sleep(duration)
        self.break_condition = False

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    # backwards compatibility / deprecated functions

    def flushInput(self):
        self.reset_input_buffer()

    def flushOutput(self):
        self.reset_output_buffer()

    def inWaiting(self):
        return self.in_waiting

    def sendBreak(self, duration=0.25):
        self.send_break(duration)

    def setRTS(self, value=1):
        self.rts = value

    def setDTR(self, value=1):
        self.dtr = value

    def getCTS(self):
        return self.cts

    def getDSR(self):
        return self.dsr

    def getRI(self):
        return self.ri

    def getCD(self):
        return self.cd

    def setPort(self, port):
        self.port = port

    @property
    def writeTimeout(self):
        return self.write_timeout

    @writeTimeout.setter
    def writeTimeout(self, timeout):
        self.write_timeout = timeout

    @property
    def interCharTimeout(self):
        return self.inter_byte_timeout

    @interCharTimeout.setter
    def interCharTimeout(self, interCharTimeout):
        self.inter_byte_timeout = interCharTimeout

    def getSettingsDict(self):
        return self.get_settings()

    def applySettingsDict(self, d):
        self.apply_settings(d)

    def isOpen(self):
        return self.is_open

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    # additional functionality

    def read_all(self):
        """\
        Read all bytes currently available in the buffer of the OS.
        """
        return self.read(self.in_waiting)

    def read_until(self, expected=LF, size=None):
        """\
        Read until an expected sequence is found ('\n' by default), the size
        is exceeded or until timeout occurs.
        """
        lenterm = len(expected)
        line = bytearray()
        timeout = Timeout(self._timeout)
        while True:
            c = self.read(1)
            if c:
                line += c
                if line[-lenterm:] == expected:
                    break
                if size is not None and len(line) >= size:
                    break
            else:
                break
            if timeout.expired():
                break
        return bytes(line)

    def iread_until(self, *args, **kwargs):
        """\
        Read lines, implemented as generator. It will raise StopIteration on
        timeout (empty read).
        """
        while True:
            line = self.read_until(*args, **kwargs)
            if not line:
                break
            yield line


#  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
if __name__ == '__main__':
    import sys
    s = SerialBase()
    sys.stdout.write('port name:  {}\n'.format(s.name))
    sys.stdout.write('baud rates: {}\n'.format(s.BAUDRATES))
    sys.stdout.write('byte sizes: {}\n'.format(s.BYTESIZES))
    sys.stdout.write('parities:   {}\n'.format(s.PARITIES))
    sys.stdout.write('stop bits:  {}\n'.format(s.STOPBITS))
    sys.stdout.write('{}\n'.format(s))
