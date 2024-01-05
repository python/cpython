"""
Bootstrap script for IDLE as an application bundle.
"""
import sys, os

# Change the current directory the user's home directory, that way we'll get
# a more useful default location in the open/save dialogs.
os.chdir(os.path.expanduser('~/Documents'))


# Make sure sys.executable points to the python interpreter inside the
# framework, instead of at the helper executable inside the application
# bundle (the latter works, but doesn't allow access to the window server)
#
#  .../IDLE.app/
#       Contents/
#           MacOS/
#               IDLE (a python script)
#               Python{-32} (symlink)
#           Resources/
#               idlemain.py (this module)
#               ...
#
# ../IDLE.app/Contents/MacOS/Python{-32} is symlinked to
#       ..Library/Frameworks/Python.framework/Versions/m.n
#                   /Resources/Python.app/Contents/MacOS/Python{-32}
#       which is the Python interpreter executable
#
# The flow of control is as follows:
# 1. IDLE.app is launched which starts python running the IDLE script
# 2. IDLE script exports
#       PYTHONEXECUTABLE = .../IDLE.app/Contents/MacOS/Python{-32}
#           (the symlink to the framework python)
# 3. IDLE script alters sys.argv and uses os.execve to replace itself with
#       idlemain.py running under the symlinked python.
#       This is the magic step.
# 4. During interpreter initialization, because PYTHONEXECUTABLE is defined,
#    sys.executable may get set to an useless value.
#
# (Note that the IDLE script and the setting of PYTHONEXECUTABLE is
#  generated automatically by bundlebuilder in the Python 2.x build.
#  Also, IDLE invoked via command line, i.e. bin/idle, bypasses all of
#  this.)
#
# Now fix up the execution environment before importing idlelib.

# Reset sys.executable to its normal value, the actual path of
# the interpreter in the framework, by following the symlink
# exported in PYTHONEXECUTABLE.
pyex = os.environ['PYTHONEXECUTABLE']
sys.executable = os.path.join(sys.prefix, 'bin', 'python%d.%d'%(sys.version_info[:2]))

# Remove any sys.path entries for the Resources dir in the IDLE.app bundle.
p = pyex.partition('.app')
if p[2].startswith('/Contents/MacOS/Python'):
    sys.path = [value for value in sys.path if
            value.partition('.app') != (p[0], p[1], '/Contents/Resources')]

# Unexport PYTHONEXECUTABLE so that the other Python processes started
# by IDLE have a normal sys.executable.
del os.environ['PYTHONEXECUTABLE']

# Look for the -psn argument that the launcher adds and remove it, it will
# only confuse the IDLE startup code.
for idx, value in enumerate(sys.argv):
    if value.startswith('-psn_'):
        del sys.argv[idx]
        break

# Now it is safe to import idlelib.
from idlelib.pyshell import main
if __name__ == '__main__':
    main()
