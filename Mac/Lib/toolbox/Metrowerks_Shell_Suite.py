"""Suite Metrowerks Shell Suite: Events supported by the Metrowerks Project Shell
Level 1, version 1

Generated from flap:Metrowerks:Metrowerks CodeWarrior:CodeWarrior IDE 1.7.4
AETE/AEUT resource version 1/0, language 0, script 0
"""

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
	'compiler_warning' : 'ErCW',	# 
	'compiler_error' : 'ErCE',	# 
	'definition' : 'ErDf',	# 
	'linker_warning' : 'ErLW',	# 
	'linker_error' : 'ErLE',	# 
	'find_result' : 'ErFn',	# 
	'generic_error' : 'ErGn',	# 
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
	'unknown' : 'FUnk',	# An unknown file type.
}

_Enum_PPrm = {
	'absolute' : 'Abso',	# An absolute path name, including volume name.
	'project_relative' : 'PRel',	# A path relative to the current projectπs folder.
	'shell_relative' : 'SRel',	# A path relative to the CodeWarriorÅ folder.
}

_Enum_RefP = {
	'Think_Reference' : 'DanR',	# 
	'QuickView' : 'ALTV',	# 
}

_Enum_Lang = {
	'C' : 'LC  ',	# 
	'C_2b__2b_' : 'LC++',	# 
	'Pascal' : 'LP  ',	# 
	'Object_Pascal' : 'LP++',	# 
	'Java' : 'LJav',	# 
	'Assembler' : 'LAsm',	# 
	'Unknown' : 'L?  ',	# 
}

_Enum_Acce = {
	'public' : 'Publ',	# 
	'protected' : 'Prot',	# 
	'private' : 'Priv',	# 
}

