"""BuildCGIApplet.py -- Create a CGI applet from a Python script.

Specilized version of BuildApplet, enabling Python CGI scripts to be
used under Mac web servers like WebStar. The __main__ program is
PythonCGISlave.py, which provides a compatibility layer, emulating
Unix-style CGI scripts. See CGI_README.txt for details.
"""

import sys
import os
import macfs
import MacOS
import Res
import EasyDialogs
import buildtools
import py_resource


def main():
	try:
		buildcgiapplet()
	except buildtools.BuildError, detail:
		EasyDialogs.Message(detail)


def buildcgiapplet():
	buildtools.DEBUG=1
	
	# Find the template
	# (there's no point in proceeding if we can't find it)
	
	template = buildtools.findtemplate()
	wrapper = "PythonCGISlave.py"
	if not os.path.exists("PythonCGISlave.py"):
		wrapper = os.path.join(sys.exec_prefix, ":Mac:Tools:CGI", wrapper)
	
	# Ask for source text if not specified in sys.argv[1:]
	if not sys.argv[1:]:
		srcfss, ok = macfs.PromptGetFile('Select a CGI script:', 'TEXT', 'APPL')
		if not ok:
			return
		filename = srcfss.as_pathname()
		dstfilename = mkcgifilename(filename)
		dstfss, ok = macfs.StandardPutFile('Save application as:', 
				os.path.basename(dstfilename))
		if not ok:
			return
		dstfilename = dstfss.as_pathname()
		buildone(template, wrapper, filename, dstfilename)
	else:
		# Loop over all files to be processed
		for filename in sys.argv[1:]:
			dstfilename = mkcgifilename(filename)
			buildone(template, wrapper, filename, dstfilename)


def mkcgifilename(filename):
	if filename[-3:] == '.py':
		filename = filename[:-3]
	filename = filename + ".cgi"
	return filename


def buildone(template, wrapper, src, dst):
	buildtools.process(template, wrapper, dst, 1)
	# write source as a PYC resource into dst
	ref = Res.FSpOpenResFile(dst, 1)
	try:
		Res.UseResFile(ref)
		py_resource.frompyfile(src, "CGI_MAIN", preload=1)
	finally:
		Res.CloseResFile(ref)


if __name__ == '__main__':
	main()
