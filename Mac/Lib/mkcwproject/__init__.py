import cwxmlgen
import cwtalker
import os
import AppleEvents
import macfs

def mkproject(outputfile, modulename, settings):
	#
	# Copy the dictionary
	#
	dictcopy = {}
	for k, v in settings.items():
		dictcopy[k] = v
	#
	# Fill in mac-specific values
	#
	dictcopy['mac_projectxmlname'] = outputfile + '.xml'
	dictcopy['mac_exportname'] = os.path.split(outputfile)[1] + '.exp'
	dictcopy['mac_outputdir'] = ':lib:'  # XXX Is this correct??
	dictcopy['mac_dllname'] = modulename + '.ppc.slb'
	dictcopy['mac_targetname'] = modulename + '.ppc'
	if os.path.isabs(dictcopy['sysprefix']):
		dictcopy['mac_sysprefixtype'] = 'Absolute'
	else:
		dictcopy['mac_sysprefixtype'] = 'Project' # XXX not sure this is right...
	#
	# Generate the XML for the project
	#
	xmlbuilder = cwxmlgen.ProjectBuilder(dictcopy)
	xmlbuilder.generate()
	fp = open(dictcopy['mac_projectxmlname'], "w")
	fp.write(dictcopy["tmp_projectxmldata"])
	fp.close()
	#
	# Generate the export file
	#
	fp = open(outputfile + '.exp', 'w')
	fp.write('init%s\n'%modulename)
	fp.close()
	#
	# Generate the project from the xml
	#
	cw = cwtalker.MyCodeWarrior(start=1)
	cw.send_timeout = AppleEvents.kNoTimeOut
##	xmlfss = macfs.FSSpec(dictcopy['mac_projectxmlname'])
##	prjfss = macfs.FSSpec(outputfile)
	xmlfss = dictcopy['mac_projectxmlname']
	prjfss = outputfile
	cw.activate()
	cw.my_mkproject(prjfss, xmlfss)
	
def buildproject(projectfile):
	cw = cwtalker.MyCodeWarrior(start=1)
	cw.send_timeout = AppleEvents.kNoTimeOut
	prjfss = macfs.FSSpec(projectfile)
	cw.open(prjfss)
	cw.Make_Project()	# XXX Should set target
	
def cleanproject(projectfile):
	cw = cwtalker.MyCodeWarrior(start=1)
	cw.send_timeout = AppleEvents.kNoTimeOut
	prjfss = macfs.FSSpec(projectfile)
	cw.open(prjfss)
	cw.Remove_Binaries()
		
