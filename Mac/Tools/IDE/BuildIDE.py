import sys
import os
import buildtools
import Res
import py_resource

buildtools.DEBUG=1

template = buildtools.findtemplate()

ide_home = os.path.join(sys.exec_prefix, ":Mac:Tools:IDE")

mainfilename = os.path.join(ide_home, "PythonIDE.py")
dstfilename = os.path.join(sys.exec_prefix, "Python IDE")

buildtools.process(template, mainfilename, dstfilename, 1)

targetref = Res.OpenResFile(dstfilename)
Res.UseResFile(targetref)

files = os.listdir(ide_home)

files = filter(lambda x: x[-3:] == '.py' and x not in ("BuildIDE.py", "PythonIDE.py"), files)

for name in files:
	print "adding", name
	fullpath = os.path.join(ide_home, name)
	id, name = py_resource.frompyfile(fullpath, name[:-3], preload=1,
		ispackage=0)

wresref = Res.OpenResFile(os.path.join(ide_home, "Widgets.rsrc"))
buildtools.copyres(wresref, targetref, [], 0)
