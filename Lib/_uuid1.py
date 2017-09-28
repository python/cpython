bytes_ = bytes  # The built-in bytes type


def _popen(command, *args):
    import os, shutil, subprocess
    executable = shutil.which(command)
    if executable is None:
        path = os.pathsep.join(('/sbin', '/usr/sbin'))
        executable = shutil.which(command, path=path)
        if executable is None:
            return None

    # LC_ALL=C to ensure English output, stderr=DEVNULL to prevent output
    # on stderr (Note: we don't have an example where the words we search
    # for are actually localized, but in theory some system could do so.)
    env = dict(os.environ)
    env['LC_ALL'] = 'C'
    proc = subprocess.Popen((executable,) + args,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL,
                            env=env)
    return proc


def _find_mac(command, args, hw_identifiers, get_index):
    try:
        proc = _popen(command, *args.split())
        if not proc:
            return

        with proc:
            for line in proc.stdout:
                words = line.lower().rstrip().split()
                for i in range(len(words)):
                    if words[i] in hw_identifiers:
                        try:
                            word = words[get_index(i)]
                            mac = int(word.replace(b':', b''), 16)
                            if mac:
                                return mac
                        except (ValueError, IndexError):
                            # Virtual interfaces, such as those provided by
                            # VPNs, do not have a colon-delimited MAC address
                            # as expected, but a 16-byte HWAddr separated by
                            # dashes. These should be ignored in favor of a
                            # real MAC address
                            pass
    except OSError:
        pass


def _ifconfig_getnode():
    """Get the hardware address on Unix by running ifconfig."""
    # This works on Linux ('' or '-a'), Tru64 ('-av'), but not all Unixes.
    for args in ('', '-a', '-av'):
        mac = _find_mac('ifconfig', args, [b'hwaddr', b'ether'], lambda i: i+1)
        if mac:
            return mac


def _ip_getnode():
    """Get the hardware address on Unix by running ip."""
    # This works on Linux with iproute2.
    mac = _find_mac('ip', 'link list', [b'link/ether'], lambda i: i+1)
    if mac:
        return mac


def _arp_getnode():
    """Get the hardware address on Unix by running arp."""
    import os, socket
    try:
        ip_addr = socket.gethostbyname(socket.gethostname())
    except OSError:
        return None

    # Try getting the MAC addr from arp based on our IP address (Solaris).
    return _find_mac('arp', '-an', [os.fsencode(ip_addr)], lambda i: -1)


def _lanscan_getnode():
    """Get the hardware address on Unix by running lanscan."""
    # This might work on HP-UX.
    return _find_mac('lanscan', '-ai', [b'lan0'], lambda i: 0)


def _netstat_getnode():
    """Get the hardware address on Unix by running netstat."""
    # This might work on AIX, Tru64 UNIX.
    try:
        proc = _popen('netstat', '-ia')
        if not proc:
            return

        with proc:
            words = proc.stdout.readline().rstrip().split()
            try:
                i = words.index(b'Address')
            except ValueError:
                return
            for line in proc.stdout:
                try:
                    words = line.rstrip().split()
                    word = words[i]
                    if len(word) == 17 and word.count(b':') == 5:
                        mac = int(word.replace(b':', b''), 16)
                        if mac:
                            return mac
                except (ValueError, IndexError):
                    pass
    except OSError:
        pass


def _ipconfig_getnode():
    """Get the hardware address on Windows by running ipconfig.exe."""
    import os, re
    dirs = ['', r'c:\windows\system32', r'c:\winnt\system32']
    try:
        import ctypes
        buffer = ctypes.create_string_buffer(300)
        ctypes.windll.kernel32.GetSystemDirectoryA(buffer, 300)
        dirs.insert(0, buffer.value.decode('mbcs'))
    except:
        pass
    for dir in dirs:
        try:
            pipe = os.popen(os.path.join(dir, 'ipconfig') + ' /all')
        except OSError:
            continue
        with pipe:
            for line in pipe:
                value = line.split(':')[-1].strip().lower()
                if re.match('([0-9a-f][0-9a-f]-){5}[0-9a-f][0-9a-f]', value):
                    return int(value.replace('-', ''), 16)


def _netbios_getnode():
    """Get the hardware address on Windows using NetBIOS calls.
    See http://support.microsoft.com/kb/118623 for details."""
    import win32wnet, netbios
    ncb = netbios.NCB()
    ncb.Command = netbios.NCBENUM
    ncb.Buffer = adapters = netbios.LANA_ENUM()
    adapters._pack()
    if win32wnet.Netbios(ncb) != 0:
        return

    adapters._unpack()
    for i in range(adapters.length):
        ncb.Reset()
        ncb.Command = netbios.NCBRESET
        ncb.Lana_num = ord(adapters.lana[i])
        if win32wnet.Netbios(ncb) != 0:
            continue
        ncb.Reset()
        ncb.Command = netbios.NCBASTAT
        ncb.Lana_num = ord(adapters.lana[i])
        ncb.Callname = '*'.ljust(16)
        ncb.Buffer = status = netbios.ADAPTER_STATUS()
        if win32wnet.Netbios(ncb) != 0:
            continue
        status._unpack()
        bytes = status.adapter_address[:6]
        if len(bytes) != 6:
            continue
        return int.from_bytes(bytes, 'big')


# Thanks to Thomas Heller for ctypes and for his help with its use here.

