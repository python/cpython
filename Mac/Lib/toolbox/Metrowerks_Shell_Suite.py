"""Suite Metrowerks Shell Suite: Events supported by the Metrowerks Project Shell
Level 1, version 1

Generated from Sap:CodeWarrior6:Metrowerks C/C++:MW C/C++ PPC 1.2.2
AETE/AEUT resource version 1/0, language 0, script 0
"""

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')

import aetools
import MacOS

_code = 'MMPR'

_Enum_savo = {
	'yes' : 'yes ',	# Save changes
	'no' : 'no  ',	# Do not save changes
	'ask' : 'ask ',	# Ask the user whether to save
}

_Enum_ErrT = {
	'information' : 'ErIn',	# 
	'compiler_error' : 'ErCE',	# 
	'compiler_warning' : 'ErCW',	# 
	'definition' : 'ErDf',	# 
	'linker_error' : 'ErLE',	# 
	'linker_warning' : 'ErLW',	# 
}

_Enum_Mode = {
	'ReadWrite' : 'RdWr',	# The file is open with read/write privileges
	'ReadOnly' : 'Read',	# The file is open with read/only privileges
	'CheckedOut_ReadWrite' : 'CkRW',	# The file is checked out with read/write privileges
	'CheckedOut_ReadOnly' : 'CkRO',	# The file is checked out with read/only privileges
	'CheckedOut_ReadModify' : 'CkRM',	# The file is checked out with read/modify privileges
	'Locked' : 'Lock',	# The file is locked on disk
	'None' : 'None',	# The file is new
}

_Enum_SrcT = {
	'source' : 'FTxt',	# A source file (.c, .cp, .p, etc).
	'library' : 'FLib',	# An object code library.
	'shared_library' : 'FShl',	# A shared library.
	'resource' : 'FRes',	# A resource file.
	'xcoff' : 'FXco',	# An XCOFF library.
}

