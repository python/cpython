#
# After running python setup.py install, run this program from the command
# line like so:
#
# % python2.3 buildapp.py build
#
# A double-clickable IDLE application will be created in the build/ directory.
#

from bundlebuilder import buildapp

buildapp(
        name="IDLE",
        mainprogram="idle.py",
        argv_emulation=1,
        iconfile="Icons/idle.icns",
)
