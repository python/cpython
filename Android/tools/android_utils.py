"""Android utils."""

import sys
import os
import time
import random
import subprocess
import socket
import errno
import argparse
from subprocess import PIPE, STDOUT

class AndroidError(BaseException): pass

_adb_cmd = None
def get_adb_cmd():
    global _adb_cmd
    if _adb_cmd is None:
        _adb_cmd = [os.environ['ADB'],
                    '-s', 'emulator-%s' % os.environ['CONSOLE_PORT']]
    return _adb_cmd

def is_emulator_listening(port):
    """Check if an emulator is listening on 'port'."""
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
        proc = subprocess.run(get_adb_cmd() + ['shell',
                               'getprop ro.build.version.sdk'],
                              check=True, stdout=PIPE, stderr=PIPE)
        _android_api = int(proc.stdout)
    return _android_api

MOUNT_SDCARD_TIME = 600
KEY_MSG = 'random key found: '
_first_invocation = True

def adb_shell(cmd, wait_for_sdcard=False):
    global _first_invocation

    if not (wait_for_sdcard and _first_invocation):
        run_subprocess(*get_adb_cmd(), 'shell', cmd)
        return
    _first_invocation = False

    # Use a random key to find out if the shell command was successfull as the
    # adb shell does not report the exit status of the executed command.
    key = str(random.random())
    args = get_adb_cmd() + ['shell',
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
                print('%s shell %s' % (' '.join(get_adb_cmd()), cmd), end='',
                      flush=True)
            else:
                print('.', end='', flush=True)
            time.sleep(.500)
            curtime = time.time()
    except KeyboardInterrupt:
        raise AndroidError('\nKeyboardInterrupt: adb_shell("%s") = %d <%s>' %
                           (cmd, rc, stdout))

    if curtime == starttime:
        print('%s shell %s' % (' '.join(get_adb_cmd()), cmd))

    if stdout:
        for line in stdout.decode().split('\n'):
            if not line.startswith(KEY_MSG):
                print(line.strip('\r'))
    if not key in stdout or rc != 0:
        raise AndroidError('Error: adb_shell("%s") = %d' % (cmd, rc))

def adb_push_to_dir(src, dest):
    """Push src to a directory on Android."""
    print('Please wait: pushing %s to %s...' % (src, dest), flush=True)
    run_subprocess(*get_adb_cmd(), 'push', src, dest, verbose=False)

def adb_pull(remote, dest):
    run_subprocess(*get_adb_cmd(), 'pull', remote, dest, verbose=False)

def run_script(script_path):
    """Push a script to the emulator and run it."""
    try:
        bin_dir = os.path.join(os.environ['SYS_EXEC_PREFIX'], 'bin')
        adb_push_to_dir(script_path, bin_dir)
        path = os.path.join(bin_dir, os.path.basename(script_path))
        print()
        print('Running %s' % path, flush=True)
        subprocess.run(get_adb_cmd() + ['shell', 'sh %s' % path])
    finally:
        try:
            os.unlink(script_path)
        except OSError:
            pass

def parse_prefixes(cmd_line):
    """ Parse the prefixes of a configure command line."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--prefix')
    parser.add_argument('--exec-prefix')
    args, unknown = parser.parse_known_args(cmd_line.split())
    if args.prefix is None:
        args.prefix = '/usr/local'
    if args.exec_prefix is None:
        args.exec_prefix = args.prefix
    print(args.prefix, args.exec_prefix)