class Metrowerks_Shell_Suite:

	_argmap_Close_Window = {
		'Saving' : 'savo',
	}

	def Close_Window(self, object, *arguments):
		"""Close Window: Close the windows showing the specified files
		Required argument: The files to close
		Keyword argument Saving: Whether to save changes to each file before closing its window
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'ClsW'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Close_Window)
		aetools.enumsubst(arguments, 'savo', _Enum_savo)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Add_Files = {
		'To_Segment' : 'Segm',
	}

	def Add_Files(self, object, *arguments):
		"""Add Files: Add the specified file(s) to the current project
		Required argument: List of files to add
		Keyword argument To_Segment: Segment number into which to add the file(s)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each file added
		"""
		_code = 'MMPR'
		_subcode = 'AddF'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Add_Files)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Remove_Files(self, object, *arguments):
		"""Remove Files: Remove the specified file(s) from the current project
		Required argument: List of files to remove
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each file removed
		"""
		_code = 'MMPR'
		_subcode = 'RemF'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Touch(self, object, *arguments):
		"""Touch: Touch the modification date of the specified file(s)
		Required argument: List of files to compile
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each file touched
		"""
		_code = 'MMPR'
		_subcode = 'Toch'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Check_Syntax = {
		'ExternalEditor' : 'Errs',
	}

	def Check_Syntax(self, object, *arguments):
		"""Check Syntax: Check the syntax of the specified file(s)
		Required argument: List of files to check the syntax of
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for each file whose syntax was checked
		"""
		_code = 'MMPR'
		_subcode = 'Chek'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Check_Syntax)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Compile = {
		'ExternalEditor' : 'Errs',
	}

	def Compile(self, object, *arguments):
		"""Compile: Compile the specified file(s)
		Required argument: List of files to compile
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for each file compiled
		"""
		_code = 'MMPR'
		_subcode = 'Comp'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Compile)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Precompile = {
		'Saving_As' : 'Targ',
		'ExternalEditor' : 'Errs',
	}

	def Precompile(self, object, *arguments):
		"""Precompile: Precompile the specified file to the specified destination file
		Required argument: File to precompile
		Keyword argument Saving_As: Destination file for precompiled header
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for the precompiled file
		"""
		_code = 'MMPR'
		_subcode = 'PreC'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Precompile)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Create_Project = {
		'from_stationery' : 'Tmpl',
	}

	def Create_Project(self, object, *arguments):
		"""Create Project: Create a new project file
		Required argument: New project file specifier
		Keyword argument from_stationery: undocumented, typecode 'alis'
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'NewP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Create_Project)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Close_Project(self, *arguments):
		"""Close Project: Close the current project
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'ClsP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Remove_Binaries(self, *arguments):
		"""Remove Binaries: Remove the binary object code from the current project
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'RemB'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Update_Project = {
		'ExternalEditor' : 'Errs',
	}

	def Update_Project(self, *arguments):
		"""Update Project: Update the current project
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors that occurred while updating the project
		"""
		_code = 'MMPR'
		_subcode = 'UpdP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Update_Project)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Make_Project = {
		'ExternalEditor' : 'Errs',
	}

	def Make_Project(self, *arguments):
		"""Make Project: Make the current project
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors that occurred while making the project
		"""
		_code = 'MMPR'
		_subcode = 'Make'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Make_Project)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Run_Project = {
		'ExternalEditor' : 'Errs',
		'SourceDebugger' : 'DeBg',
	}

	def Run_Project(self, *arguments):
		"""Run Project: Run the current project
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument SourceDebugger: Run the application under the control of the source-level debugger
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors that occurred when running the project
		"""
		_code = 'MMPR'
		_subcode = 'RunP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Run_Project)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Set_Preferences = {
		'To' : 'PRec',
	}

	def Set_Preferences(self, *arguments):
		"""Set Preferences: Set the preferences for the current project
		Keyword argument To: Preferences settings
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'Pref'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Set_Preferences)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Get_Preferences = {
		'Of' : 'PRec',
	}

	def Get_Preferences(self, *arguments):
		"""Get Preferences: Get the preferences for the current project
		Keyword argument Of: Preferences settings
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: List of preferences
		"""
		_code = 'MMPR'
		_subcode = 'Gref'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Get_Preferences)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Is_In_Project(self, object, *arguments):
		"""Is In Project: Whether or not the specified file(s) is in the current project
		Required argument: List of files to check for project membership
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Result code for each file
		"""
		_code = 'MMPR'
		_subcode = 'FInP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Get_Project_Specifier(self, *arguments):
		"""Get Project Specifier: Return the File Specifier for the current project
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: File Specifier for the current project
		"""
		_code = 'MMPR'
		_subcode = 'GetP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Reset_File_Paths(self, *arguments):
		"""Reset File Paths: Resets access paths for all files belonging to open project.
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'ReFP'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Goto_Line(self, object, *arguments):
		"""Goto Line: Goto Specified Line Number
		Required argument: The requested source file line number
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'GoLn'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Goto_Function(self, object, *arguments):
		"""Goto Function: Goto Specified Function Name
		Required argument: undocumented, typecode 'TEXT'
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'GoFn'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Save_Message_Window_As(self, object, *arguments):
		"""Save Message Window As: Saves the message window as a text file
		Required argument: Destination file for Save Message Window As
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SvMs'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Get_Open_Documents(self, *arguments):
		"""Get Open Documents: Returns the list of open documents
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The list of documents
		"""
		_code = 'MMPR'
		_subcode = 'GDoc'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Get_Segments(self, *arguments):
		"""Get Segments: Returns a description of each segment in the project.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'Seg '
		"""
		_code = 'MMPR'
		_subcode = 'GSeg'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Set_Segment = {
		'to' : 'Segm',
	}

	def Set_Segment(self, object, *arguments):
		"""Set Segment: Changes the name and attributes of a segment.
		Required argument: The segment to change
		Keyword argument to: The new name and attributes for the segment.
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SSeg'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Set_Segment)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Get_Project_File = {
		'Segment' : 'Segm',
	}

	def Get_Project_File(self, object, *arguments):
		"""Get Project File: Returns a description of a file in the project window.
		Required argument: The index of the file within its segment.
		Keyword argument Segment: The segment containing the file.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'SrcF'
		"""
		_code = 'MMPR'
		_subcode = 'GFil'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Get_Project_File)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_Set_Project_File = {
		'to' : 'SrcS',
	}

	def Set_Project_File(self, object, *arguments):
		"""Set Project File: Changes the settings for a given file in the project.
		Required argument: The name of the file
		Keyword argument to: The new settings for the file
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SFil'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_Set_Project_File)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def Get_Definition(self, object, *arguments):
		"""Get Definition: Returns the location(s) of a globally scoped function or data object.
		Required argument: undocumented, typecode 'TEXT'
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'FDef'
		"""
		_code = 'MMPR'
		_subcode = 'GDef'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']


#    Class 'C/C++ Language' ('FECP') -- 'C/C++ Language specifics'
#        property 'Activate CPlusPlus' ('FE01') 'bool' -- 'To compile \322.c\323 files that use C++ features.' [mutable]
#        property 'ARM Conformance' ('FE02') 'bool' -- 'To enforce ARM conformance for your source code.' [mutable]
#        property 'ANSI Keywords Only' ('FE03') 'bool' -- 'To allow only ANSI C and C++ keywords.' [mutable]
#        property 'Require Function Prototypes' ('FE04') 'bool' -- 'To enforce prototype checking.' [mutable]
#        property 'Expand Trigraph Sequences' ('FE05') 'bool' -- 'To allow ANSI trigraph expressions.' [mutable]
#        property 'Enums Always Ints' ('FE06') 'bool' -- 'To interpret enumeration constants as integers.' [mutable]
#        property 'MPW Pointer Type Rules' ('FE07') 'bool' -- 'To use MPW (relaxed) pointer type-checking rules.' [mutable]
#        property 'Direct Destruction' ('FE09') 'bool' -- 'To remove local and temporary C++ class objects that have destructors with a direct destructor call.' [mutable]
#        property 'Dont Inline' ('FE10') 'bool' -- 'To generate C++ functions and function calls instead of inline functions.' [mutable]
#        property 'Pool Strings' ('FE11') 'bool' -- 'To group all character string constants into a single object.' [mutable]
#        property 'Dont Reuse Strings' ('FE12') 'bool' -- 'To prevent merging identical character string literals.' [mutable]
#        property 'ANSI Strict' ('FE13') 'bool' -- 'To enforce strict ANSI C and ANSI C++ rules.' [mutable]
#        property 'MPW Newlines' ('FE14') 'bool' -- 'To convert \324\\n\325 to carriage returns (ASCII 13) and \324\\r\325 to line feeds (ASCII 10).' [mutable]
#        property 'Illegal Pragmas' ('WA01') 'bool' -- 'To enforce illegal pragma checking.' [mutable]
#        property 'Empty Declarations' ('WA02') 'bool' -- 'To enforce empty declaration checking.' [mutable]
#        property 'Possible Errors' ('WA03') 'bool' -- 'To enforce possible error checking.' [mutable]
#        property 'Extra Commas' ('WA06') 'bool' -- 'To enforce extra comma checking.' [mutable]
#        property 'Extended Error Checking' ('WA07') 'bool' -- 'To enforce extended error checking.' [mutable]
#        property 'Treat Warnings As Errors' ('WA08') 'bool' -- 'To treat the warnings as compiler errors.' [mutable]

#    Class 'Pascal Language' ('FEPA') -- 'Pascal Language specifics'
#        property 'Activate Range Checking' ('FP01') 'bool' -- 'To activate array index reference and variable assignment checking at compile and run time.' [mutable]
#        property 'Use Propagation' ('FP03') 'bool' -- 'To propagate unit objects among imported objects.' [mutable]
#        property 'Activate Overflow Checking' ('FP04') 'bool' -- 'To activate run-time overflow checking.' [mutable]
#        property 'Generate Map' ('FP05') 'bool' -- 'To generate a Make map file.' [mutable]
#        property 'Case Sensitive' ('FP06') 'bool' -- 'To support case sensitive identifier.' [mutable]
#        property 'ANS Conformance' ('FP07') 'bool' -- 'To conform to the ANS standard.' [mutable]
#        property 'Modified ForLoop Indexes' ('WP01') 'bool' -- 'To verify that for loop indexes are not overwritten.' [mutable]
#        property 'Function Returns' ('WP02') 'bool' -- 'To verify that functions get at least one return value.' [mutable]
#        property 'Undefined Routines' ('WP03') 'bool' -- 'To report routines declared but not defined.' [mutable]
#        property 'GotoAndLabels' ('WP04') 'bool' -- 'To enable goto checking in for, with, case and if statements.' [mutable]
#        property 'BranchingIntoWith' ('WP05') 'bool' -- 'Verify that labels in a WITH statement are referenced from within the same statement' [mutable]
#        property 'BranchingIntoFor' ('WP06') 'bool' -- 'Verify that labels in a FOR statement are referenced from within the same statement' [mutable]
#        property 'BranchingBetweenCase' ('WP07') 'bool' -- 'Verify that labels in a CASE statement leg are not referenced from within another leg of the same statement.' [mutable]
#        property 'BranchingBetweenIfAndElse' ('WP08') 'bool' -- 'Verify that there\325s no jump between the IF and the ELSE part of the same IF statement.' [mutable]

#    Class 'Common Language' ('FECO') -- 'Common to the C/C++ and Pascal language'
#        property 'Prefix File' ('FE08') 'TEXT' -- 'The name of the prefix file you wish to include in all of your project files.' [mutable]
#        property 'Unused Variables' ('WA04') 'bool' -- 'To enforce unused variable checking.' [mutable]
#        property 'Unused Arguments' ('WA05') 'bool' -- 'To enforce unused argument checking.' [mutable]

#    Class 'MC68K Linker' ('LN68') -- 'MC68K Linker specifics'
#        property 'Code Model' ('BE01') 'shor' -- 'To select the type of object code you wish to generate. (0 : Small; 1 : Smart; 2 : Large)' [mutable]
#        property 'MC68020 CodeGen' ('BE03') 'bool' -- 'To generate object code for computers with the MC68020, MC68030, or MC68040 processor.' [mutable]
#        property 'MC68881 CodeGen' ('BE04') 'bool' -- 'To reroute all math calls from SANE directly to a MC68881 or MC68882 FPU.' [mutable]
#        property 'Far Virtual Function Tables' ('BE08') 'bool' -- 'To make far branches in subroutines. (Not supported in Pascal)' [mutable]
#        property 'Far String Constants' ('BE09') 'bool' -- 'To place all string constants into far data space.' [mutable]
#        property 'Four Bytes Ints' ('BE10') 'bool' -- 'To force integers to occupy 4 bytes. (Not supported in Pascal)' [mutable]
#        property 'Eight Byte Double' ('BE11') 'bool' -- 'To force doubles to occupy 8 bytes.' [mutable]
#        property 'Far Data' ('BE12') 'bool' -- 'To remove the 68K limit on global data.' [mutable]
#        property 'PC Relative Strings' ('BE14') 'bool' -- 'To reference string literals using PC-relative addressing.' [mutable]
#        property 'MPW Calling Conventions' ('BE15') 'bool' -- 'To use MPW C calling conventions.' [mutable]
#        property 'MacsBug Symbols' ('LN01') 'shor' -- 'To select the MacsBug version you are using. ( 0 : None; 1 : Old Style; 2 : New Style)' [mutable]
#        property 'Generate A6 Stack Frames' ('LN05') 'bool' -- 'To be able to use the Metrowerks Debugger, The Debugger by Jasik Designs, or SourceBug\252 to examine your source code.' [mutable]
#        property 'The Debugger Aware' ('LN06') 'bool' -- 'To allow Steve Jasik\325s The Debugger to debug multiple segment projects.' [mutable]
#        property 'Link Single Segment' ('LN07') 'bool' -- 'To link all of your project segments into one segment.' [mutable]
#        property 'Multi Segment' ('PR10') 'bool' -- 'To divide your resource into many code segments in your resource file.' [mutable]
#        property 'Header Type' ('PR16') 'shor' -- 'To select the type of code resource to create. (Not supported yet)' [mutable]
#        property 'A4 Relative Data' ('PR17') 'bool' -- 'To use A4 relative data.' [mutable]
#        property 'Seg Type' ('PR18') 'shor' -- 'To set the segment type for your code resource.' [mutable]

#    Class 'PowerPC Linker' ('LNCP') -- 'PowerPC Linker specifics'
#        property 'Make String ReadOnly' ('B601') 'bool' -- 'To store string literals in the code section.' [mutable]
#        property 'Instruction Scheduling' ('B602') 'bool' -- 'To rearrange instructions to take advantage of the RISC pipelined architecture.' [mutable]
#        property 'Optimization Level' ('B603') 'shor' -- 'To select the level of optimization to be applied (from 1 to 4).' [mutable]
#        property 'Global Optimization' ('B604') 'bool' -- 'To turn on the global optimizer.' [mutable]
#        property 'Suppress Warnings' ('L601') 'bool' -- 'To suppress reporting of non-fatal linker errors.' [mutable]
#        property 'Initialization Name' ('L602') 'TEXT' -- 'The name of the function to call, when the PEF fragment is loaded.' [mutable]
#        property 'Main Name' ('L603') 'TEXT' -- 'The name of the main function.' [mutable]
#        property 'Termination Name' ('L604') 'TEXT' -- 'The name of the last function to call, before the PEF fragment is unloaded.' [mutable]
#        property 'Export Symbols' ('PE01') 'shor' -- 'To select which symbols to export from the PEF container.' [mutable]
#        property 'Old Definition' ('PE02') 'long' -- 'The version number for the PEF container.' [mutable]
#        property 'Old Implementation' ('PE03') 'long' -- 'The version number for the PEF container.' [mutable]
#        property 'Current Version' ('PE04') 'long' -- 'The version number for the PEF container.' [mutable]
#        property 'Order Code Section By Segment' ('PE05') 'bool' -- 'To order functions in the Code section of your program according to their #pragma segment names.' [mutable]
#        property 'Share Data Section' ('PE06') 'bool' -- 'To share the Data section of a shared library between all applications that use it.' [mutable]
#        property 'Expand Uninitialized Data' ('PE07') 'bool' -- 'To expand the zero-initialized portion of the Data section of your program at link time.' [mutable]
#        property 'Fragment Name' ('PE08') 'TEXT' -- 'To specify the code fragment name. The default is the output file name.' [mutable]
#        property 'Stack Size' ('P601') 'long' -- 'The minimum amount of stack your application needs when launched by the Finder.' [mutable]

#    Class 'Common Linker' ('LNCO') -- 'Common for the MC68K and PowerPC Linker'
#        property 'Struct Alignment' ('BE02') 'shor' -- 'To select how the fields of your structures will be aligned. (0 : 68K; 1 : 68K 4 bytes; 2 : PowerPC)' [mutable]
#        property 'Peephole Optimizer' ('BE05') 'bool' -- 'To apply local optimizations to small sections of your code.' [mutable]
#        property 'CSE Optimizer' ('BE06') 'bool' -- 'To activate common sub expression optimization.' [mutable]
#        property 'Optimize For Size' ('BE07') 'bool' -- 'To minimize the size of your application or code resource.' [mutable]
#        property 'Use Profiler' ('BE13') 'bool' -- 'To add profiler calls in the code.' [mutable]
#        property 'Generate SYM File' ('LN02') 'bool' -- 'To create a SYM file for your project when it is built.' [mutable]
#        property 'Full Path In Sym Files' ('LN03') 'bool' -- 'To put the full path of your application or code resource in your SYM files.' [mutable]
#        property 'Generate Link Map' ('LN04') 'bool' -- 'To create a MAP file for your project when it is built.' [mutable]
#        property 'Fast Link' ('LN08') 'bool' -- 'To link the code together in memory.' [mutable]
#        property 'Project Type' ('PR01') 'shor' -- 'To select the type of binary you wish to create.\015\015for 68K : (0 : Application; 1 : Code Resource; 2 : Library; 3 : MPW Tool) for PowerPC : (0 : Application; 1 : Shared Library; 2 : Code Resource; 3 : Library)' [mutable]
#        property 'File Name' ('PR02') 'TEXT' -- 'The name your binary will be given when created.' [mutable]
#        property 'File Creator' ('PR03') 'TEXT' -- 'The creator name of your binary.' [mutable]
#        property 'File Type' ('PR04') 'TEXT' -- 'The four character code type for your binary.' [mutable]
#        property 'Minimum Size' ('PR05') 'long' -- 'The minimum amount of RAM your application needs when launched by the Finder.' [mutable]
#        property 'Preferred Size' ('PR06') 'long' -- 'The preferred amount of RAM your application needs when launched by the Finder.' [mutable]
#        property 'SIZE Flags' ('PR07') 'shor' -- 'To tell the Finder which type of events your application accepts.' [mutable]
#        property 'SYM File' ('PR08') 'TEXT' -- 'The name of the SYM file to be generated when your binary is created.' [mutable]
#        property 'Resource Name' ('PR09') 'TEXT' -- 'The name of the code resource to be created.' [mutable]
#        property 'Display Dialog' ('PR11') 'bool' -- 'To display a dialog box which asks you where to save your built resource.' [mutable]
#        property 'Merge To File' ('PR12') 'bool' -- 'To create and merge your resource into a file which already exists.' [mutable]
#        property 'Resource Flags' ('PR13') 'bool' -- 'To select your resource flag options.' [mutable]
#        property 'Resource Type' ('PR14') 'shor' -- 'The four character code type for your code resource.' [mutable]
#        property 'Resource ID' ('PR15') 'shor' -- 'The ID to assign to your code resource.' [mutable]

#    Class 'Common' ('COMM') -- 'Common to C/C++ and Pascal Language & MC68K and PowerPC Linker.'
#        property 'User Paths' ('PA01') 'list' -- 'To add an access path for the source files.' [mutable]
#        property 'Always Full Search' ('PA02') 'bool' -- 'To force the compiler to search for system includes like it searches for user includes.' [mutable]
#        property 'System Paths' ('PA03') 'list' -- 'To add an access path for the include files. (Not supported in Pascal)' [mutable]

#    Class 'Error Information' ('ErrM') -- 'Describes a single error or warning from the compiler or the linker.'
#        property 'kind' ('ErrT') 'ErrT' -- 'The type of error or warning.' [enum]
#        property 'message' ('ErrS') 'TEXT' -- 'The error or warning message.' []
#        property 'disk file' ('file') 'fss ' -- 'The file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors).' []
#        property 'line' ('ErrL') 'long' -- 'The line in the file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors).' []

#    Class 'Document' ('docu') -- 'An open text file'
#        property 'name' ('pnam') 'TEXT' -- 'The document\325s name' []
#        property 'mode' ('Mode') 'Mode' -- 'The document\325s open mode' [enum]
#        property 'disk file' ('file') 'fss ' -- 'The document\325s location on disk' []

#    Class 'Segment' ('Seg ') -- 'A segment or group in the project'
#        property 'name' ('pnam') 'TEXT' -- '' [mutable]
#        property 'filecount' ('NumF') 'shor' -- '' []
#        property 'preloaded' ('Prel') 'bool' -- 'Is the segment preloaded ? [68K only]' [mutable]
#        property 'protected' ('Prot') 'bool' -- 'Is the segment protected ? [68K only]' [mutable]
#        property 'locked' ('PLck') 'bool' -- 'Is the segment locked ? [68K only]' [mutable]
#        property 'purgeable' ('Purg') 'bool' -- 'Is the segment purgeable ? [68K only]' [mutable]
#        property 'system heap' ('SysH') 'bool' -- 'Is the segment loaded into the system heap ? [68K only]' [mutable]

#    Class 'ProjectFile' ('SrcF') -- 'A file contained in a project'
#        property 'filetype' ('SrcT') 'SrcT' -- 'What kind of file is this ?' [enum]
#        property 'name' ('pnam') 'TEXT' -- 'The file\325s name' []
#        property 'disk file' ('file') 'fss ' -- 'The file\325s location on disk' []
#        property 'codesize' ('CSiz') 'long' -- 'The size of this file\325s code.' []
#        property 'datasize' ('DSiz') 'long' -- 'The size of this file\325s data.' []
#        property 'up to date' ('UpTD') 'bool' -- 'Has the file been compiled since its last modification ?' []
#        property 'symbols' ('SymG') 'bool' -- 'Are debugging symbols generated for this file ?' [mutable]
#        property 'weak link' ('Weak') 'bool' -- 'Is this file imported weakly into the project ? [PowerPC only]' [mutable]

#    Class 'Function Information' ('FDef') -- 'Describes the location of any function or global data definition within the current project.'
#        property 'disk file' ('file') 'fss ' -- 'The location on disk of the file containing the definition.' []
#        property 'line' ('ErrL') 'long' -- 'The line number where the definition begins.' []
