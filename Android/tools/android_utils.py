"""Android utils."""

import sys
import os
import time
import random
import subprocess
import socket
import errno
from subprocess import PIPE, STDOUT

class AndroidError(BaseException): pass

def emulator_listens(port):
    """Check that an emulator is listening on 'port'."""
    try:
        sock = socket.create_connection(('localhost', port))
        sock.close()
        print('An emulator is currently listening on port %d.' % port,
              file=sys.stderr)
        return 1
    except OSError as e:
        if e.errno == errno.ECONNREFUSED:
            return 0
        raise

def run_subprocess(*args, verbose=True):
    print(' '.join(args))
    proc = subprocess.run(args, check=True, stdout=PIPE, stderr=PIPE)
    if proc.stdout and verbose:
        print(proc.stdout.decode())
    if proc.stderr and verbose:
        print(proc.stderr.decode(), file=sys.stderr)

_android_api = None
def get_android_api():
    global _android_api
    if _android_api is None:
        proc = subprocess.run([os.environ['ADB'], 'shell',
                               'getprop ro.build.version.sdk'],
                              check=True, stdout=PIPE, stderr=PIPE)
        _android_api = int(proc.stdout)
    return _android_api

MOUNT_SDCARD_TIME = 600
KEY_MSG = 'random key found: '
_first_invocation = True

def adb_shell(cmd, wait_for_sdcard=False):
    global _first_invocation

    adb = os.environ['ADB']
    if not (wait_for_sdcard and _first_invocation):
        run_subprocess(adb, 'shell', cmd)
        return
    _first_invocation = False

    # Use a random key to find out if the shell command was successfull as the
    # adb shell does not report the exit status of the executed command.
    key = str(random.random())
    args = [adb, 'shell',
            '{ %s; } && echo "%s%s"' % (cmd.strip(';'), KEY_MSG, key)]
    key = key.encode(encoding='ascii')

    starttime = curtime = time.time()
    stdout = ''
    try:
        while 1:
            proc = subprocess.run(args, stdout=PIPE, stderr=STDOUT)
            stdout = proc.stdout
            rc = proc.returncode
            if stdout is None:
                stdout = ''
            if ((rc == 0 and key in stdout) or
                    (rc != 0 and get_android_api() < 24) or
                    (rc != 0 and b'no devices/emulators found' in stdout)):
                break
            if curtime >= starttime + MOUNT_SDCARD_TIME:
                break

            if curtime == starttime:
                print('%s shell %s' % (adb, cmd), end='', flush=True)
            else:
                print('.', end='', flush=True)
            time.sleep(.500)
            curtime = time.time()
    except KeyboardInterrupt:
        raise AndroidError('\nKeyboardInterrupt: adb_shell("%s") = %d <%s>' %
                           (cmd, rc, stdout))

    if curtime == starttime:
        print('%s shell %s' % (adb, cmd))

    if stdout:
        for line in stdout.decode().split('\n'):
            if not line.startswith(KEY_MSG):
                print(line.strip('\r'))
    if not key in stdout or rc != 0:
        raise AndroidError('Error: adb_shell("%s") = %d' % (cmd, rc))

def adb_push_to_dir(src, dest):
    """Push src to a directory on Android."""
    # On the first call to adb_push_to_dir() where 'dest' is a directory on
    # the sdcard, the mkdir command is attempted until the sdcard is mounted
    # read/write on emulator start up.
    sdcard = os.environ['SDCARD']
    wait_for_sdcard = (True if os.path.commonprefix([sdcard, dest]) == sdcard
                       else False)
    adb_shell('mkdir -p %s' % dest, wait_for_sdcard=wait_for_sdcard)
    print('Please wait: pushing %s to %s...' % (src, dest), flush=True)
    run_subprocess(os.environ['ADB'], 'push', src, dest, verbose=False)

def run_script(script_path):
    """Push a script to the emulator and run it."""
    bin_dir = os.path.join(os.environ['SYS_EXEC_PREFIX'], 'bin')
    adb_push_to_dir(script_path, bin_dir)
    script_path = os.path.join(bin_dir, os.path.basename(script_path))
    print()
    print('Running %s' % script_path, flush=True)
    subprocess.run([os.environ['ADB'], 'shell', 'sh %s' % script_path])
