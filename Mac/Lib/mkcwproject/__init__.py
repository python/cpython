import cwxmlgen
import cwtalker
import os
from Carbon import AppleEvents
import macfs

def mkproject(outputfile, modulename, settings, force=0, templatename=None):
	#
	# Copy the dictionary
	#
	dictcopy = {}
	for k, v in settings.items():
		dictcopy[k] = v
	#
	# Generate the XML for the project
	#
	dictcopy['mac_projectxmlname'] = outputfile + '.xml'
	dictcopy['mac_exportname'] = os.path.split(outputfile)[1] + '.exp'
	if not dictcopy.has_key('mac_dllname'):
		dictcopy['mac_dllname'] = modulename + '.ppc.slb'
	if not dictcopy.has_key('mac_targetname'):
		dictcopy['mac_targetname'] = modulename + '.ppc'
	
	xmlbuilder = cwxmlgen.ProjectBuilder(dictcopy, templatename=templatename)
	xmlbuilder.generate()
	if not force:
		# We do a number of checks and all must succeed before we decide to
		# skip the build-project step:
		# 1. the xml file must exist, and its content equal to what we've generated
		# 2. the project file must exist and be newer than the xml file
		# 3. the .exp file must exist
		if os.path.exists(dictcopy['mac_projectxmlname']):
			fp = open(dictcopy['mac_projectxmlname'])
			data = fp.read()
			fp.close()
			if data == dictcopy["tmp_projectxmldata"]:
				if os.path.exists(outputfile) and \
						os.stat(outputfile)[os.path.ST_MTIME] > os.stat(dictcopy['mac_projectxmlname'])[os.path.ST_MTIME]:
					if os.path.exists(outputfile + '.exp'):
						return
	fp = open(dictcopy['mac_projectxmlname'], "w")
	fp.write(dictcopy["tmp_projectxmldata"])
	fp.close()
	#
	# Generate the export file
	#
	fp = open(outputfile + '.exp', 'w')
	fp.write('init%s\n'%modulename)
	if dictcopy.has_key('extraexportsymbols'):
		for sym in dictcopy['extraexportsymbols']:
			fp.write('%s\n'%sym)
	fp.close()
	#
	# Generate the project from the xml
	#
	makeproject(dictcopy['mac_projectxmlname'], outputfile)
	
def makeproject(xmlfile, projectfile):
	cw = cwtalker.MyCodeWarrior(start=1)
	cw.send_timeout = AppleEvents.kNoTimeOut
	xmlfss = macfs.FSSpec(xmlfile)
	prjfss = macfs.FSSpec(projectfile)
	cw.my_mkproject(prjfss, xmlfss)
	
def buildproject(projectfile):
	cw = cwtalker.MyCodeWarrior(start=1)
	cw.send_timeout = AppleEvents.kNoTimeOut
	prjfss = macfs.FSSpec(projectfile)
	cw.open(prjfss)
	cw.Make_Project()	# XXX Should set target
	cw.Close_Project()
	
def cleanproject(projectfile):
	cw = cwtalker.MyCodeWarrior(start=1)
	cw.send_timeout = AppleEvents.kNoTimeOut
	prjfss = macfs.FSSpec(projectfile)
	cw.open(prjfss)
	cw.Remove_Binaries()
		