class Metrowerks_Shell_Suite:

	_argmap_Add_Files = {
		'To_Segment' : 'Segm',
	}

	def Add_Files(self, _object, _attributes={}, **_arguments):
		"""Add Files: Add the specified file(s) to the current project
		Required argument: List of files to add
		Keyword argument To_Segment: Segment number into which to add the file(s)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each file added
		"""
		_code = 'MMPR'
		_subcode = 'AddF'

		aetools.keysubst(_arguments, self._argmap_Add_Files)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Check_Syntax = {
		'ExternalEditor' : 'Errs',
	}

	def Check_Syntax(self, _object, _attributes={}, **_arguments):
		"""Check Syntax: Check the syntax of the specified file(s)
		Required argument: List of files to check the syntax of
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for each file whose syntax was checked
		"""
		_code = 'MMPR'
		_subcode = 'Chek'

		aetools.keysubst(_arguments, self._argmap_Check_Syntax)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Close_Project(self, _no_object=None, _attributes={}, **_arguments):
		"""Close Project: Close the current project
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'ClsP'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Close_Window = {
		'Saving' : 'savo',
	}

	def Close_Window(self, _object, _attributes={}, **_arguments):
		"""Close Window: Close the windows showing the specified files
		Required argument: The files to close
		Keyword argument Saving: Whether to save changes to each file before closing its window
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'ClsW'

		aetools.keysubst(_arguments, self._argmap_Close_Window)
		_arguments['----'] = _object

		aetools.enumsubst(_arguments, 'savo', _Enum_savo)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Compile = {
		'ExternalEditor' : 'Errs',
	}

	def Compile(self, _object, _attributes={}, **_arguments):
		"""Compile: Compile the specified file(s)
		Required argument: List of files to compile
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for each file compiled
		"""
		_code = 'MMPR'
		_subcode = 'Comp'

		aetools.keysubst(_arguments, self._argmap_Compile)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Create_Project = {
		'from_stationery' : 'Tmpl',
	}

	def Create_Project(self, _object, _attributes={}, **_arguments):
		"""Create Project: Create a new project file
		Required argument: New project file specifier
		Keyword argument from_stationery: undocumented, typecode 'alis'
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'NewP'

		aetools.keysubst(_arguments, self._argmap_Create_Project)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Get_Definition(self, _object, _attributes={}, **_arguments):
		"""Get Definition: Returns the location(s) of a globally scoped function or data object.
		Required argument: undocumented, typecode 'TEXT'
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'FDef'
		"""
		_code = 'MMPR'
		_subcode = 'GDef'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Get_Open_Documents(self, _no_object=None, _attributes={}, **_arguments):
		"""Get Open Documents: Returns the list of open documents
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The list of documents
		"""
		_code = 'MMPR'
		_subcode = 'GDoc'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Get_Preferences = {
		'of' : 'PRec',
		'from_panel' : 'PNam',
	}

	def Get_Preferences(self, _no_object=None, _attributes={}, **_arguments):
		"""Get Preferences: Get the preferences for the current project
		Keyword argument of: Names of requested preferences
		Keyword argument from_panel: Name of the preference panel
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The requested preferences
		"""
		_code = 'MMPR'
		_subcode = 'Gref'

		aetools.keysubst(_arguments, self._argmap_Get_Preferences)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Get_Project_File = {
		'Segment' : 'Segm',
	}

	def Get_Project_File(self, _object, _attributes={}, **_arguments):
		"""Get Project File: Returns a description of a file in the project window.
		Required argument: The index of the file within its segment.
		Keyword argument Segment: The segment containing the file.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'SrcF'
		"""
		_code = 'MMPR'
		_subcode = 'GFil'

		aetools.keysubst(_arguments, self._argmap_Get_Project_File)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Get_Project_Specifier(self, _no_object=None, _attributes={}, **_arguments):
		"""Get Project Specifier: Return the File Specifier for the current project
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: File Specifier for the current project
		"""
		_code = 'MMPR'
		_subcode = 'GetP'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Get_Segments(self, _no_object=None, _attributes={}, **_arguments):
		"""Get Segments: Returns a description of each segment in the project.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'Seg '
		"""
		_code = 'MMPR'
		_subcode = 'GSeg'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Goto_Function(self, _object, _attributes={}, **_arguments):
		"""Goto Function: Goto Specified Function Name
		Required argument: undocumented, typecode 'TEXT'
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'GoFn'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Goto_Line(self, _object, _attributes={}, **_arguments):
		"""Goto Line: Goto Specified Line Number
		Required argument: The requested source file line number
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'GoLn'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Is_In_Project(self, _object, _attributes={}, **_arguments):
		"""Is In Project: Whether or not the specified file(s) is in the current project
		Required argument: List of files to check for project membership
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Result code for each file
		"""
		_code = 'MMPR'
		_subcode = 'FInP'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Make_Project = {
		'ExternalEditor' : 'Errs',
	}

	def Make_Project(self, _no_object=None, _attributes={}, **_arguments):
		"""Make Project: Make the current project
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors that occurred while making the project
		"""
		_code = 'MMPR'
		_subcode = 'Make'

		aetools.keysubst(_arguments, self._argmap_Make_Project)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Precompile = {
		'Saving_As' : 'Targ',
		'ExternalEditor' : 'Errs',
	}

	def Precompile(self, _object, _attributes={}, **_arguments):
		"""Precompile: Precompile the specified file to the specified destination file
		Required argument: File to precompile
		Keyword argument Saving_As: Destination file for precompiled header
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for the precompiled file
		"""
		_code = 'MMPR'
		_subcode = 'PreC'

		aetools.keysubst(_arguments, self._argmap_Precompile)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Preprocess = {
		'ExternalEditor' : 'Errs',
	}

	def Preprocess(self, _object, _attributes={}, **_arguments):
		"""Preprocess: Preprocesses the specified file(s)
		Required argument: undocumented, typecode 'alis'
		Keyword argument ExternalEditor: undocumented, typecode 'bool'
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors for each preprocessed file
		"""
		_code = 'MMPR'
		_subcode = 'PreP'

		aetools.keysubst(_arguments, self._argmap_Preprocess)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Remove_Binaries(self, _no_object=None, _attributes={}, **_arguments):
		"""Remove Binaries: Remove the binary object code from the current project
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'RemB'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Remove_Files(self, _object, _attributes={}, **_arguments):
		"""Remove Files: Remove the specified file(s) from the current project
		Required argument: List of files to remove
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each file removed
		"""
		_code = 'MMPR'
		_subcode = 'RemF'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Reset_File_Paths(self, _no_object=None, _attributes={}, **_arguments):
		"""Reset File Paths: Resets access paths for all files belonging to open project.
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'ReFP'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Run_Project = {
		'ExternalEditor' : 'Errs',
		'SourceDebugger' : 'DeBg',
	}

	def Run_Project(self, _no_object=None, _attributes={}, **_arguments):
		"""Run Project: Run the current project
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument SourceDebugger: Run the application under the control of the source-level debugger
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors that occurred when running the project
		"""
		_code = 'MMPR'
		_subcode = 'RunP'

		aetools.keysubst(_arguments, self._argmap_Run_Project)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Save_Error_Window_As(self, _object, _attributes={}, **_arguments):
		"""Save Error Window As: Saves the Errors & Warnings window as a text file
		Required argument: Destination file for Save Message Window As
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SvMs'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Set_Modification_Date = {
		'to' : 'MDat',
	}

	def Set_Modification_Date(self, _object, _attributes={}, **_arguments):
		"""Set Modification Date: Changes the internal modification date of the specified file(s)
		Required argument: List of files
		Keyword argument to: undocumented, typecode 'ldt '
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each modified file
		"""
		_code = 'MMPR'
		_subcode = 'SMod'

		aetools.keysubst(_arguments, self._argmap_Set_Modification_Date)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Set_Preferences = {
		'of_panel' : 'PNam',
		'to' : 'PRec',
	}

	def Set_Preferences(self, _no_object=None, _attributes={}, **_arguments):
		"""Set Preferences: Set the preferences for the current project
		Keyword argument of_panel: Name of the preference panel
		Keyword argument to: Preferences settings
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'Pref'

		aetools.keysubst(_arguments, self._argmap_Set_Preferences)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Set_Project_File = {
		'to' : 'SrcS',
	}

	def Set_Project_File(self, _object, _attributes={}, **_arguments):
		"""Set Project File: Changes the settings for a given file in the project.
		Required argument: The name of the file
		Keyword argument to: The new settings for the file
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SFil'

		aetools.keysubst(_arguments, self._argmap_Set_Project_File)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Set_Segment = {
		'to' : 'Segm',
	}

	def Set_Segment(self, _object, _attributes={}, **_arguments):
		"""Set Segment: Changes the name and attributes of a segment.
		Required argument: The segment to change
		Keyword argument to: The new name and attributes for the segment.
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SSeg'

		aetools.keysubst(_arguments, self._argmap_Set_Segment)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Touch(self, _object, _attributes={}, **_arguments):
		"""Touch: Force recompilation of the specified file(s)
		Required argument: List of files to compile
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Error code for each file touched
		"""
		_code = 'MMPR'
		_subcode = 'Toch'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Update_Project = {
		'ExternalEditor' : 'Errs',
	}

	def Update_Project(self, _no_object=None, _attributes={}, **_arguments):
		"""Update Project: Update the current project
		Keyword argument ExternalEditor: Should the contents of the message window be returned to the caller?
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Errors that occurred while updating the project
		"""
		_code = 'MMPR'
		_subcode = 'UpdP'

		aetools.keysubst(_arguments, self._argmap_Update_Project)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Open_browser(self, _object, _attributes={}, **_arguments):
		"""Open browser: Display a class, member function, or data member object in a single class browser window
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'Brow'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Get_nonsimple_classes(self, _no_object=None, _attributes={}, **_arguments):
		"""Get nonsimple classes: Returns an alphabetical list of classes with member functions, bases classes, or subclasses
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'list'
		"""
		_code = 'MMPR'
		_subcode = 'NsCl'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Get_member_function_names(self, _object, _attributes={}, **_arguments):
		"""Get member function names: Returns a list containing the names of all the member functions of a class object
		Required argument: must be a class object
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'list'
		"""
		_code = 'MMPR'
		_subcode = 'MbFN'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#    Class 'Access Paths' ('PATH') -- 'Contains the definitions of a project\325s access (search) paths.'