# If ctypes is available, use it to find system routines for UUID generation.
# XXX This makes the module non-thread-safe!
_uuid_generate_time = _UuidCreate = None
try:
    import ctypes, ctypes.util
    import sys

    # The uuid_generate_* routines are provided by libuuid on at least
    # Linux and FreeBSD, and provided by libc on Mac OS X.
    _libnames = ['uuid']
    if not sys.platform.startswith('win'):
        _libnames.append('c')

    for libname in _libnames:
        try:
            lib = ctypes.CDLL(ctypes.util.find_library(libname))
        except Exception:                           # pragma: nocover
            continue
        # Try to find the safe variety first.
        if hasattr(lib, 'uuid_generate_time_safe'):
            _uuid_generate_time = lib.uuid_generate_time_safe
            # int uuid_generate_time_safe(uuid_t out);
            break
        elif hasattr(lib, 'uuid_generate_time'):    # pragma: nocover
            _uuid_generate_time = lib.uuid_generate_time
            # void uuid_generate_time(uuid_t out);
            _uuid_generate_time.restype = None
            break
    del _libnames

    # The uuid_generate_* functions are broken on MacOS X 10.5, as noted
    # in issue #8621 the function generates the same sequence of values
    # in the parent process and all children created using fork (unless
    # those children use exec as well).
    #
    # Assume that the uuid_generate functions are broken from 10.5 onward,
    # the test can be adjusted when a later version is fixed.
    if sys.platform == 'darwin':
        if int(os.uname().release.split('.')[0]) >= 9:
            _uuid_generate_time = None

    # On Windows prior to 2000, UuidCreate gives a UUID containing the
    # hardware address.  On Windows 2000 and later, UuidCreate makes a
    # random UUID and UuidCreateSequential gives a UUID containing the
    # hardware address.  These routines are provided by the RPC runtime.
    # NOTE:  at least on Tim's WinXP Pro SP2 desktop box, while the last
    # 6 bytes returned by UuidCreateSequential are fixed, they don't appear
    # to bear any relationship to the MAC address of any network device
    # on the box.
    try:
        lib = ctypes.windll.rpcrt4
    except:
        lib = None
    _UuidCreate = getattr(lib, 'UuidCreateSequential',
                          getattr(lib, 'UuidCreate', None))
except:
    pass


def _bytes_to_node(rawbytes):
    value = int.from_bytes(rawbytes, byteorder='big')
    node = value & 0xffffffffffff
    return node


def _unixdll_getnode():
    """Get the hardware address on Unix using ctypes."""
    _buffer = ctypes.create_string_buffer(16)
    _uuid_generate_time(_buffer)
    return _bytes_to_node(bytes_(_buffer.raw))


def _windll_getnode():
    """Get the hardware address on Windows using ctypes."""
    _buffer = ctypes.create_string_buffer(16)
    if _UuidCreate(_buffer) == 0:
        return _bytes_to_node(bytes_(_buffer.raw))


def _random_getnode():
    """Get a random node ID, with eighth bit set as suggested by RFC 4122."""
    import random
    return random.getrandbits(48) | 0x010000000000


_node = None


def getnode():
    """Get the hardware address as a 48-bit positive integer.

    The first time this runs, it may launch a separate program, which could
    be quite slow.  If all attempts to obtain the hardware address fail, we
    choose a random 48-bit number with its eighth bit set to 1 as recommended
    in RFC 4122.
    """

    global _node
    if _node is not None:
        return _node

    import sys
    if sys.platform == 'win32':
        getters = [_windll_getnode, _netbios_getnode, _ipconfig_getnode]
    else:
        getters = [_unixdll_getnode, _ifconfig_getnode, _ip_getnode,
                   _arp_getnode, _lanscan_getnode, _netstat_getnode]

    for getter in getters + [_random_getnode]:
        try:
            _node = getter()
        except:
            continue
        if _node is not None:
            return _node


_last_timestamp = None


def uuid1(UUID, SafeUUID, node, clock_seq):
    # When the system provides a version-1 UUID generator, use it (but don't
    # use UuidCreate here because its UUIDs don't conform to RFC 4122).
    if _uuid_generate_time and node is clock_seq is None:
        _buffer = ctypes.create_string_buffer(16)
        safely_generated = _uuid_generate_time(_buffer)
        try:
            is_safe = SafeUUID(safely_generated)
        except ValueError:
            is_safe = SafeUUID.unknown
        return UUID(bytes=bytes_(_buffer.raw), is_safe=is_safe)

    global _last_timestamp
    import time
    nanoseconds = int(time.time() * 1e9)
    # 0x01b21dd213814000 is the number of 100-ns intervals between the
    # UUID epoch 1582-10-15 00:00:00 and the Unix epoch 1970-01-01 00:00:00.
    timestamp = int(nanoseconds/100) + 0x01b21dd213814000
    if _last_timestamp is not None and timestamp <= _last_timestamp:
        timestamp = _last_timestamp + 1
    _last_timestamp = timestamp

    if clock_seq is None:
        import random
        clock_seq = random.getrandbits(14) # instead of stable storage

    time_low = timestamp & 0xffffffff
    time_mid = (timestamp >> 32) & 0xffff
    time_hi_version = (timestamp >> 48) & 0x0fff
    clock_seq_low = clock_seq & 0xff
    clock_seq_hi_variant = (clock_seq >> 8) & 0x3f
    if node is None:
        node = getnode()

    return UUID(fields=(time_low, time_mid, time_hi_version,
                        clock_seq_hi_variant, clock_seq_low, node),
                version=1)
