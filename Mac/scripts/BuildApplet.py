"""Create an applet from a Python script.

This puts up a dialog asking for a Python source file ('TEXT').
The output is a file with the same name but its ".py" suffix dropped.
It is created by copying an applet template and then adding a 'PYC '
resource named __main__ containing the compiled, marshalled script.
"""


import sys
sys.stdout = sys.stderr

import os
import macfs
import MacOS
import EasyDialogs
import buildtools


def main():
	try:
		buildapplet()
	except buildtools.BuildError, detail:
		EasyDialogs.Message(detail)


def buildapplet():
	buildtools.DEBUG=1
	
	# Find the template
	# (there's no point in proceeding if we can't find it)
	
	template = buildtools.findtemplate()
	
	# Ask for source text if not specified in sys.argv[1:]
	
	if not sys.argv[1:]:
		srcfss, ok = macfs.PromptGetFile('Select Python source or applet:', 'TEXT', 'APPL')
		if not ok:
			return
		filename = srcfss.as_pathname()
		tp, tf = os.path.split(filename)
		if tf[-3:] == '.py':
			tf = tf[:-3]
		else:
			tf = tf + '.applet'
		dstfss, ok = macfs.StandardPutFile('Save application as:', tf)
		if not ok: return
		dstfilename = dstfss.as_pathname()
		cr, tp = MacOS.GetCreatorAndType(filename)
		if tp == 'APPL':
			buildtools.update(template, filename, dstfilename)
		else:
			buildtools.process(template, filename, dstfilename, 1)
	else:
		
		# Loop over all files to be processed
		for filename in sys.argv[1:]:
			cr, tp = MacOS.GetCreatorAndType(filename)
			if tp == 'APPL':
				buildtools.update(template, filename, '')
			else:
				buildtools.process(template, filename, '', 1)


if __name__ == '__main__':
	main()