#        property 'User Paths' ('PA01') 'PInf' -- 'To add an access path for the source files.' [mutable list]
#        property 'System Paths' ('PA03') 'PInf' -- 'To add an access path for the include files. (Not supported in Pascal)' [mutable list]
#        property 'Always Full Search' ('PA02') 'bool' -- 'To force the compiler to search for system includes like it searches for user includes.' [mutable]

#    Class 'Editor' ('EDTR') -- ''
#        property 'Flash delay' ('ED01') 'long' -- 'The amount of time, in sixtieths of a second, the editor highlights a matching bracket.' [mutable]
#        property 'Dynamic scroll' ('ED02') 'bool' -- 'Display a window\325s contents as you move the scroll box.' [mutable]
#        property 'Balance' ('ED03') 'bool' -- 'Flash the matching opening bracket when you type a closing bracket.' [mutable]
#        property 'Use Drag & Drop Editing' ('ED04') 'bool' -- 'Use Drag & Drop text editing.' [mutable]
#        property 'Save on update' ('ED05') 'bool' -- 'Save all editor windows automatically when you choose the Update command.' [mutable]
#        property 'Sort Function Popup' ('ED06') 'bool' -- '' [mutable]
#        property 'Use Multiple Undo' ('ED07') 'bool' -- '' [mutable]
#        property 'Remember font' ('ED08') 'bool' -- 'Display a source file with its own font settings.' [mutable]
#        property 'Remember selection' ('ED09') 'bool' -- 'Restore the previous selection in a file when you open it.' [mutable]
#        property 'Remember window' ('ED10') 'bool' -- 'Restore the last size and position for a source file window when you open it.' [mutable]
#        property 'Main Text Color' ('ED12') 'cRGB' -- 'Main, default, color for text.' [mutable]
#        property 'Background Color' ('ED13') 'cRGB' -- 'Color of the background of editor windows.' [mutable]
#        property 'Context Popup Delay' ('ED14') 'bool' -- 'The amount of time, in sixtieths of a second, before the context popup is displayed if you click and hold on a browser symbol.' [mutable]

