# First attempt at automatically generating CodeWarior projects
import os

Error="gencwproject.Error"
#
# These templates are executed in-order.
# 
TEMPLATELIST= [
	("tmp_allsources", "file", "template-allsources.xml", "sources"),
	("tmp_linkorder", "file", "template-linkorder.xml", "sources"),
	("tmp_grouplist", "file", "template-grouplist.xml", "sources"),
	("tmp_projectxmldata", "file", "template.prj.xml", None)
]

class ProjectBuilder:
	def __init__(self, dict, templatelist=TEMPLATELIST, templatedir=None):
		if templatedir == None:
			try:
				packagedir = os.path.split(__file__)[0]
			except NameError:
				packagedir = os.curdir
			templatedir = os.path.join(packagedir, 'template')
		if not os.path.exists(templatedir):
			raise Error, "Cannot file templatedir"
		self.dict = dict
		self.templatelist = templatelist
		self.templatedir = templatedir
		
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
						self.dict[key] = curkeyvalue
						curkeyvalueresult = self._generate_one_value(datasource, dataname)
						result = result + curkeyvalueresult
				finally:
					# Restore the list
					self.dict[key] = keyvalues
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
		"mac_projectxmlname" : "xxnew.prj.xml",
		"mac_targetname" : "xxnew.ppc",
		"mac_dllname" : "xxnew.ppc.slb",
		"sources" : ["xxnewmodule.c"],
		"mac_exportname" : "xxnew.prj.exp",
	}
	pb = ProjectBuilder(dict)
	pb.generate()
	fp = open(dict["mac_projectxmlname"], "w")
	fp.write(dict["tmp_projectxmldata"])
	
if __name__ == '__main__':
	_test()
		
