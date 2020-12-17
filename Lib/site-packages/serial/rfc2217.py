#! python
#
# This module implements a RFC2217 compatible client. RF2217 descibes a
# protocol to access serial ports over TCP/IP and allows setting the baud rate,
# modem control lines etc.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2001-2015 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

# TODO:
# - setting control line -> answer is not checked (had problems with one of the
#   severs). consider implementing a compatibility mode flag to make check
#   conditional
# - write timeout not implemented at all

# ###########################################################################
# observations and issues with servers
# ===========================================================================
# sredird V2.2.1
# - http://www.ibiblio.org/pub/Linux/system/serial/   sredird-2.2.2.tar.gz
# - does not acknowledge SET_CONTROL (RTS/DTR) correctly, always responding
#   [105 1] instead of the actual value.
# - SET_BAUDRATE answer contains 4 extra null bytes -> probably for larger
#   numbers than 2**32?
# - To get the signature [COM_PORT_OPTION 0] has to be sent.
# - run a server: while true; do nc -l -p 7000 -c "sredird debug /dev/ttyUSB0 /var/lock/sredir"; done
# ===========================================================================
# telnetcpcd (untested)
# - http://ftp.wayne.edu/kermit/sredird/telnetcpcd-1.09.tar.gz
# - To get the signature [COM_PORT_OPTION] w/o data has to be sent.
# ===========================================================================
# ser2net
# - does not negotiate BINARY or COM_PORT_OPTION for his side but at least
#   acknowledges that the client activates these options
# - The configuration may be that the server prints a banner. As this client
#   implementation does a flushInput on connect, this banner is hidden from
#   the user application.
# - NOTIFY_MODEMSTATE: the poll interval of the server seems to be one
#   second.
# - To get the signature [COM_PORT_OPTION 0] has to be sent.
# - run a server: run ser2net daemon, in /etc/ser2net.conf:
#     2000:telnet:0:/dev/ttyS0:9600 remctl banner
# ###########################################################################

# How to identify ports? pySerial might want to support other protocols in the
# future, so lets use an URL scheme.
# for RFC2217 compliant servers we will use this:
#    rfc2217://<host>:<port>[?option[&option...]]
#
# options:
# - "logging" set log level print diagnostic messages (e.g. "logging=debug")
# - "ign_set_control": do not look at the answers to SET_CONTROL
# - "poll_modem": issue NOTIFY_MODEMSTATE requests when CTS/DTR/RI/CD is read.
#   Without this option it expects that the server sends notifications
#   automatically on change (which most servers do and is according to the
#   RFC).
# the order of the options is not relevant

from __future__ import absolute_import

import logging
import socket
import struct
import threading
import time
try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse
try:
    import Queue
except ImportError:
    import queue as Queue

import serial
from serial.serialutil import SerialBase, SerialException, to_bytes, \
    iterbytes, PortNotOpenError, Timeout

# port string is expected to be something like this:
# rfc2217://host:port
# host may be an IP or including domain, whatever.
# port is 0...65535

# map log level names to constants. used in from_url()
LOGGER_LEVELS = {
    'debug': logging.DEBUG,
    'info': logging.INFO,
    'warning': logging.WARNING,
    'error': logging.ERROR,
}


# telnet protocol characters
SE = b'\xf0'    # Subnegotiation End
NOP = b'\xf1'   # No Operation
DM = b'\xf2'    # Data Mark
BRK = b'\xf3'   # Break
IP = b'\xf4'    # Interrupt process
AO = b'\xf5'    # Abort output
AYT = b'\xf6'   # Are You There
EC = b'\xf7'    # Erase Character
EL = b'\xf8'    # Erase Line
GA = b'\xf9'    # Go Ahead
SB = b'\xfa'    # Subnegotiation Begin
WILL = b'\xfb'
WONT = b'\xfc'
DO = b'\xfd'
DONT = b'\xfe'
IAC = b'\xff'   # Interpret As Command
IAC_DOUBLED = b'\xff\xff'

# selected telnet options
BINARY = b'\x00'    # 8-bit data path
ECHO = b'\x01'      # echo
SGA = b'\x03'       # suppress go ahead

# RFC2217
COM_PORT_OPTION = b'\x2c'

# Client to Access Server
SET_BAUDRATE = b'\x01'
SET_DATASIZE = b'\x02'
SET_PARITY = b'\x03'
SET_STOPSIZE = b'\x04'
SET_CONTROL = b'\x05'
NOTIFY_LINESTATE = b'\x06'
NOTIFY_MODEMSTATE = b'\x07'
FLOWCONTROL_SUSPEND = b'\x08'
FLOWCONTROL_RESUME = b'\x09'
SET_LINESTATE_MASK = b'\x0a'
SET_MODEMSTATE_MASK = b'\x0b'
PURGE_DATA = b'\x0c'

SERVER_SET_BAUDRATE = b'\x65'
SERVER_SET_DATASIZE = b'\x66'
SERVER_SET_PARITY = b'\x67'
SERVER_SET_STOPSIZE = b'\x68'
SERVER_SET_CONTROL = b'\x69'
SERVER_NOTIFY_LINESTATE = b'\x6a'
SERVER_NOTIFY_MODEMSTATE = b'\x6b'
SERVER_FLOWCONTROL_SUSPEND = b'\x6c'
SERVER_FLOWCONTROL_RESUME = b'\x6d'
SERVER_SET_LINESTATE_MASK = b'\x6e'
SERVER_SET_MODEMSTATE_MASK = b'\x6f'
SERVER_PURGE_DATA = b'\x70'

RFC2217_ANSWER_MAP = {
    SET_BAUDRATE: SERVER_SET_BAUDRATE,
    SET_DATASIZE: SERVER_SET_DATASIZE,
    SET_PARITY: SERVER_SET_PARITY,
    SET_STOPSIZE: SERVER_SET_STOPSIZE,
    SET_CONTROL: SERVER_SET_CONTROL,
    NOTIFY_LINESTATE: SERVER_NOTIFY_LINESTATE,
    NOTIFY_MODEMSTATE: SERVER_NOTIFY_MODEMSTATE,
    FLOWCONTROL_SUSPEND: SERVER_FLOWCONTROL_SUSPEND,
    FLOWCONTROL_RESUME: SERVER_FLOWCONTROL_RESUME,
    SET_LINESTATE_MASK: SERVER_SET_LINESTATE_MASK,
    SET_MODEMSTATE_MASK: SERVER_SET_MODEMSTATE_MASK,
    PURGE_DATA: SERVER_PURGE_DATA,
}