#    Class 'Syntax Coloring' ('SNTX') -- ''
#        property 'Syntax coloring' ('GH01') 'bool' -- 'Mark keywords and comments with color.' [mutable]
#        property 'Comment color' ('GH02') 'cRGB' -- 'The color for comments.' [mutable]
#        property 'Keyword color' ('GH03') 'cRGB' -- 'The color for language keywords.' [mutable]
#        property 'String color' ('GH04') 'cRGB' -- 'The color for strings.' [mutable]
#        property 'Custom color 1' ('GH05') 'cRGB' -- 'The color for the first set of custom keywords.' [mutable]
#        property 'Custom color 2' ('GH06') 'cRGB' -- 'The color for the second set custom keywords.' [mutable]
#        property 'Custom color 3' ('GH07') 'cRGB' -- 'The color for the third set of custom keywords.' [mutable]
#        property 'Custom color 4' ('GH08') 'cRGB' -- 'The color for the fourth set of custom keywords.' [mutable]

#    Class 'Custom Keywords' ('CUKW') -- ''
#        property 'Custom color 1' ('KW01') 'cRGB' -- 'The color for the first set of custom keywords.' [mutable]
#        property 'Custom color 2' ('KW02') 'cRGB' -- 'The color for the second set custom keywords.' [mutable]
#        property 'Custom color 3' ('KW03') 'cRGB' -- 'The color for the third set of custom keywords.' [mutable]
#        property 'Custom color 4' ('KW04') 'cRGB' -- 'The color for the fourth set of custom keywords.' [mutable]

