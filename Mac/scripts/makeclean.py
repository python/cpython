"""	  ***DANGEROUS***
	  script to remove 
	  all results of a 
	   build process. 

	    ***Don't*** 
	run this if you are
	     ***not***
	  building Python 
	  from the source
	        !!!
"""

import macfs
import os
import sys
import re

sweepfiletypes = [
	'APPL', 	# applications
	'Atmp',		# applet template
	'shlb', 	# shared libs
	'MPSY',		# SYM and xSYM files
	'PYC ',		# .pyc files
	]

sweepfolderre = re.compile(r"(.*) Data$")


def remove(top):
	if os.path.isdir(top):
		for name in os.listdir(top):
			path = os.path.join(top, name)
			remove(path)
	os.remove(top)


def walk(top):
	if os.path.isdir(top):
		m = sweepfolderre.match(top)
		if m and os.path.exists(m.group(1) + ".prj"):
			print "removing folder:", top
			remove(top)
		else:
			for name in os.listdir(top):
				path = os.path.join(top, name)
				walk(path)
	else:
		fss = macfs.FSSpec(top)
		cr, tp = fss.GetCreatorType()
		if tp in sweepfiletypes and top <> sys.executable:
			print "removing file:  ", top
			remove(top)
		

fss, ok = macfs.GetDirectory("Please locate the Python home directory")
if ok:
	walk(fss.as_pathname())
	sys.exit(1)  # so we see the results