SET_CONTROL_REQ_FLOW_SETTING = b'\x00'        # Request Com Port Flow Control Setting (outbound/both)
SET_CONTROL_USE_NO_FLOW_CONTROL = b'\x01'     # Use No Flow Control (outbound/both)
SET_CONTROL_USE_SW_FLOW_CONTROL = b'\x02'     # Use XON/XOFF Flow Control (outbound/both)
SET_CONTROL_USE_HW_FLOW_CONTROL = b'\x03'     # Use HARDWARE Flow Control (outbound/both)
SET_CONTROL_REQ_BREAK_STATE = b'\x04'         # Request BREAK State
SET_CONTROL_BREAK_ON = b'\x05'                # Set BREAK State ON
SET_CONTROL_BREAK_OFF = b'\x06'               # Set BREAK State OFF
SET_CONTROL_REQ_DTR = b'\x07'                 # Request DTR Signal State
SET_CONTROL_DTR_ON = b'\x08'                  # Set DTR Signal State ON
SET_CONTROL_DTR_OFF = b'\x09'                 # Set DTR Signal State OFF
SET_CONTROL_REQ_RTS = b'\x0a'                 # Request RTS Signal State
SET_CONTROL_RTS_ON = b'\x0b'                  # Set RTS Signal State ON
SET_CONTROL_RTS_OFF = b'\x0c'                 # Set RTS Signal State OFF
SET_CONTROL_REQ_FLOW_SETTING_IN = b'\x0d'     # Request Com Port Flow Control Setting (inbound)
SET_CONTROL_USE_NO_FLOW_CONTROL_IN = b'\x0e'  # Use No Flow Control (inbound)
SET_CONTROL_USE_SW_FLOW_CONTOL_IN = b'\x0f'   # Use XON/XOFF Flow Control (inbound)
SET_CONTROL_USE_HW_FLOW_CONTOL_IN = b'\x10'   # Use HARDWARE Flow Control (inbound)
SET_CONTROL_USE_DCD_FLOW_CONTROL = b'\x11'    # Use DCD Flow Control (outbound/both)
SET_CONTROL_USE_DTR_FLOW_CONTROL = b'\x12'    # Use DTR Flow Control (inbound)
SET_CONTROL_USE_DSR_FLOW_CONTROL = b'\x13'    # Use DSR Flow Control (outbound/both)

LINESTATE_MASK_TIMEOUT = 128        # Time-out Error
LINESTATE_MASK_SHIFTREG_EMPTY = 64  # Transfer Shift Register Empty
LINESTATE_MASK_TRANSREG_EMPTY = 32  # Transfer Holding Register Empty
LINESTATE_MASK_BREAK_DETECT = 16    # Break-detect Error
LINESTATE_MASK_FRAMING_ERROR = 8    # Framing Error
LINESTATE_MASK_PARTIY_ERROR = 4     # Parity Error
LINESTATE_MASK_OVERRUN_ERROR = 2    # Overrun Error
LINESTATE_MASK_DATA_READY = 1       # Data Ready

MODEMSTATE_MASK_CD = 128            # Receive Line Signal Detect (also known as Carrier Detect)
MODEMSTATE_MASK_RI = 64             # Ring Indicator
MODEMSTATE_MASK_DSR = 32            # Data-Set-Ready Signal State
MODEMSTATE_MASK_CTS = 16            # Clear-To-Send Signal State
MODEMSTATE_MASK_CD_CHANGE = 8       # Delta Receive Line Signal Detect
MODEMSTATE_MASK_RI_CHANGE = 4       # Trailing-edge Ring Detector
MODEMSTATE_MASK_DSR_CHANGE = 2      # Delta Data-Set-Ready
MODEMSTATE_MASK_CTS_CHANGE = 1      # Delta Clear-To-Send

PURGE_RECEIVE_BUFFER = b'\x01'      # Purge access server receive data buffer
PURGE_TRANSMIT_BUFFER = b'\x02'     # Purge access server transmit data buffer
PURGE_BOTH_BUFFERS = b'\x03'        # Purge both the access server receive data
                                    # buffer and the access server transmit data buffer


RFC2217_PARITY_MAP = {
    serial.PARITY_NONE: 1,
    serial.PARITY_ODD: 2,
    serial.PARITY_EVEN: 3,
    serial.PARITY_MARK: 4,
    serial.PARITY_SPACE: 5,
}
RFC2217_REVERSE_PARITY_MAP = dict((v, k) for k, v in RFC2217_PARITY_MAP.items())

RFC2217_STOPBIT_MAP = {
    serial.STOPBITS_ONE: 1,
    serial.STOPBITS_ONE_POINT_FIVE: 3,
    serial.STOPBITS_TWO: 2,
}
RFC2217_REVERSE_STOPBIT_MAP = dict((v, k) for k, v in RFC2217_STOPBIT_MAP.items())

# Telnet filter states
M_NORMAL = 0
M_IAC_SEEN = 1
M_NEGOTIATE = 2

# TelnetOption and TelnetSubnegotiation states
REQUESTED = 'REQUESTED'
ACTIVE = 'ACTIVE'
INACTIVE = 'INACTIVE'
REALLY_INACTIVE = 'REALLY_INACTIVE'


class TelnetOption(object):
    """Manage a single telnet option, keeps track of DO/DONT WILL/WONT."""

    def __init__(self, connection, name, option, send_yes, send_no, ack_yes,
                 ack_no, initial_state, activation_callback=None):
        """\
        Initialize option.
        :param connection: connection used to transmit answers
        :param name: a readable name for debug outputs
        :param send_yes: what to send when option is to be enabled.
        :param send_no: what to send when option is to be disabled.
        :param ack_yes: what to expect when remote agrees on option.
        :param ack_no: what to expect when remote disagrees on option.
        :param initial_state: options initialized with REQUESTED are tried to
            be enabled on startup. use INACTIVE for all others.
        """
        self.connection = connection
        self.name = name
        self.option = option
        self.send_yes = send_yes
        self.send_no = send_no
        self.ack_yes = ack_yes
        self.ack_no = ack_no
        self.state = initial_state
        self.active = False
        self.activation_callback = activation_callback

    def __repr__(self):
        """String for debug outputs"""
        return "{o.name}:{o.active}({o.state})".format(o=self)

    def process_incoming(self, command):
        """\
        A DO/DONT/WILL/WONT was received for this option, update state and
        answer when needed.
        """
        if command == self.ack_yes:
            if self.state is REQUESTED:
                self.state = ACTIVE
                self.active = True
                if self.activation_callback is not None:
                    self.activation_callback()
            elif self.state is ACTIVE:
                pass
            elif self.state is INACTIVE:
                self.state = ACTIVE
                self.connection.telnet_send_option(self.send_yes, self.option)
                self.active = True
                if self.activation_callback is not None:
                    self.activation_callback()
            elif self.state is REALLY_INACTIVE:
                self.connection.telnet_send_option(self.send_no, self.option)
            else:
                raise ValueError('option in illegal state {!r}'.format(self))
        elif command == self.ack_no:
            if self.state is REQUESTED:
                self.state = INACTIVE
                self.active = False
            elif self.state is ACTIVE:
                self.state = INACTIVE
                self.connection.telnet_send_option(self.send_no, self.option)
                self.active = False
            elif self.state is INACTIVE:
                pass
            elif self.state is REALLY_INACTIVE:
                pass
            else:
                raise ValueError('option in illegal state {!r}'.format(self))