#    Class 'Browser Coloring' ('BRKW') -- 'Colors for Browser symbols.'
#        property 'Browser Keywords' ('BW00') 'bool' -- 'Mark Browser symbols with color.' [mutable]
#        property 'Classes Color' ('BW01') 'cRGB' -- 'The color for classes.' [mutable]
#        property 'Constants Color' ('BW02') 'cRGB' -- 'The color for constants.' [mutable]
#        property 'Enums Color' ('BW03') 'cRGB' -- 'The color for enums.' [mutable]
#        property 'Functions Color' ('BW04') 'cRGB' -- 'Set color for functions.' [mutable]
#        property 'Globals Color' ('BW05') 'cRGB' -- 'The color for globals' [mutable]
#        property 'Macros Color' ('BW06') 'cRGB' -- 'The color for macros.' [mutable]
#        property 'Templates Color' ('BW07') 'cRGB' -- 'Set color for templates.' [mutable]
#        property 'Typedefs Color' ('BW08') 'cRGB' -- 'The color for typedefs.' [mutable]

#    Class 'Error Information' ('ErrM') -- 'Describes a single error or warning from the compiler or the linker.'
#        property 'kind' ('ErrT') 'ErrT' -- 'The type of error or warning.' [enum]
#        property 'message' ('ErrS') 'TEXT' -- 'The error or warning message.' []
#        property 'disk file' ('file') 'fss ' -- 'The file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors).' []
#        property 'lineNumber' ('ErrL') 'long' -- 'The line in the file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors).' []

#    Class 'Extras' ('GXTR') -- ''
#        property 'Completion sound' ('EX01') 'bool' -- 'Play a sound when finished a Bring Up To Date or Make command.' [mutable]
#        property 'Success sound' ('EX02') 'TEXT' -- 'The sound CodeWarrior plays when it successfully finishes a Bring Up To Date or Make command.' [mutable]
#        property 'Failure sound' ('EX03') 'TEXT' -- 'The sound CodeWarrior plays when it cannot finish a Bring Up To Date or Make command.' [mutable]
#        property 'Full screen zoom' ('EX07') 'bool' -- 'Zoom windows to the full screen width.' [mutable]
#        property 'External reference' ('EX08') 'RefP' -- 'Which on-line function reference to use.' [mutable enum]
#        property 'Use Script Menu' ('EX12') 'bool' -- 'Controls the use of the AppleScript menu' [mutable]
#        property 'Use Editor Extensions' ('EX10') 'bool' -- 'Controls the use of the Editor Extensions menu' [mutable]
#        property 'Use External Editor' ('EX11') 'bool' -- 'Controls whether CodeWarrior uses its own integrated editor or an external application for editing text files.' [mutable]
#        property 'Honor Projector State for Projects' ('EX13') 'bool' -- 'Controls whether CodeWarrior opens files set to read-only by Projector.' [mutable]

#    Class 'Build Extras' ('LXTR') -- ''
#        property 'Browser active' ('EX09') 'bool' -- 'Allow the collection of browser information.' [mutable]
#        property 'Modification date caching' ('EX04') 'bool' -- '' [mutable]
#        property 'Generate Map' ('EX05') 'bool' -- 'Generate a Pascal Make map file.' [mutable]
#        property 'Store analysis results' ('EX06') 'bool' -- '' [mutable]

#    Class 'Font' ('mFNT') -- ''
#        property 'Auto indent' ('FN01') 'bool' -- 'Indent new lines automatically.' [mutable]
#        property 'Tab size' ('FN02') 'shor' -- '' [mutable]
#        property 'Text font' ('ptxf') 'TEXT' -- 'The font used in editing windows.' [mutable]
#        property 'Text size' ('ptps') 'shor' -- 'The size of the text in an editing window.' [mutable]

