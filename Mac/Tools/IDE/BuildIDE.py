"""Build a "big" applet for the IDE, and put it in the Python home
directory. It will contain all IDE-specific modules as PYC resources,
which reduces the startup time (especially on slower machines)."""

import sys
import os
import buildtools
from Carbon import Res
import py_resource

buildtools.DEBUG=1

template = buildtools.findtemplate()

ide_home = os.path.join(sys.exec_prefix, ":Mac:Tools:IDE")

mainfilename = os.path.join(ide_home, "PythonIDE.py")
dstfilename = os.path.join(sys.exec_prefix, "Python IDE")

buildtools.process(template, mainfilename, dstfilename, 1)

targetref = Res.FSpOpenResFile(dstfilename, 3)
Res.UseResFile(targetref)

files = os.listdir(ide_home)

# skip this script and the main program
files = filter(lambda x: x[-3:] == '.py' and
                x not in ("BuildIDE.py", "PythonIDE.py"), files)

# add the modules as PYC resources
for name in files:
    print "adding", name
    fullpath = os.path.join(ide_home, name)
    id, name = py_resource.frompyfile(fullpath, name[:-3], preload=1,
            ispackage=0)

# add W resources
wresref = Res.FSpOpenResFile(os.path.join(ide_home, "Widgets.rsrc"), 1)
buildtools.copyres(wresref, targetref, [], 0)
