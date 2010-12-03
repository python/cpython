import os
import sys
import sysconfig

from test import support
from test import regrtest

# findtestdir() gets the dirname out of __file__, so we have to make it
# absolute before changing the working directory.
# For example __file__ may be relative when running trace or profile.
# See issue #9323.
__file__ = os.path.abspath(__file__)

# sanity check
assert __file__ == os.path.abspath(sys.argv[0])

# When tests are run from the Python build directory, it is best practice
# to keep the test files in a subfolder.  It eases the cleanup of leftover
# files using command "make distclean".
if sysconfig.is_python_build():
    TEMPDIR = os.path.join(sysconfig.get_config_var('srcdir'), 'build')
    TEMPDIR = os.path.abspath(TEMPDIR)
    if not os.path.exists(TEMPDIR):
        os.mkdir(TEMPDIR)
    regrtest.TEMPDIR = TEMPDIR

# Define a writable temp dir that will be used as cwd while running
# the tests. The name of the dir includes the pid to allow parallel
# testing (see the -j option).
TESTCWD = 'test_python_{}'.format(os.getpid())

TESTCWD = os.path.join(TEMPDIR, TESTCWD)
regrtest.TESTCWD = TESTCWD

# Run the tests in a context manager that temporary changes the CWD to a
# temporary and writable directory. If it's not possible to create or
# change the CWD, the original CWD will be used. The original CWD is
# available from support.SAVEDCWD.
with support.temp_cwd(TESTCWD, quiet=True):
    regrtest.main()
