"""Suite Metrowerks Shell Suite: Events supported by the Metrowerks Project Shell
Level 1, version 1

Generated from Sap:Codewarrior7:Metrowerks CodeWarrior:CodeWarrior IDE 1.3
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
	'unknown' : 'FUnk',	# An unknown file type.
}

_Enum_PPrm = {
	'absolute' : 'Abso',	# An absolute path name, including volume name.
	'project_relative' : 'PRel',	# A path relative to the current projectÕs folder.
	'shell_relative' : 'SRel',	# A path relative to the CodeWarriorª folder.
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Save_Message_Window_As(self, _object, _attributes={}, **_arguments):
		"""Save Message Window As: Saves the message window as a text file
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#    Class 'Access Paths' ('PATH') -- 'Contains the definitions of a project\325s access (search) paths.'
#        property 'User Paths' ('PA01') 'PInf' -- 'To add an access path for the source files.' [mutable list]
#        property 'System Paths' ('PA03') 'PInf' -- 'To add an access path for the include files. (Not supported in Pascal)' [mutable list]
#        property 'Always Full Search' ('PA02') 'bool' -- 'To force the compiler to search for system includes like it searches for user includes.' [mutable]

#    Class 'Document' ('docu') -- 'An open text file'
#        property 'name' ('pnam') 'TEXT' -- 'The document\325s name' []
#        property 'mode' ('Mode') 'Mode' -- 'The document\325s open mode' [enum]
#        property 'disk file' ('file') 'fss ' -- 'The document\325s location on disk' []

#    Class 'Error Information' ('ErrM') -- 'Describes a single error or warning from the compiler or the linker.'
#        property 'kind' ('ErrT') 'ErrT' -- 'The type of error or warning.' [enum]
#        property 'message' ('ErrS') 'TEXT' -- 'The error or warning message.' []
#        property 'disk file' ('file') 'fss ' -- 'The file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors).' []
#        property 'line' ('ErrL') 'long' -- 'The line in the file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors).' []

#    Class 'Function Information' ('FDef') -- 'Describes the location of any function or global data definition within the current project.'
#        property 'disk file' ('file') 'fss ' -- 'The location on disk of the file containing the definition.' []
#        property 'line' ('ErrL') 'long' -- 'The line number where the definition begins.' []

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
