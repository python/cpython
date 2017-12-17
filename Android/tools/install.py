"""Install Python."""

import sys
import os
import subprocess
import tempfile
from os.path import join, commonprefix

from android_utils import (run_subprocess, adb_push_to_dir, AndroidError,
                           adb_shell)

def adb_push_from_zip(zippath, sys_prefix):
    with tempfile.TemporaryDirectory() as tmpdir:
        run_subprocess('unzip', '-q', zippath, '-d', tmpdir)

        # On the first call to adb_push_to_dir() where 'dest' is a directory
        # on the sdcard, the mkdir command is attempted until /sdcard is
        # mounted read/write on emulator start up.
        dest = sys_prefix
        sdcard = '/sdcard'
        wait = True if commonprefix([sdcard, dest]) == sdcard else False
        adb_shell('mkdir -p %s' % dest, wait_for_sdcard=wait)

        src = join(tmpdir, sys_prefix[1:] if
                           os.path.isabs(sys_prefix) else sys_prefix)
        adb_push_to_dir(src, os.path.split(dest)[0])

def install():
    stdlib_path = os.environ['STDLIB_DIR']

    adb_push_from_zip(os.environ['PY_STDLIB_ZIP'], os.environ['SYS_PREFIX'])
    adb_push_from_zip(os.environ['PYTHON_ZIP'], os.environ['SYS_EXEC_PREFIX'])
    srcdir = os.environ.get('PY_SRCDIR')
    if srcdir:
        # Push the script run by buildbottest.
        bin_dir = join(os.environ['SYS_EXEC_PREFIX'], 'bin')
        adb_push_to_dir(join(srcdir, 'Tools/scripts/run_tests.py'), bin_dir)

if __name__ == "__main__":
    try:
        install()
    except subprocess.CalledProcessError as e:
        print('CalledProcessError: Command %(cmd)s: stdout=<%(output)s> '
               'stderr=<%(stderr)s>' % e.__dict__, file=sys.stderr)
        sys.exit(1)
    except AndroidError as e:
        sys.exit(e)
