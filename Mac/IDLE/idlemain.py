"""
Bootstrap script for IDLE as an application bundle.
"""
import sys, os

from idlelib.PyShell import main

# Change the current directory the user's home directory, that way we'll get
# a more useful default location in the open/save dialogs.
os.chdir(os.path.expanduser('~/Documents'))


# Make sure sys.executable points to the python interpreter inside the
# framework, instead of at the helper executable inside the application
# bundle (the latter works, but doesn't allow access to the window server)
sys.executable = os.path.join(sys.prefix, 'bin', 'python')

# Look for the -psn argument that the launcher adds and remove it, it will
# only confuse the IDLE startup code.
for idx, value in enumerate(sys.argv):
    if value.startswith('-psn_'):
        del sys.argv[idx]
        break

#argvemulator.ArgvCollector().mainloop()
if __name__ == '__main__':
    main()
