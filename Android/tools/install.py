"""Install Python."""

import sys
import os
import subprocess
import tempfile
from os.path import join

from android_utils import (run_subprocess, adb_shell, adb_push_to_dir,
                           AndroidError)

def adb_push_from_zip(zippath, dirname, destination):
    with tempfile.TemporaryDirectory() as tmpdir:
        run_subprocess('unzip', '-q', zippath, '-d', tmpdir)
        src= join(tmpdir, dirname)
        adb_push_to_dir(src, destination)

def install():
    stdlib_path = os.environ['STDLIB_DIR']

    adb_push_from_zip(os.environ['PY_STDLIB_ZIP'],
                   stdlib_path.split('/')[0],
                   os.environ['SYS_PREFIX'])

    adb_push_from_zip(os.environ['PYTHON_ZIP'],
                   os.environ['ZIPBASE_DIR'],
                   os.environ['ANDROID_APP_DIR'])

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
