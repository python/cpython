import CodeWarrior
import aetools
import aetypes

# There is both a class "project document" and a property "project document".
# We want the class, but the property overrides it.
#
##class project_document(aetools.ComponentItem):
##	"""project document - a project document """
##	want = 'PRJD'
project_document=aetypes.Type('PRJD')

class MyCodeWarrior(CodeWarrior.CodeWarrior):
	# Bug in the CW OSA dictionary
	def export(self, object, _attributes={}, **_arguments):
		"""export: Export the project file as an XML file
		Keyword argument _in: the XML file in which to export the project
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CWIE'
		_subcode = 'EXPT'

		aetools.keysubst(_arguments, self._argmap_export)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def my_mkproject(self, prjfile, xmlfile):
		self.make(new=project_document, with_data=xmlfile, as=prjfile)
