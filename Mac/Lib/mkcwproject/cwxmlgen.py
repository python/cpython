# First attempt at automatically generating CodeWarior projects
import os
import MacOS
import string

Error="gencwproject.Error"
#
# These templates are executed in-order.
# 
TEMPLATELIST= [
	("tmp_allsources", "file", "template-allsources.xml", "sources"),
	("tmp_linkorder", "file", "template-linkorder.xml", "sources"),
	("tmp_grouplist", "file", "template-grouplist.xml", "sources"),
	("tmp_alllibraries", "file", "template-alllibraries.xml", "libraries"),
	("tmp_linkorderlib", "file", "template-linkorderlib.xml", "libraries"),
	("tmp_grouplistlib", "file", "template-grouplistlib.xml", "libraries"),
	("tmp_extrasearchdirs", "file", "template-searchdirs.xml", "extrasearchdirs"),
	("tmp_projectxmldata", "file", "template.prj.xml", None)
]

class ProjectBuilder:
	def __init__(self, dict, templatelist=TEMPLATELIST, templatename=None):
		self._adddefaults(dict)
		if templatename == None:
			if hasattr(MacOS, 'runtimemodel'):
				templatename = 'template-%s'%MacOS.runtimemodel
			else:
				templatename = 'template'
		if os.sep in templatename:
			templatedir = templatename
		else:
			try:
				packagedir = os.path.split(__file__)[0]
			except NameError:
				packagedir = os.curdir
			templatedir = os.path.join(packagedir, templatename)
		if not os.path.exists(templatedir):
			raise Error, "Cannot find templatedir %s"%templatedir
		self.dict = dict
		if not dict.has_key('prefixname'):
			if hasattr(MacOS, 'runtimemodel') and MacOS.runtimemodel == "carbon":
				dict['prefixname'] = 'mwerks_carbonplugin_config.h'
			else:
				dict['prefixname'] = 'mwerks_plugin_config.h'
		self.templatelist = templatelist
		self.templatedir = templatedir
	
	def _adddefaults(self, dict):
		# Set all suitable defaults set for values which were omitted.
		if not dict.has_key('mac_outputdir'):
			dict['mac_outputdir'] = ':lib:'
		if not dict.has_key('stdlibraryflags'):
			dict['stdlibraryflags'] = 'Debug'
		if not dict.has_key('libraryflags'):
			dict['libraryflags'] = 'Debug'
		if not dict.has_key('mac_sysprefixtype'):
			if os.path.isabs(dict['sysprefix']):
				dict['mac_sysprefixtype'] = 'Absolute'
			else:
				dict['mac_sysprefixtype'] = 'Project' # XXX not sure this is right...
		
	def generate(self):
		for tmpl in self.templatelist:
			self._generate_one_template(tmpl)
		
	def _generate_one_template(self, tmpl):
		resultname, datasource, dataname, key = tmpl
		result = ''
		if key:
			# This is a multi-element rule. Run for every item in dict[key]
			if self.dict.has_key(key):
				keyvalues = self.dict[key]
				try:
					if not type(keyvalues) in (type(()), type([])):
						raise Error, "List or tuple expected for %s"%key
					for curkeyvalue in keyvalues:
						if string.lower(curkeyvalue[:10]) == '{compiler}':
							curkeyvalue = curkeyvalue[10:]
							self.dict['pathtype'] = 'CodeWarrior'
						elif string.lower(curkeyvalue[:9]) == '{project}':
							curkeyvalue = curkeyvalue[9:]
							self.dict['pathtype'] = 'Project'
						elif curkeyvalue[0] == '{':
							raise Error, "Unknown {} escape in %s"%curkeyvalue
						elif os.path.isabs(curkeyvalue):
							self.dict['pathtype'] = 'Absolute'
						else:
							self.dict['pathtype'] = 'Project'
						if curkeyvalue[-2:] == ':*':
							curkeyvalue = curkeyvalue[:-2]
							self.dict['recursive'] = 'true'
						else:
							self.dict['recursive'] = 'false'
						self.dict[key] = curkeyvalue
						curkeyvalueresult = self._generate_one_value(datasource, dataname)
						result = result + curkeyvalueresult
				finally:
					# Restore the list
					self.dict[key] = keyvalues
					self.dict['pathtype'] = None
					del self.dict['pathtype']
					self.dict['recursive'] = None
					del self.dict['recursive']
		else:
			# Not a multi-element rule. Simply generate
			result = self._generate_one_value(datasource, dataname)
		# And store the result
		self.dict[resultname] = result
		
	def _generate_one_value(self, datasource, dataname):
		if datasource == 'file':
			filepath = os.path.join(self.templatedir, dataname)
			fp = open(filepath, "r")
			format = fp.read()
		elif datasource == 'string':
			format = dataname
		else:
			raise Error, 'Datasource should be file or string, not %s'%datasource
		return format % self.dict
		
def _test():
	dict = {
		"mac_projectxmlname" : "controlstrip.prj.xml",	# The XML filename (full path)
		"mac_exportname" : "controlstrip.prj.exp",	# Export file (relative to project)
		"mac_outputdir" : ":",	# The directory where the DLL is put (relative to project)
		"mac_dllname" : "controlstrip.ppc.slb",	# The DLL filename (within outputdir)
		"mac_targetname" : "controlstrip.ppc",	# The targetname within the project
		"sysprefix" : sys.prefix,	# Where the Python sources live
		"mac_sysprefixtype" : "Absolute",	# Type of previous pathname
		"sources" : ["controlstripmodule.c"],
		"extrasearchdirs": [],	# -I and -L, in unix terms
	}
	pb = ProjectBuilder(dict)
	pb.generate()
	fp = open(dict["mac_projectxmlname"], "w")
	fp.write(dict["tmp_projectxmldata"])
	
if __name__ == '__main__':
	_test()
		