class TelnetSubnegotiation(object):
    """\
    A object to handle subnegotiation of options. In this case actually
    sub-sub options for RFC 2217. It is used to track com port options.
    """

    def __init__(self, connection, name, option, ack_option=None):
        if ack_option is None:
            ack_option = option
        self.connection = connection
        self.name = name
        self.option = option
        self.value = None
        self.ack_option = ack_option
        self.state = INACTIVE

    def __repr__(self):
        """String for debug outputs."""
        return "{sn.name}:{sn.state}".format(sn=self)

    def set(self, value):
        """\
        Request a change of the value. a request is sent to the server. if
        the client needs to know if the change is performed he has to check the
        state of this object.
        """
        self.value = value
        self.state = REQUESTED
        self.connection.rfc2217_send_subnegotiation(self.option, self.value)
        if self.connection.logger:
            self.connection.logger.debug("SB Requesting {} -> {!r}".format(self.name, self.value))

    def is_ready(self):
        """\
        Check if answer from server has been received. when server rejects
        the change, raise a ValueError.
        """
        if self.state == REALLY_INACTIVE:
            raise ValueError("remote rejected value for option {!r}".format(self.name))
        return self.state == ACTIVE
    # add property to have a similar interface as TelnetOption
    active = property(is_ready)

    def wait(self, timeout=3):
        """\
        Wait until the subnegotiation has been acknowledged or timeout. It
        can also throw a value error when the answer from the server does not
        match the value sent.
        """
        timeout_timer = Timeout(timeout)
        while not timeout_timer.expired():
            time.sleep(0.05)    # prevent 100% CPU load
            if self.is_ready():
                break
        else:
            raise SerialException("timeout while waiting for option {!r}".format(self.name))

    def check_answer(self, suboption):
        """\
        Check an incoming subnegotiation block. The parameter already has
        cut off the header like sub option number and com port option value.
        """
        if self.value == suboption[:len(self.value)]:
            self.state = ACTIVE
        else:
            # error propagation done in is_ready
            self.state = REALLY_INACTIVE
        if self.connection.logger:
            self.connection.logger.debug("SB Answer {} -> {!r} -> {}".format(self.name, suboption, self.state))