#    Class 'Function Information' ('FDef') -- 'Describes the location of any function or global data definition within the current project.'
#        property 'disk file' ('file') 'fss ' -- 'The location on disk of the file containing the definition.' []
#        property 'lineNumber' ('ErrL') 'long' -- 'The line number where the definition begins.' []

#    Class 'Path Information' ('PInf') -- 'Contains all of the parameters that describe an access path.'
#        property 'name' ('pnam') 'TEXT' -- 'The actual path name.' [mutable]
#        property 'recursive' ('Recu') 'bool' -- 'Will the path be searched recursively?  (Default is true)' [mutable]
#        property 'origin' ('Orig') 'PPrm' -- '' [mutable enum]

#    Class 'ProjectFile' ('SrcF') -- 'A file contained in a project'
#        property 'filetype' ('SrcT') 'SrcT' -- 'What kind of file is this ?' [enum]
#        property 'name' ('pnam') 'TEXT' -- 'The file\325s name' []
#        property 'disk file' ('file') 'fss ' -- 'The file\325s location on disk' []
#        property 'codesize' ('CSiz') 'long' -- 'The size of this file\325s code.' []
#        property 'datasize' ('DSiz') 'long' -- 'The size of this file\325s data.' []
#        property 'up to date' ('UpTD') 'bool' -- 'Has the file been compiled since its last modification ?' []
#        property 'symbols' ('SymG') 'bool' -- 'Are debugging symbols generated for this file ?' [mutable]
#        property 'weak link' ('Weak') 'bool' -- 'Is this file imported weakly into the project ? [PowerPC only]' [mutable]
#        property 'initialize before' ('Bfor') 'bool' -- 'Intiailize the shared library before the main application.' [mutable]
#        property 'includes' ('IncF') 'fss ' -- '' []

#    Class 'Segment' ('Seg ') -- 'A segment or group in the project'
#        property 'name' ('pnam') 'TEXT' -- '' [mutable]
#        property 'filecount' ('NumF') 'shor' -- '' []
#        property 'preloaded' ('Prel') 'bool' -- 'Is the segment preloaded ? [68K only]' [mutable]
#        property 'protected' ('Prot') 'bool' -- 'Is the segment protected ? [68K only]' [mutable]
#        property 'locked' ('PLck') 'bool' -- 'Is the segment locked ? [68K only]' [mutable]
#        property 'purgeable' ('Purg') 'bool' -- 'Is the segment purgeable ? [68K only]' [mutable]
#        property 'system heap' ('SysH') 'bool' -- 'Is the segment loaded into the system heap ? [68K only]' [mutable]

#    Class 'Target' ('TARG') -- 'Contains the definitions of a project\325s target.'
#        property 'Current Target' ('TA01') 'TEXT' -- 'The name of the current target.' [mutable]
#        property 'Mappings' ('TA08') 'TInf' -- '' [mutable list]

#    Class 'Target Information' ('TInf') -- ''
#        property 'File Type' ('PR04') 'TEXT' -- '' [mutable]
#        property 'Extension' ('TA02') 'TEXT' -- '' [mutable]
#        property 'Precompiled' ('TA03') 'bool' -- '' [mutable]
#        property 'Resource File' ('TA04') 'bool' -- '' [mutable]
#        property 'Launchable' ('TA05') 'bool' -- '' [mutable]
#        property 'Ignored by Make' ('TA06') 'bool' -- '' [mutable]
#        property 'Compiler' ('TA07') 'TEXT' -- '' [mutable]

