"""Install Python."""

import sys
import os
import subprocess
import tempfile
from os.path import join

from android_utils import (run_subprocess, adb_shell, adb_push_to_dir,
                           AndroidError)

def adb_push_from_zip(zippath, dirname, destination, remove=True):
    # Remove the previous installation.
    if remove:
        previous = join(destination, dirname)
        print('Removing %s.' % previous)
        adb_shell('rm -rf %s' % previous)

    with tempfile.TemporaryDirectory() as tmpdir:
        run_subprocess('unzip', '-q', zippath, '-d', tmpdir)
        src= join(tmpdir, dirname)
        adb_push_to_dir(src, destination)

def install():
    stdlib_path = os.environ['STDLIB_DIR']

    # The 'sdclean' prerequisite of the 'install' and 'python' targets of
    # emulator.mk ensures that the sdcard has been newly created and we set
    # the 'remove' keyword parameter to False when the destination is
    # SYS_PREFIX because it is not needed and because the 'rm -rf' command in
    # adb_push_to_dir() would have failed to remove the previous installation
    # when the sdcard is not yet mounted as read-write.
    #
    # Start with installing on SYS_PREFIX on the sdcard so that the first
    # adb_push_to_dir() call waits for the sdcard to be mounted read-write.
    adb_push_from_zip(os.environ['PY_STDLIB_ZIP'],
                   stdlib_path.split('/')[0],
                   os.environ['SYS_PREFIX'], remove=False)

    adb_push_from_zip(os.environ['PYTHON_ZIP'],
                   os.environ['ZIPBASE_DIR'],
                   os.environ['ANDROID_APP_DIR'])

if __name__ == "__main__":
    try:
        install()
    except subprocess.CalledProcessError as e:
        print('CalledProcessError: Command %(cmd)s: stdout=<%(output)s> '
               'stderr=<%(stderr)s>' % e.__dict__, file=sys.stderr)
        sys.exit(1)
    except AndroidError as e:
        sys.exit(e)