class Serial(SerialBase):
    """Serial port implementation for RFC 2217 remote serial ports."""

    BAUDRATES = (50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
                 9600, 19200, 38400, 57600, 115200)

    def __init__(self, *args, **kwargs):
        self._thread = None
        self._socket = None
        self._linestate = 0
        self._modemstate = None
        self._modemstate_timeout = Timeout(-1)
        self._remote_suspend_flow = False
        self._write_lock = None
        self.logger = None
        self._ignore_set_control_answer = False
        self._poll_modem_state = False
        self._network_timeout = 3
        self._telnet_options = None
        self._rfc2217_port_settings = None
        self._rfc2217_options = None
        self._read_buffer = None
        super(Serial, self).__init__(*args, **kwargs)  # must be last call in case of auto-open

    def open(self):
        """\
        Open port with current settings. This may throw a SerialException
        if the port cannot be opened.
        """
        self.logger = None
        self._ignore_set_control_answer = False
        self._poll_modem_state = False
        self._network_timeout = 3
        if self._port is None:
            raise SerialException("Port must be configured before it can be used.")
        if self.is_open:
            raise SerialException("Port is already open.")
        try:
            self._socket = socket.create_connection(self.from_url(self.portstr), timeout=5)  # XXX good value?
            self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        except Exception as msg:
            self._socket = None
            raise SerialException("Could not open port {}: {}".format(self.portstr, msg))

        # use a thread save queue as buffer. it also simplifies implementing
        # the read timeout
        self._read_buffer = Queue.Queue()
        # to ensure that user writes does not interfere with internal
        # telnet/rfc2217 options establish a lock
        self._write_lock = threading.Lock()
        # name the following separately so that, below, a check can be easily done
        mandadory_options = [
            TelnetOption(self, 'we-BINARY', BINARY, WILL, WONT, DO, DONT, INACTIVE),
            TelnetOption(self, 'we-RFC2217', COM_PORT_OPTION, WILL, WONT, DO, DONT, REQUESTED),
        ]
        # all supported telnet options
        self._telnet_options = [
            TelnetOption(self, 'ECHO', ECHO, DO, DONT, WILL, WONT, REQUESTED),
            TelnetOption(self, 'we-SGA', SGA, WILL, WONT, DO, DONT, REQUESTED),
            TelnetOption(self, 'they-SGA', SGA, DO, DONT, WILL, WONT, REQUESTED),
            TelnetOption(self, 'they-BINARY', BINARY, DO, DONT, WILL, WONT, INACTIVE),
            TelnetOption(self, 'they-RFC2217', COM_PORT_OPTION, DO, DONT, WILL, WONT, REQUESTED),
        ] + mandadory_options
        # RFC 2217 specific states
        # COM port settings
        self._rfc2217_port_settings = {
            'baudrate': TelnetSubnegotiation(self, 'baudrate', SET_BAUDRATE, SERVER_SET_BAUDRATE),
            'datasize': TelnetSubnegotiation(self, 'datasize', SET_DATASIZE, SERVER_SET_DATASIZE),
            'parity':   TelnetSubnegotiation(self, 'parity',   SET_PARITY,   SERVER_SET_PARITY),
            'stopsize': TelnetSubnegotiation(self, 'stopsize', SET_STOPSIZE, SERVER_SET_STOPSIZE),
        }
        # There are more subnegotiation objects, combine all in one dictionary
        # for easy access
        self._rfc2217_options = {
            'purge':    TelnetSubnegotiation(self, 'purge',    PURGE_DATA,   SERVER_PURGE_DATA),
            'control':  TelnetSubnegotiation(self, 'control',  SET_CONTROL,  SERVER_SET_CONTROL),
        }
        self._rfc2217_options.update(self._rfc2217_port_settings)
        # cache for line and modem states that the server sends to us
        self._linestate = 0
        self._modemstate = None
        self._modemstate_timeout = Timeout(-1)
        # RFC 2217 flow control between server and client
        self._remote_suspend_flow = False

        self.is_open = True
        self._thread = threading.Thread(target=self._telnet_read_loop)
        self._thread.setDaemon(True)
        self._thread.setName('pySerial RFC 2217 reader thread for {}'.format(self._port))
        self._thread.start()

        try:    # must clean-up if open fails
            # negotiate Telnet/RFC 2217 -> send initial requests
            for option in self._telnet_options:
                if option.state is REQUESTED:
                    self.telnet_send_option(option.send_yes, option.option)
            # now wait until important options are negotiated
            timeout = Timeout(self._network_timeout)
            while not timeout.expired():
                time.sleep(0.05)    # prevent 100% CPU load
                if sum(o.active for o in mandadory_options) == sum(o.state != INACTIVE for o in mandadory_options):
                    break
            else:
                raise SerialException(
                    "Remote does not seem to support RFC2217 or BINARY mode {!r}".format(mandadory_options))
            if self.logger:
                self.logger.info("Negotiated options: {}".format(self._telnet_options))

            # fine, go on, set RFC 2217 specific things
            self._reconfigure_port()
            # all things set up get, now a clean start
            if not self._dsrdtr:
                self._update_dtr_state()
            if not self._rtscts:
                self._update_rts_state()
            self.reset_input_buffer()
            self.reset_output_buffer()
        except:
            self.close()
            raise

    def _reconfigure_port(self):
        """Set communication parameters on opened port."""
        if self._socket is None:
            raise SerialException("Can only operate on open ports")

        # if self._timeout != 0 and self._interCharTimeout is not None:
            # XXX

        if self._write_timeout is not None:
            raise NotImplementedError('write_timeout is currently not supported')
            # XXX

        # Setup the connection
        # to get good performance, all parameter changes are sent first...
        if not 0 < self._baudrate < 2 ** 32:
            raise ValueError("invalid baudrate: {!r}".format(self._baudrate))
        self._rfc2217_port_settings['baudrate'].set(struct.pack(b'!I', self._baudrate))
        self._rfc2217_port_settings['datasize'].set(struct.pack(b'!B', self._bytesize))
        self._rfc2217_port_settings['parity'].set(struct.pack(b'!B', RFC2217_PARITY_MAP[self._parity]))
        self._rfc2217_port_settings['stopsize'].set(struct.pack(b'!B', RFC2217_STOPBIT_MAP[self._stopbits]))

        # and now wait until parameters are active
        items = self._rfc2217_port_settings.values()
        if self.logger:
            self.logger.debug("Negotiating settings: {}".format(items))
        timeout = Timeout(self._network_timeout)
        while not timeout.expired():
            time.sleep(0.05)    # prevent 100% CPU load
            if sum(o.active for o in items) == len(items):
                break
        else:
            raise SerialException("Remote does not accept parameter change (RFC2217): {!r}".format(items))
        if self.logger:
            self.logger.info("Negotiated settings: {}".format(items))

        if self._rtscts and self._xonxoff:
            raise ValueError('xonxoff and rtscts together are not supported')
        elif self._rtscts:
            self.rfc2217_set_control(SET_CONTROL_USE_HW_FLOW_CONTROL)
        elif self._xonxoff:
            self.rfc2217_set_control(SET_CONTROL_USE_SW_FLOW_CONTROL)
        else:
            self.rfc2217_set_control(SET_CONTROL_USE_NO_FLOW_CONTROL)

    def close(self):
        """Close port"""
        self.is_open = False
        if self._socket:
            try:
                self._socket.shutdown(socket.SHUT_RDWR)
                self._socket.close()
            except:
                # ignore errors.
                pass
        if self._thread:
            self._thread.join(7)  # XXX more than socket timeout
            self._thread = None
            # in case of quick reconnects, give the server some time
            time.sleep(0.3)
        self._socket = None

    def from_url(self, url):
        """\
        extract host and port from an URL string, other settings are extracted
        an stored in instance
        """
        parts = urlparse.urlsplit(url)
        if parts.scheme != "rfc2217":
            raise SerialException(
                'expected a string in the form '
                '"rfc2217://<host>:<port>[?option[&option...]]": '
                'not starting with rfc2217:// ({!r})'.format(parts.scheme))
        try:
            # process options now, directly altering self
            for option, values in urlparse.parse_qs(parts.query, True).items():
                if option == 'logging':
                    logging.basicConfig()   # XXX is that good to call it here?
                    self.logger = logging.getLogger('pySerial.rfc2217')
                    self.logger.setLevel(LOGGER_LEVELS[values[0]])
                    self.logger.debug('enabled logging')
                elif option == 'ign_set_control':
                    self._ignore_set_control_answer = True
                elif option == 'poll_modem':
                    self._poll_modem_state = True
                elif option == 'timeout':
                    self._network_timeout = float(values[0])
                else:
                    raise ValueError('unknown option: {!r}'.format(option))
            if not 0 <= parts.port < 65536:
                raise ValueError("port not in range 0...65535")
        except ValueError as e:
            raise SerialException(
                'expected a string in the form '
                '"rfc2217://<host>:<port>[?option[&option...]]": {}'.format(e))
        return (parts.hostname, parts.port)

    #  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

    @property
    def in_waiting(self):
        """Return the number of bytes currently in the input buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        return self._read_buffer.qsize()

    def read(self, size=1):
        """\
        Read size bytes from the serial port. If a timeout is set it may
        return less characters as requested. With no timeout it will block
        until the requested number of bytes is read.
        """
        if not self.is_open:
            raise PortNotOpenError()
        data = bytearray()
        try:
            timeout = Timeout(self._timeout)
            while len(data) < size:
                if self._thread is None or not self._thread.is_alive():
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
        """\
        Output the given byte string over the serial port. Can block if the
        connection is blocked. May raise SerialException if the connection is
        closed.
        """
        if not self.is_open:
            raise PortNotOpenError()
        with self._write_lock:
            try:
                self._socket.sendall(to_bytes(data).replace(IAC, IAC_DOUBLED))
            except socket.error as e:
                raise SerialException("connection failed (socket error): {}".format(e))
        return len(data)

    def reset_input_buffer(self):
        """Clear input buffer, discarding all that is in the buffer."""
        if not self.is_open:
            raise PortNotOpenError()
        self.rfc2217_send_purge(PURGE_RECEIVE_BUFFER)
        # empty read buffer
        while self._read_buffer.qsize():
            self._read_buffer.get(False)

    def reset_output_buffer(self):
        """\
        Clear output buffer, aborting the current output and
        discarding all that is in the buffer.
        """
        if not self.is_open:
            raise PortNotOpenError()
        self.rfc2217_send_purge(PURGE_TRANSMIT_BUFFER)

    def _update_break_state(self):
        """\
        Set break: Controls TXD. When active, to transmitting is
        possible.
        """
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('set BREAK to {}'.format('active' if self._break_state else 'inactive'))
        if self._break_state:
            self.rfc2217_set_control(SET_CONTROL_BREAK_ON)
        else:
            self.rfc2217_set_control(SET_CONTROL_BREAK_OFF)

    def _update_rts_state(self):
        """Set terminal status line: Request To Send."""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('set RTS to {}'.format('active' if self._rts_state else 'inactive'))
        if self._rts_state:
            self.rfc2217_set_control(SET_CONTROL_RTS_ON)
        else:
            self.rfc2217_set_control(SET_CONTROL_RTS_OFF)

    def _update_dtr_state(self):
        """Set terminal status line: Data Terminal Ready."""
        if not self.is_open:
            raise PortNotOpenError()
        if self.logger:
            self.logger.info('set DTR to {}'.format('active' if self._dtr_state else 'inactive'))
        if self._dtr_state:
            self.rfc2217_set_control(SET_CONTROL_DTR_ON)
        else:
            self.rfc2217_set_control(SET_CONTROL_DTR_OFF)

    @property
    def cts(self):
        """Read terminal status line: Clear To Send."""
        if not self.is_open:
            raise PortNotOpenError()
        return bool(self.get_modem_state() & MODEMSTATE_MASK_CTS)

    @property
    def dsr(self):
        """Read terminal status line: Data Set Ready."""
        if not self.is_open:
            raise PortNotOpenError()
        return bool(self.get_modem_state() & MODEMSTATE_MASK_DSR)

    @property
    def ri(self):
        """Read terminal status line: Ring Indicator."""
        if not self.is_open:
            raise PortNotOpenError()
        return bool(self.get_modem_state() & MODEMSTATE_MASK_RI)

    @property
    def cd(self):
        """Read terminal status line: Carrier Detect."""
        if not self.is_open:
            raise PortNotOpenError()
        return bool(self.get_modem_state() & MODEMSTATE_MASK_CD)

    # - - - platform specific - - -
    # None so far

    # - - - RFC2217 specific - - -

    def _telnet_read_loop(self):
        """Read loop for the socket."""
        mode = M_NORMAL
        suboption = None
        try:
            while self.is_open:
                try:
                    data = self._socket.recv(1024)
                except socket.timeout:
                    # just need to get out of recv form time to time to check if
                    # still alive
                    continue
                except socket.error as e:
                    # connection fails -> terminate loop
                    if self.logger:
                        self.logger.debug("socket error in reader thread: {}".format(e))
                    self._read_buffer.put(None)
                    break
                if not data:
                    self._read_buffer.put(None)
                    break  # lost connection
                for byte in iterbytes(data):
                    if mode == M_NORMAL:
                        # interpret as command or as data
                        if byte == IAC:
                            mode = M_IAC_SEEN
                        else:
                            # store data in read buffer or sub option buffer
                            # depending on state
                            if suboption is not None:
                                suboption += byte
                            else:
                                self._read_buffer.put(byte)
                    elif mode == M_IAC_SEEN:
                        if byte == IAC:
                            # interpret as command doubled -> insert character
                            # itself
                            if suboption is not None:
                                suboption += IAC
                            else:
                                self._read_buffer.put(IAC)
                            mode = M_NORMAL
                        elif byte == SB:
                            # sub option start
                            suboption = bytearray()
                            mode = M_NORMAL
                        elif byte == SE:
                            # sub option end -> process it now
                            self._telnet_process_subnegotiation(bytes(suboption))
                            suboption = None
                            mode = M_NORMAL
                        elif byte in (DO, DONT, WILL, WONT):
                            # negotiation
                            telnet_command = byte
                            mode = M_NEGOTIATE
                        else:
                            # other telnet commands
                            self._telnet_process_command(byte)
                            mode = M_NORMAL
                    elif mode == M_NEGOTIATE:  # DO, DONT, WILL, WONT was received, option now following
                        self._telnet_negotiate_option(telnet_command, byte)
                        mode = M_NORMAL
        finally:
            if self.logger:
                self.logger.debug("read thread terminated")

    # - incoming telnet commands and options

    def _telnet_process_command(self, command):
        """Process commands other than DO, DONT, WILL, WONT."""
        # Currently none. RFC2217 only uses negotiation and subnegotiation.
        if self.logger:
            self.logger.warning("ignoring Telnet command: {!r}".format(command))

    def _telnet_negotiate_option(self, command, option):
        """Process incoming DO, DONT, WILL, WONT."""
        # check our registered telnet options and forward command to them
        # they know themselves if they have to answer or not
        known = False
        for item in self._telnet_options:
            # can have more than one match! as some options are duplicated for
            # 'us' and 'them'
            if item.option == option:
                item.process_incoming(command)
                known = True
        if not known:
            # handle unknown options
            # only answer to positive requests and deny them
            if command == WILL or command == DO:
                self.telnet_send_option((DONT if command == WILL else WONT), option)
                if self.logger:
                    self.logger.warning("rejected Telnet option: {!r}".format(option))

    def _telnet_process_subnegotiation(self, suboption):
        """Process subnegotiation, the data between IAC SB and IAC SE."""
        if suboption[0:1] == COM_PORT_OPTION:
            if suboption[1:2] == SERVER_NOTIFY_LINESTATE and len(suboption) >= 3:
                self._linestate = ord(suboption[2:3])  # ensure it is a number
                if self.logger:
                    self.logger.info("NOTIFY_LINESTATE: {}".format(self._linestate))
            elif suboption[1:2] == SERVER_NOTIFY_MODEMSTATE and len(suboption) >= 3:
                self._modemstate = ord(suboption[2:3])  # ensure it is a number
                if self.logger:
                    self.logger.info("NOTIFY_MODEMSTATE: {}".format(self._modemstate))
                # update time when we think that a poll would make sense
                self._modemstate_timeout.restart(0.3)
            elif suboption[1:2] == FLOWCONTROL_SUSPEND:
                self._remote_suspend_flow = True
            elif suboption[1:2] == FLOWCONTROL_RESUME:
                self._remote_suspend_flow = False
            else:
                for item in self._rfc2217_options.values():
                    if item.ack_option == suboption[1:2]:
                        #~ print "processing COM_PORT_OPTION: %r" % list(suboption[1:])
                        item.check_answer(bytes(suboption[2:]))
                        break
                else:
                    if self.logger:
                        self.logger.warning("ignoring COM_PORT_OPTION: {!r}".format(suboption))
        else:
            if self.logger:
                self.logger.warning("ignoring subnegotiation: {!r}".format(suboption))

    # - outgoing telnet commands and options

    def _internal_raw_write(self, data):
        """internal socket write with no data escaping. used to send telnet stuff."""
        with self._write_lock:
            self._socket.sendall(data)

    def telnet_send_option(self, action, option):
        """Send DO, DONT, WILL, WONT."""
        self._internal_raw_write(IAC + action + option)

    def rfc2217_send_subnegotiation(self, option, value=b''):
        """Subnegotiation of RFC2217 parameters."""
        value = value.replace(IAC, IAC_DOUBLED)
        self._internal_raw_write(IAC + SB + COM_PORT_OPTION + option + value + IAC + SE)

    def rfc2217_send_purge(self, value):
        """\
        Send purge request to the remote.
        (PURGE_RECEIVE_BUFFER / PURGE_TRANSMIT_BUFFER / PURGE_BOTH_BUFFERS)
        """
        item = self._rfc2217_options['purge']
        item.set(value)  # transmit desired purge type
        item.wait(self._network_timeout)  # wait for acknowledge from the server

    def rfc2217_set_control(self, value):
        """transmit change of control line to remote"""
        item = self._rfc2217_options['control']
        item.set(value)  # transmit desired control type
        if self._ignore_set_control_answer:
            # answers are ignored when option is set. compatibility mode for
            # servers that answer, but not the expected one... (or no answer
            # at all) i.e. sredird
            time.sleep(0.1)  # this helps getting the unit tests passed
        else:
            item.wait(self._network_timeout)  # wait for acknowledge from the server

    def rfc2217_flow_server_ready(self):
        """\
        check if server is ready to receive data. block for some time when
        not.
        """
        #~ if self._remote_suspend_flow:
        #~     wait---

    def get_modem_state(self):
        """\
        get last modem state (cached value. If value is "old", request a new
        one. This cache helps that we don't issue to many requests when e.g. all
        status lines, one after the other is queried by the user (CTS, DSR
        etc.)
        """
        # active modem state polling enabled? is the value fresh enough?
        if self._poll_modem_state and self._modemstate_timeout.expired():
            if self.logger:
                self.logger.debug('polling modem state')
            # when it is older, request an update
            self.rfc2217_send_subnegotiation(NOTIFY_MODEMSTATE)
            timeout = Timeout(self._network_timeout)
            while not timeout.expired():
                time.sleep(0.05)    # prevent 100% CPU load
                # when expiration time is updated, it means that there is a new
                # value
                if not self._modemstate_timeout.expired():
                    break
            else:
                if self.logger:
                    self.logger.warning('poll for modem state failed')
            # even when there is a timeout, do not generate an error just
            # return the last known value. this way we can support buggy
            # servers that do not respond to polls, but send automatic
            # updates.
        if self._modemstate is not None:
            if self.logger:
                self.logger.debug('using cached modem state')
            return self._modemstate
        else:
            # never received a notification from the server
            raise SerialException("remote sends no NOTIFY_MODEMSTATE")


#############################################################################
# The following is code that helps implementing an RFC 2217 server.

class PortManager(object):
    """\
    This class manages the state of Telnet and RFC 2217. It needs a serial
    instance and a connection to work with. Connection is expected to implement
    a (thread safe) write function, that writes the string to the network.
    """

    def __init__(self, serial_port, connection, logger=None):
        self.serial = serial_port
        self.connection = connection
        self.logger = logger
        self._client_is_rfc2217 = False

        # filter state machine
        self.mode = M_NORMAL
        self.suboption = None
        self.telnet_command = None

        # states for modem/line control events
        self.modemstate_mask = 255
        self.last_modemstate = None
        self.linstate_mask = 0

        # all supported telnet options
        self._telnet_options = [
            TelnetOption(self, 'ECHO', ECHO, WILL, WONT, DO, DONT, REQUESTED),
            TelnetOption(self, 'we-SGA', SGA, WILL, WONT, DO, DONT, REQUESTED),
            TelnetOption(self, 'they-SGA', SGA, DO, DONT, WILL, WONT, INACTIVE),
            TelnetOption(self, 'we-BINARY', BINARY, WILL, WONT, DO, DONT, INACTIVE),
            TelnetOption(self, 'they-BINARY', BINARY, DO, DONT, WILL, WONT, REQUESTED),
            TelnetOption(self, 'we-RFC2217', COM_PORT_OPTION, WILL, WONT, DO, DONT, REQUESTED, self._client_ok),
            TelnetOption(self, 'they-RFC2217', COM_PORT_OPTION, DO, DONT, WILL, WONT, INACTIVE, self._client_ok),
        ]

        # negotiate Telnet/RFC2217 -> send initial requests
        if self.logger:
            self.logger.debug("requesting initial Telnet/RFC 2217 options")
        for option in self._telnet_options:
            if option.state is REQUESTED:
                self.telnet_send_option(option.send_yes, option.option)
        # issue 1st modem state notification

    def _client_ok(self):
        """\
        callback of telnet option. It gets called when option is activated.
        This one here is used to detect when the client agrees on RFC 2217. A
        flag is set so that other functions like check_modem_lines know if the
        client is OK.
        """
        # The callback is used for we and they so if one party agrees, we're
        # already happy. it seems not all servers do the negotiation correctly
        # and i guess there are incorrect clients too.. so be happy if client
        # answers one or the other positively.
        self._client_is_rfc2217 = True
        if self.logger:
            self.logger.info("client accepts RFC 2217")
        # this is to ensure that the client gets a notification, even if there
        # was no change
        self.check_modem_lines(force_notification=True)

    # - outgoing telnet commands and options

    def telnet_send_option(self, action, option):
        """Send DO, DONT, WILL, WONT."""
        self.connection.write(IAC + action + option)

    def rfc2217_send_subnegotiation(self, option, value=b''):
        """Subnegotiation of RFC 2217 parameters."""
        value = value.replace(IAC, IAC_DOUBLED)
        self.connection.write(IAC + SB + COM_PORT_OPTION + option + value + IAC + SE)

    # - check modem lines, needs to be called periodically from user to
    # establish polling

    def check_modem_lines(self, force_notification=False):
        """\
        read control lines from serial port and compare the last value sent to remote.
        send updates on changes.
        """
        modemstate = (
            (self.serial.cts and MODEMSTATE_MASK_CTS) |
            (self.serial.dsr and MODEMSTATE_MASK_DSR) |
            (self.serial.ri and MODEMSTATE_MASK_RI) |
            (self.serial.cd and MODEMSTATE_MASK_CD))
        # check what has changed
        deltas = modemstate ^ (self.last_modemstate or 0)  # when last is None -> 0
        if deltas & MODEMSTATE_MASK_CTS:
            modemstate |= MODEMSTATE_MASK_CTS_CHANGE
        if deltas & MODEMSTATE_MASK_DSR:
            modemstate |= MODEMSTATE_MASK_DSR_CHANGE
        if deltas & MODEMSTATE_MASK_RI:
            modemstate |= MODEMSTATE_MASK_RI_CHANGE
        if deltas & MODEMSTATE_MASK_CD:
            modemstate |= MODEMSTATE_MASK_CD_CHANGE
        # if new state is different and the mask allows this change, send
        # notification. suppress notifications when client is not rfc2217
        if modemstate != self.last_modemstate or force_notification:
            if (self._client_is_rfc2217 and (modemstate & self.modemstate_mask)) or force_notification:
                self.rfc2217_send_subnegotiation(
                    SERVER_NOTIFY_MODEMSTATE,
                    to_bytes([modemstate & self.modemstate_mask]))
                if self.logger:
                    self.logger.info("NOTIFY_MODEMSTATE: {}".format(modemstate))
            # save last state, but forget about deltas.
            # otherwise it would also notify about changing deltas which is
            # probably not very useful
            self.last_modemstate = modemstate & 0xf0

    # - outgoing data escaping

    def escape(self, data):
        """\
        This generator function is for the user. All outgoing data has to be
        properly escaped, so that no IAC character in the data stream messes up
        the Telnet state machine in the server.

        socket.sendall(escape(data))
        """
        for byte in iterbytes(data):
            if byte == IAC:
                yield IAC
                yield IAC
            else:
                yield byte

    # - incoming data filter

    def filter(self, data):
        """\
        Handle a bunch of incoming bytes. This is a generator. It will yield
        all characters not of interest for Telnet/RFC 2217.

        The idea is that the reader thread pushes data from the socket through
        this filter:

        for byte in filter(socket.recv(1024)):
            # do things like CR/LF conversion/whatever
            # and write data to the serial port
            serial.write(byte)

        (socket error handling code left as exercise for the reader)
        """
        for byte in iterbytes(data):
            if self.mode == M_NORMAL:
                # interpret as command or as data
                if byte == IAC:
                    self.mode = M_IAC_SEEN
                else:
                    # store data in sub option buffer or pass it to our
                    # consumer depending on state
                    if self.suboption is not None:
                        self.suboption += byte
                    else:
                        yield byte
            elif self.mode == M_IAC_SEEN:
                if byte == IAC:
                    # interpret as command doubled -> insert character
                    # itself
                    if self.suboption is not None:
                        self.suboption += byte
                    else:
                        yield byte
                    self.mode = M_NORMAL
                elif byte == SB:
                    # sub option start
                    self.suboption = bytearray()
                    self.mode = M_NORMAL
                elif byte == SE:
                    # sub option end -> process it now
                    self._telnet_process_subnegotiation(bytes(self.suboption))
                    self.suboption = None
                    self.mode = M_NORMAL
                elif byte in (DO, DONT, WILL, WONT):
                    # negotiation
                    self.telnet_command = byte
                    self.mode = M_NEGOTIATE
                else:
                    # other telnet commands
                    self._telnet_process_command(byte)
                    self.mode = M_NORMAL
            elif self.mode == M_NEGOTIATE:  # DO, DONT, WILL, WONT was received, option now following
                self._telnet_negotiate_option(self.telnet_command, byte)
                self.mode = M_NORMAL

    # - incoming telnet commands and options

    def _telnet_process_command(self, command):
        """Process commands other than DO, DONT, WILL, WONT."""
        # Currently none. RFC2217 only uses negotiation and subnegotiation.
        if self.logger:
            self.logger.warning("ignoring Telnet command: {!r}".format(command))

    def _telnet_negotiate_option(self, command, option):
        """Process incoming DO, DONT, WILL, WONT."""
        # check our registered telnet options and forward command to them
        # they know themselves if they have to answer or not
        known = False
        for item in self._telnet_options:
            # can have more than one match! as some options are duplicated for
            # 'us' and 'them'
            if item.option == option:
                item.process_incoming(command)
                known = True
        if not known:
            # handle unknown options
            # only answer to positive requests and deny them
            if command == WILL or command == DO:
                self.telnet_send_option((DONT if command == WILL else WONT), option)
                if self.logger:
                    self.logger.warning("rejected Telnet option: {!r}".format(option))

    def _telnet_process_subnegotiation(self, suboption):
        """Process subnegotiation, the data between IAC SB and IAC SE."""
        if suboption[0:1] == COM_PORT_OPTION:
            if self.logger:
                self.logger.debug('received COM_PORT_OPTION: {!r}'.format(suboption))
            if suboption[1:2] == SET_BAUDRATE:
                backup = self.serial.baudrate
                try:
                    (baudrate,) = struct.unpack(b"!I", suboption[2:6])
                    if baudrate != 0:
                        self.serial.baudrate = baudrate
                except ValueError as e:
                    if self.logger:
                        self.logger.error("failed to set baud rate: {}".format(e))
                    self.serial.baudrate = backup
                else:
                    if self.logger:
                        self.logger.info("{} baud rate: {}".format('set' if baudrate else 'get', self.serial.baudrate))
                self.rfc2217_send_subnegotiation(SERVER_SET_BAUDRATE, struct.pack(b"!I", self.serial.baudrate))
            elif suboption[1:2] == SET_DATASIZE:
                backup = self.serial.bytesize
                try:
                    (datasize,) = struct.unpack(b"!B", suboption[2:3])
                    if datasize != 0:
                        self.serial.bytesize = datasize
                except ValueError as e:
                    if self.logger:
                        self.logger.error("failed to set data size: {}".format(e))
                    self.serial.bytesize = backup
                else:
                    if self.logger:
                        self.logger.info("{} data size: {}".format('set' if datasize else 'get', self.serial.bytesize))
                self.rfc2217_send_subnegotiation(SERVER_SET_DATASIZE, struct.pack(b"!B", self.serial.bytesize))
            elif suboption[1:2] == SET_PARITY:
                backup = self.serial.parity
                try:
                    parity = struct.unpack(b"!B", suboption[2:3])[0]
                    if parity != 0:
                        self.serial.parity = RFC2217_REVERSE_PARITY_MAP[parity]
                except ValueError as e:
                    if self.logger:
                        self.logger.error("failed to set parity: {}".format(e))
                    self.serial.parity = backup
                else:
                    if self.logger:
                        self.logger.info("{} parity: {}".format('set' if parity else 'get', self.serial.parity))
                self.rfc2217_send_subnegotiation(
                    SERVER_SET_PARITY,
                    struct.pack(b"!B", RFC2217_PARITY_MAP[self.serial.parity]))
            elif suboption[1:2] == SET_STOPSIZE:
                backup = self.serial.stopbits
                try:
                    stopbits = struct.unpack(b"!B", suboption[2:3])[0]
                    if stopbits != 0:
                        self.serial.stopbits = RFC2217_REVERSE_STOPBIT_MAP[stopbits]
                except ValueError as e:
                    if self.logger:
                        self.logger.error("failed to set stop bits: {}".format(e))
                    self.serial.stopbits = backup
                else:
                    if self.logger:
                        self.logger.info("{} stop bits: {}".format('set' if stopbits else 'get', self.serial.stopbits))
                self.rfc2217_send_subnegotiation(
                    SERVER_SET_STOPSIZE,
                    struct.pack(b"!B", RFC2217_STOPBIT_MAP[self.serial.stopbits]))
            elif suboption[1:2] == SET_CONTROL:
                if suboption[2:3] == SET_CONTROL_REQ_FLOW_SETTING:
                    if self.serial.xonxoff:
                        self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_USE_SW_FLOW_CONTROL)
                    elif self.serial.rtscts:
                        self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_USE_HW_FLOW_CONTROL)
                    else:
                        self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_USE_NO_FLOW_CONTROL)
                elif suboption[2:3] == SET_CONTROL_USE_NO_FLOW_CONTROL:
                    self.serial.xonxoff = False
                    self.serial.rtscts = False
                    if self.logger:
                        self.logger.info("changed flow control to None")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_USE_NO_FLOW_CONTROL)
                elif suboption[2:3] == SET_CONTROL_USE_SW_FLOW_CONTROL:
                    self.serial.xonxoff = True
                    if self.logger:
                        self.logger.info("changed flow control to XON/XOFF")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_USE_SW_FLOW_CONTROL)
                elif suboption[2:3] == SET_CONTROL_USE_HW_FLOW_CONTROL:
                    self.serial.rtscts = True
                    if self.logger:
                        self.logger.info("changed flow control to RTS/CTS")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_USE_HW_FLOW_CONTROL)
                elif suboption[2:3] == SET_CONTROL_REQ_BREAK_STATE:
                    if self.logger:
                        self.logger.warning("requested break state - not implemented")
                    pass  # XXX needs cached value
                elif suboption[2:3] == SET_CONTROL_BREAK_ON:
                    self.serial.break_condition = True
                    if self.logger:
                        self.logger.info("changed BREAK to active")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_BREAK_ON)
                elif suboption[2:3] == SET_CONTROL_BREAK_OFF:
                    self.serial.break_condition = False
                    if self.logger:
                        self.logger.info("changed BREAK to inactive")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_BREAK_OFF)
                elif suboption[2:3] == SET_CONTROL_REQ_DTR:
                    if self.logger:
                        self.logger.warning("requested DTR state - not implemented")
                    pass  # XXX needs cached value
                elif suboption[2:3] == SET_CONTROL_DTR_ON:
                    self.serial.dtr = True
                    if self.logger:
                        self.logger.info("changed DTR to active")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_DTR_ON)
                elif suboption[2:3] == SET_CONTROL_DTR_OFF:
                    self.serial.dtr = False
                    if self.logger:
                        self.logger.info("changed DTR to inactive")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_DTR_OFF)
                elif suboption[2:3] == SET_CONTROL_REQ_RTS:
                    if self.logger:
                        self.logger.warning("requested RTS state - not implemented")
                    pass  # XXX needs cached value
                    #~ self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_RTS_ON)
                elif suboption[2:3] == SET_CONTROL_RTS_ON:
                    self.serial.rts = True
                    if self.logger:
                        self.logger.info("changed RTS to active")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_RTS_ON)
                elif suboption[2:3] == SET_CONTROL_RTS_OFF:
                    self.serial.rts = False
                    if self.logger:
                        self.logger.info("changed RTS to inactive")
                    self.rfc2217_send_subnegotiation(SERVER_SET_CONTROL, SET_CONTROL_RTS_OFF)
                #~ elif suboption[2:3] == SET_CONTROL_REQ_FLOW_SETTING_IN:
                #~ elif suboption[2:3] == SET_CONTROL_USE_NO_FLOW_CONTROL_IN:
                #~ elif suboption[2:3] == SET_CONTROL_USE_SW_FLOW_CONTOL_IN:
                #~ elif suboption[2:3] == SET_CONTROL_USE_HW_FLOW_CONTOL_IN:
                #~ elif suboption[2:3] == SET_CONTROL_USE_DCD_FLOW_CONTROL:
                #~ elif suboption[2:3] == SET_CONTROL_USE_DTR_FLOW_CONTROL:
                #~ elif suboption[2:3] == SET_CONTROL_USE_DSR_FLOW_CONTROL:
            elif suboption[1:2] == NOTIFY_LINESTATE:
                # client polls for current state
                self.rfc2217_send_subnegotiation(
                    SERVER_NOTIFY_LINESTATE,
                    to_bytes([0]))   # sorry, nothing like that implemented
            elif suboption[1:2] == NOTIFY_MODEMSTATE:
                if self.logger:
                    self.logger.info("request for modem state")
                # client polls for current state
                self.check_modem_lines(force_notification=True)
            elif suboption[1:2] == FLOWCONTROL_SUSPEND:
                if self.logger:
                    self.logger.info("suspend")
                self._remote_suspend_flow = True
            elif suboption[1:2] == FLOWCONTROL_RESUME:
                if self.logger:
                    self.logger.info("resume")
                self._remote_suspend_flow = False
            elif suboption[1:2] == SET_LINESTATE_MASK:
                self.linstate_mask = ord(suboption[2:3])  # ensure it is a number
                if self.logger:
                    self.logger.info("line state mask: 0x{:02x}".format(self.linstate_mask))
            elif suboption[1:2] == SET_MODEMSTATE_MASK:
                self.modemstate_mask = ord(suboption[2:3])  # ensure it is a number
                if self.logger:
                    self.logger.info("modem state mask: 0x{:02x}".format(self.modemstate_mask))
            elif suboption[1:2] == PURGE_DATA:
                if suboption[2:3] == PURGE_RECEIVE_BUFFER:
                    self.serial.reset_input_buffer()
                    if self.logger:
                        self.logger.info("purge in")
                    self.rfc2217_send_subnegotiation(SERVER_PURGE_DATA, PURGE_RECEIVE_BUFFER)
                elif suboption[2:3] == PURGE_TRANSMIT_BUFFER:
                    self.serial.reset_output_buffer()
                    if self.logger:
                        self.logger.info("purge out")
                    self.rfc2217_send_subnegotiation(SERVER_PURGE_DATA, PURGE_TRANSMIT_BUFFER)
                elif suboption[2:3] == PURGE_BOTH_BUFFERS:
                    self.serial.reset_input_buffer()
                    self.serial.reset_output_buffer()
                    if self.logger:
                        self.logger.info("purge both")
                    self.rfc2217_send_subnegotiation(SERVER_PURGE_DATA, PURGE_BOTH_BUFFERS)
                else:
                    if self.logger:
                        self.logger.error("undefined PURGE_DATA: {!r}".format(list(suboption[2:])))
            else:
                if self.logger:
                    self.logger.error("undefined COM_PORT_OPTION: {!r}".format(list(suboption[1:])))
        else:
            if self.logger:
                self.logger.warning("unknown subnegotiation: {!r}".format(suboption))


# simple client test
if __name__ == '__main__':
    import sys
    s = Serial('rfc2217://localhost:7000', 115200)
    sys.stdout.write('{}\n'.format(s))

    sys.stdout.write("write...\n")
    s.write(b"hello\n")
    s.flush()
    sys.stdout.write("read: {}\n".format(s.read(5)))
    s.close()