#    Class 'class' ('Clas') -- 'A class, struct, or record type in the current project'
#        property 'name' ('pnam') 'TEXT' -- '' []
#        property 'language' ('Lang') 'Lang' -- 'Implementation language of this class' [enum]
#        property 'declaration file' ('DcFl') 'fss ' -- 'Source file containing the class declaration' []
#        property 'declaration start offset' ('DcSt') 'long' -- 'Start of class declaration source code' []
#        property 'declaration end offset' ('DcEn') 'long' -- 'End of class declaration' []
#        property 'subclasses' ('SubC') 'Clas' -- 'the immediate subclasses of this class' [list]
#        property 'all subclasses' ('SubA') 'Clas' -- 'the classes directly or indirectly derived from this class' [list]
#        element 'BsCl' as ['indx']
#        element 'MbFn' as ['indx', 'name']
#        element 'DtMb' as ['indx', 'name']

#    Class 'classes' ('Clas') -- ''
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'member function' ('MbFn') -- 'A class member function or method.'
#        property 'name' ('pnam') 'TEXT' -- '' []
#        property 'access' ('Acce') 'Acce' -- '' [enum]
#        property 'virtual' ('Virt') 'bool' -- '' []
#        property 'static' ('Stat') 'bool' -- '' []
#        property 'declaration file' ('DcFl') 'fss ' -- 'Source file containing the member function declaration' []
#        property 'declaration start offset' ('DcSt') 'long' -- 'start of member function declaration source code' []
#        property 'declaration end offset' ('DcEn') 'long' -- 'end of member function declaration' []
#        property 'implementation file' ('DfFl') 'fss ' -- 'Source file containing the member function definition' []
#        property 'implementation start offset' ('DfSt') 'long' -- 'start of member function definition source code' []
#        property 'implementation end offset' ('DfEn') 'long' -- 'end of member function definition' []

#    Class 'member functions' ('MbFn') -- ''
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'data member' ('DtMb') -- 'A class data member or field'
#        property 'name' ('pnam') 'TEXT' -- '' []
#        property 'access' ('Acce') 'Acce' -- '' [enum]
#        property 'static' ('Stat') 'bool' -- '' []
#        property 'declaration start offset' ('DcSt') 'long' -- '' []
#        property 'declaration end offset' ('DcEn') 'long' -- '' []

#    Class 'data members' ('DtMb') -- ''
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'base class' ('BsCl') -- 'A base class or super class of a class'
#        property 'class' ('Clas') 'obj ' -- 'The class object corresponding to this base class' []
#        property 'access' ('Acce') 'Acce' -- '' [enum]
#        property 'virtual' ('Virt') 'bool' -- '' []

#    Class 'base classes' ('BsCl') -- ''
#        property '' ('c@#!') 'type' -- '' [0 enum]

#    Class 'browser catalog' ('Cata') -- 'The browser symbol catalog for the current project'
#        element 'Clas' as ['indx', 'name']

#    Class 'VCS Setup' ('VCSs') -- 'The version control system perferences.'
#        property 'VCS Active' ('VC01') 'bool' -- 'Use Version Control' [mutable]
#        property 'Connection Method' ('VC02') 'TEXT' -- 'Name of Version Control System to use.' [mutable]
#        property 'Username' ('VC03') 'TEXT' -- 'The user name for the VCS.' [mutable]
#        property 'Password' ('VC04') 'TEXT' -- 'The password for the VCS.' [mutable]
#        property 'Auto Connect' ('VC05') 'bool' -- 'Automatically connect to database when starting.' [mutable]
#        property 'Store Password' ('VC06') 'bool' -- 'Store the password.' [mutable]
#        property 'Always Prompt' ('VC07') 'bool' -- 'Always show login dialog' [mutable]
#        property 'Mount Volume' ('VC08') 'bool' -- "Attempt to mount the database volume if it isn't available." [mutable]
#        property 'Database Path' ('VC09') 'PInf' -- 'Path to the VCS database.' [mutable]
#        property 'Local Root' ('VC10') 'PInf' -- 'Path to the local directory to checkout to.' [mutable]
