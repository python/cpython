"""Suite Metrowerks Shell Suite: Events supported by the Metrowerks Project Shell
Level 1, version 1

Generated from flap:Metrowerks:Metrowerks CodeWarrior:CodeWarrior IDE 2.0.1
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'MMPR'

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

	def Set_Current_Target(self, _object=None, _attributes={}, **_arguments):
		"""Set Current Target: Set the current target of a project
		Required argument: Name of target
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'STrg'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def Set_Default_Project(self, _object, _attributes={}, **_arguments):
		"""Set Default Project: Set the default project
		Required argument: Name of project
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'MMPR'
		_subcode = 'SDfP'

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


class Access_Paths(aetools.ComponentItem):
	"""Access Paths - Contains the definitions of a project’s access (search) paths."""
	want = 'PATH'
class User_Paths(aetools.NProperty):
	"""User Paths - To add an access path for the source files."""
	which = 'PA01'
	want = 'PInf'
class System_Paths(aetools.NProperty):
	"""System Paths - To add an access path for the include files. (Not supported in Pascal)"""
	which = 'PA03'
	want = 'PInf'
class Always_Full_Search(aetools.NProperty):
	"""Always Full Search - To force the compiler to search for system includes like it searches for user includes."""
	which = 'PA02'
	want = 'bool'

class Editor(aetools.ComponentItem):
	"""Editor - """
	want = 'EDTR'
class Flash_delay(aetools.NProperty):
	"""Flash delay - The amount of time, in sixtieths of a second, the editor highlights a matching bracket."""
	which = 'ED01'
	want = 'long'
class Dynamic_scroll(aetools.NProperty):
	"""Dynamic scroll - Display a window’s contents as you move the scroll box."""
	which = 'ED02'
	want = 'bool'
class Balance(aetools.NProperty):
	"""Balance - Flash the matching opening bracket when you type a closing bracket."""
	which = 'ED03'
	want = 'bool'
class Use_Drag__26__Drop_Editing(aetools.NProperty):
	"""Use Drag & Drop Editing - Use Drag & Drop text editing."""
	which = 'ED04'
	want = 'bool'
class Save_on_update(aetools.NProperty):
	"""Save on update - Save all editor windows automatically when you choose the Update command."""
	which = 'ED05'
	want = 'bool'
class Sort_Function_Popup(aetools.NProperty):
	"""Sort Function Popup - """
	which = 'ED06'
	want = 'bool'
class Use_Multiple_Undo(aetools.NProperty):
	"""Use Multiple Undo - """
	which = 'ED07'
	want = 'bool'
class Remember_font(aetools.NProperty):
	"""Remember font - Display a source file with its own font settings."""
	which = 'ED08'
	want = 'bool'
class Remember_selection(aetools.NProperty):
	"""Remember selection - Restore the previous selection in a file when you open it."""
	which = 'ED09'
	want = 'bool'
class Remember_window(aetools.NProperty):
	"""Remember window - Restore the last size and position for a source file window when you open it."""
	which = 'ED10'
	want = 'bool'
class Main_Text_Color(aetools.NProperty):
	"""Main Text Color - Main, default, color for text."""
	which = 'ED12'
	want = 'cRGB'
class Background_Color(aetools.NProperty):
	"""Background Color - Color of the background of editor windows."""
	which = 'ED13'
	want = 'cRGB'
class Context_Popup_Delay(aetools.NProperty):
	"""Context Popup Delay - The amount of time, in sixtieths of a second, before the context popup is displayed if you click and hold on a browser symbol."""
	which = 'ED14'
	want = 'bool'

class Syntax_Coloring(aetools.ComponentItem):
	"""Syntax Coloring - """
	want = 'SNTX'
class Syntax_coloring(aetools.NProperty):
	"""Syntax coloring - Mark keywords and comments with color."""
	which = 'GH01'
	want = 'bool'
class Comment_color(aetools.NProperty):
	"""Comment color - The color for comments."""
	which = 'GH02'
	want = 'cRGB'
class Keyword_color(aetools.NProperty):
	"""Keyword color - The color for language keywords."""
	which = 'GH03'
	want = 'cRGB'
class String_color(aetools.NProperty):
	"""String color - The color for strings."""
	which = 'GH04'
	want = 'cRGB'
class Custom_color_1(aetools.NProperty):
	"""Custom color 1 - The color for the first set of custom keywords."""
	which = 'GH05'
	want = 'cRGB'
class Custom_color_2(aetools.NProperty):
	"""Custom color 2 - The color for the second set custom keywords."""
	which = 'GH06'
	want = 'cRGB'
class Custom_color_3(aetools.NProperty):
	"""Custom color 3 - The color for the third set of custom keywords."""
	which = 'GH07'
	want = 'cRGB'
class Custom_color_4(aetools.NProperty):
	"""Custom color 4 - The color for the fourth set of custom keywords."""
	which = 'GH08'
	want = 'cRGB'

class Custom_Keywords(aetools.ComponentItem):
	"""Custom Keywords - """
	want = 'CUKW'
class Custom_color_1(aetools.NProperty):
	"""Custom color 1 - The color for the first set of custom keywords."""
	which = 'KW01'
	want = 'cRGB'
class Custom_color_2(aetools.NProperty):
	"""Custom color 2 - The color for the second set custom keywords."""
	which = 'KW02'
	want = 'cRGB'
class Custom_color_3(aetools.NProperty):
	"""Custom color 3 - The color for the third set of custom keywords."""
	which = 'KW03'
	want = 'cRGB'
class Custom_color_4(aetools.NProperty):
	"""Custom color 4 - The color for the fourth set of custom keywords."""
	which = 'KW04'
	want = 'cRGB'

class Browser_Coloring(aetools.ComponentItem):
	"""Browser Coloring - Colors for Browser symbols."""
	want = 'BRKW'
class Browser_Keywords(aetools.NProperty):
	"""Browser Keywords - Mark Browser symbols with color."""
	which = 'BW00'
	want = 'bool'
class Classes_Color(aetools.NProperty):
	"""Classes Color - The color for classes."""
	which = 'BW01'
	want = 'cRGB'
class Constants_Color(aetools.NProperty):
	"""Constants Color - The color for constants."""
	which = 'BW02'
	want = 'cRGB'
class Enums_Color(aetools.NProperty):
	"""Enums Color - The color for enums."""
	which = 'BW03'
	want = 'cRGB'
class Functions_Color(aetools.NProperty):
	"""Functions Color - Set color for functions."""
	which = 'BW04'
	want = 'cRGB'
class Globals_Color(aetools.NProperty):
	"""Globals Color - The color for globals"""
	which = 'BW05'
	want = 'cRGB'
class Macros_Color(aetools.NProperty):
	"""Macros Color - The color for macros."""
	which = 'BW06'
	want = 'cRGB'
class Templates_Color(aetools.NProperty):
	"""Templates Color - Set color for templates."""
	which = 'BW07'
	want = 'cRGB'
class Typedefs_Color(aetools.NProperty):
	"""Typedefs Color - The color for typedefs."""
	which = 'BW08'
	want = 'cRGB'

class Error_Information(aetools.ComponentItem):
	"""Error Information - Describes a single error or warning from the compiler or the linker."""
	want = 'ErrM'
class kind(aetools.NProperty):
	"""kind - The type of error or warning."""
	which = 'ErrT'
	want = 'ErrT'
class message(aetools.NProperty):
	"""message - The error or warning message."""
	which = 'ErrS'
	want = 'TEXT'
class disk_file(aetools.NProperty):
	"""disk file - The file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors)."""
	which = 'file'
	want = 'fss '
class lineNumber(aetools.NProperty):
	"""lineNumber - The line in the file where the error occurred.  May not be returned for certain kinds of errors (eg, link errors)."""
	which = 'ErrL'
	want = 'long'

class Extras(aetools.ComponentItem):
	"""Extras - """
	want = 'GXTR'
class Completion_sound(aetools.NProperty):
	"""Completion sound - Play a sound when finished a Bring Up To Date or Make command."""
	which = 'EX01'
	want = 'bool'
class Success_sound(aetools.NProperty):
	"""Success sound - The sound CodeWarrior plays when it successfully finishes a Bring Up To Date or Make command."""
	which = 'EX02'
	want = 'TEXT'
class Failure_sound(aetools.NProperty):
	"""Failure sound - The sound CodeWarrior plays when it cannot finish a Bring Up To Date or Make command."""
	which = 'EX03'
	want = 'TEXT'
class Full_screen_zoom(aetools.NProperty):
	"""Full screen zoom - Zoom windows to the full screen width."""
	which = 'EX07'
	want = 'bool'
class External_reference(aetools.NProperty):
	"""External reference - Which on-line function reference to use."""
	which = 'EX08'
	want = 'RefP'
class Use_Script_Menu(aetools.NProperty):
	"""Use Script Menu - Controls the use of the AppleScript menu"""
	which = 'EX12'
	want = 'bool'
class Use_Editor_Extensions(aetools.NProperty):
	"""Use Editor Extensions - Controls the use of the Editor Extensions menu"""
	which = 'EX10'
	want = 'bool'
class Use_External_Editor(aetools.NProperty):
	"""Use External Editor - Controls whether CodeWarrior uses its own integrated editor or an external application for editing text files."""
	which = 'EX11'
	want = 'bool'
class Honor_Projector_State_for_Projects(aetools.NProperty):
	"""Honor Projector State for Projects - Controls whether CodeWarrior opens files set to read-only by Projector."""
	which = 'EX13'
	want = 'bool'

class Build_Extras(aetools.ComponentItem):
	"""Build Extras - """
	want = 'LXTR'
class Browser_active(aetools.NProperty):
	"""Browser active - Allow the collection of browser information."""
	which = 'EX09'
	want = 'bool'
class Modification_date_caching(aetools.NProperty):
	"""Modification date caching - """
	which = 'EX04'
	want = 'bool'
class Multiprocessing_Compilation(aetools.NProperty):
	"""Multiprocessing Compilation - """
	which = 'EX14'
	want = 'bool'
class Show_ToolServer_Menu(aetools.NProperty):
	"""Show ToolServer Menu - """
	which = 'EX18'
	want = 'bool'
class Enable_Automatic_Toolbar_Help(aetools.NProperty):
	"""Enable Automatic Toolbar Help - """
	which = 'EX19'
	want = 'bool'
class Include_File_Cache_Size__28_K_29_(aetools.NProperty):
	"""Include File Cache Size (K) - """
	which = 'EX15'
	want = 'long'
class Recent_Documents(aetools.NProperty):
	"""Recent Documents - """
	which = 'EX16'
	want = 'shor'
class Recent_Projects(aetools.NProperty):
	"""Recent Projects - """
	which = 'EX17'
	want = 'shor'

class File_Mappings(aetools.ComponentItem):
	"""File Mappings - Mappings of extensions & file types to compilers"""
	want = 'FLMP'
class mappings(aetools.NProperty):
	"""mappings - """
	which = 'FMps'
	want = 'FMap'

class Font(aetools.ComponentItem):
	"""Font - """
	want = 'mFNT'
class Auto_indent(aetools.NProperty):
	"""Auto indent - Indent new lines automatically."""
	which = 'FN01'
	want = 'bool'
class Tab_size(aetools.NProperty):
	"""Tab size - """
	which = 'FN02'
	want = 'shor'
class Text_font(aetools.NProperty):
	"""Text font - The font used in editing windows."""
	which = 'ptxf'
	want = 'TEXT'
class Text_size(aetools.NProperty):
	"""Text size - The size of the text in an editing window."""
	which = 'ptps'
	want = 'shor'

class Function_Information(aetools.ComponentItem):
	"""Function Information - Describes the location of any function or global data definition within the current project."""
	want = 'FDef'
# repeated property disk_file The location on disk of the file containing the definition.
# repeated property lineNumber The line number where the definition begins.

class Path_Information(aetools.ComponentItem):
	"""Path Information - Contains all of the parameters that describe an access path."""
	want = 'PInf'
class name(aetools.NProperty):
	"""name - The actual path name."""
	which = 'pnam'
	want = 'TEXT'
class recursive(aetools.NProperty):
	"""recursive - Will the path be searched recursively?  (Default is true)"""
	which = 'Recu'
	want = 'bool'
class origin(aetools.NProperty):
	"""origin - """
	which = 'Orig'
	want = 'PPrm'

class ProjectFile(aetools.ComponentItem):
	"""ProjectFile - A file contained in a project"""
	want = 'SrcF'
class filetype(aetools.NProperty):
	"""filetype - What kind of file is this ?"""
	which = 'SrcT'
	want = 'SrcT'
# repeated property name The file’s name
# repeated property disk_file The file’s location on disk
class codesize(aetools.NProperty):
	"""codesize - The size of this file’s code."""
	which = 'CSiz'
	want = 'long'
class datasize(aetools.NProperty):
	"""datasize - The size of this file’s data."""
	which = 'DSiz'
	want = 'long'
class up_to_date(aetools.NProperty):
	"""up to date - Has the file been compiled since its last modification ?"""
	which = 'UpTD'
	want = 'bool'
class symbols(aetools.NProperty):
	"""symbols - Are debugging symbols generated for this file ?"""
	which = 'SymG'
	want = 'bool'
class weak_link(aetools.NProperty):
	"""weak link - Is this file imported weakly into the project ? [PowerPC only]"""
	which = 'Weak'
	want = 'bool'
class initialize_before(aetools.NProperty):
	"""initialize before - Intiailize the shared library before the main application."""
	which = 'Bfor'
	want = 'bool'
class includes(aetools.NProperty):
	"""includes - """
	which = 'IncF'
	want = 'fss '

class Segment(aetools.ComponentItem):
	"""Segment - A segment or group in the project"""
	want = 'Seg '
# repeated property name 
class filecount(aetools.NProperty):
	"""filecount - """
	which = 'NumF'
	want = 'shor'
class preloaded(aetools.NProperty):
	"""preloaded - Is the segment preloaded ? [68K only]"""
	which = 'Prel'
	want = 'bool'
class protected(aetools.NProperty):
	"""protected - Is the segment protected ? [68K only]"""
	which = 'Prot'
	want = 'bool'
class locked(aetools.NProperty):
	"""locked - Is the segment locked ? [68K only]"""
	which = 'PLck'
	want = 'bool'
class purgeable(aetools.NProperty):
	"""purgeable - Is the segment purgeable ? [68K only]"""
	which = 'Purg'
	want = 'bool'
class system_heap(aetools.NProperty):
	"""system heap - Is the segment loaded into the system heap ? [68K only]"""
	which = 'SysH'
	want = 'bool'

class Target_Settings(aetools.ComponentItem):
	"""Target Settings - Contains the definitions of a project’s target."""
	want = 'TARG'
class Linker(aetools.NProperty):
	"""Linker - The name of the current linker."""
	which = 'TA01'
	want = 'TEXT'
class Post_Linker(aetools.NProperty):
	"""Post Linker - """
	which = 'TA09'
	want = 'TEXT'
class Target_Name(aetools.NProperty):
	"""Target Name - """
	which = 'TA10'
	want = 'TEXT'
class Output_Directory_Path(aetools.NProperty):
	"""Output Directory Path - """
	which = 'TA11'
	want = 'TEXT'
class Output_Directory_Origin(aetools.NProperty):
	"""Output Directory Origin - """
	which = 'TA12'
	want = 'PPrm'

class File_Mapping(aetools.ComponentItem):
	"""File Mapping - """
	want = 'FMap'
class File_Type(aetools.NProperty):
	"""File Type - """
	which = 'PR04'
	want = 'TEXT'
class Extension(aetools.NProperty):
	"""Extension - """
	which = 'TA02'
	want = 'TEXT'
class Precompiled(aetools.NProperty):
	"""Precompiled - """
	which = 'TA03'
	want = 'bool'
class Resource_File(aetools.NProperty):
	"""Resource File - """
	which = 'TA04'
	want = 'bool'
class Launchable(aetools.NProperty):
	"""Launchable - """
	which = 'TA05'
	want = 'bool'
class Ignored_by_Make(aetools.NProperty):
	"""Ignored by Make - """
	which = 'TA06'
	want = 'bool'
class Compiler(aetools.NProperty):
	"""Compiler - """
	which = 'TA07'
	want = 'TEXT'

class _class(aetools.ComponentItem):
	"""class - A class, struct, or record type in the current project"""
	want = 'Clas'
# repeated property name 
class language(aetools.NProperty):
	"""language - Implementation language of this class"""
	which = 'Lang'
	want = 'Lang'
class declaration_file(aetools.NProperty):
	"""declaration file - Source file containing the class declaration"""
	which = 'DcFl'
	want = 'fss '
class declaration_start_offset(aetools.NProperty):
	"""declaration start offset - Start of class declaration source code"""
	which = 'DcSt'
	want = 'long'
class declaration_end_offset(aetools.NProperty):
	"""declaration end offset - End of class declaration"""
	which = 'DcEn'
	want = 'long'
class subclasses(aetools.NProperty):
	"""subclasses - the immediate subclasses of this class"""
	which = 'SubC'
	want = 'Clas'
class all_subclasses(aetools.NProperty):
	"""all subclasses - the classes directly or indirectly derived from this class"""
	which = 'SubA'
	want = 'Clas'
#        element 'BsCl' as ['indx']
#        element 'MbFn' as ['indx', 'name']
#        element 'DtMb' as ['indx', 'name']

classes = _class

class member_function(aetools.ComponentItem):
	"""member function - A class member function or method."""
	want = 'MbFn'
# repeated property name 
class access(aetools.NProperty):
	"""access - """
	which = 'Acce'
	want = 'Acce'
class virtual(aetools.NProperty):
	"""virtual - """
	which = 'Virt'
	want = 'bool'
class static(aetools.NProperty):
	"""static - """
	which = 'Stat'
	want = 'bool'
# repeated property declaration_file Source file containing the member function declaration
# repeated property declaration_start_offset start of member function declaration source code
# repeated property declaration_end_offset end of member function declaration
class implementation_file(aetools.NProperty):
	"""implementation file - Source file containing the member function definition"""
	which = 'DfFl'
	want = 'fss '
class implementation_start_offset(aetools.NProperty):
	"""implementation start offset - start of member function definition source code"""
	which = 'DfSt'
	want = 'long'
class implementation_end_offset(aetools.NProperty):
	"""implementation end offset - end of member function definition"""
	which = 'DfEn'
	want = 'long'

member_functions = member_function

class data_member(aetools.ComponentItem):
	"""data member - A class data member or field"""
	want = 'DtMb'
# repeated property name 
# repeated property access 
# repeated property static 
# repeated property declaration_start_offset 
# repeated property declaration_end_offset 

data_members = data_member

class base_class(aetools.ComponentItem):
	"""base class - A base class or super class of a class"""
	want = 'BsCl'
class _class(aetools.NProperty):
	"""class - The class object corresponding to this base class"""
	which = 'Clas'
	want = 'obj '
# repeated property access 
# repeated property virtual 

base_classes = base_class

class browser_catalog(aetools.ComponentItem):
	"""browser catalog - The browser symbol catalog for the current project"""
	want = 'Cata'
#        element 'Clas' as ['indx', 'name']

class VCS_Setup(aetools.ComponentItem):
	"""VCS Setup - The version control system perferences."""
	want = 'VCSs'
class VCS_Active(aetools.NProperty):
	"""VCS Active - Use Version Control"""
	which = 'VC01'
	want = 'bool'
class Connection_Method(aetools.NProperty):
	"""Connection Method - Name of Version Control System to use."""
	which = 'VC02'
	want = 'TEXT'
class Username(aetools.NProperty):
	"""Username - The user name for the VCS."""
	which = 'VC03'
	want = 'TEXT'
class Password(aetools.NProperty):
	"""Password - The password for the VCS."""
	which = 'VC04'
	want = 'TEXT'
class Auto_Connect(aetools.NProperty):
	"""Auto Connect - Automatically connect to database when starting."""
	which = 'VC05'
	want = 'bool'
class Store_Password(aetools.NProperty):
	"""Store Password - Store the password."""
	which = 'VC06'
	want = 'bool'
class Always_Prompt(aetools.NProperty):
	"""Always Prompt - Always show login dialog"""
	which = 'VC07'
	want = 'bool'
class Mount_Volume(aetools.NProperty):
	"""Mount Volume - Attempt to mount the database volume if it isn't available."""
	which = 'VC08'
	want = 'bool'
class Database_Path(aetools.NProperty):
	"""Database Path - Path to the VCS database."""
	which = 'VC09'
	want = 'PInf'
class Local_Root(aetools.NProperty):
	"""Local Root - Path to the local directory to checkout to."""
	which = 'VC10'
	want = 'PInf'
Access_Paths._propdict = {
	'User_Paths' : User_Paths,
	'System_Paths' : System_Paths,
	'Always_Full_Search' : Always_Full_Search,
}
Access_Paths._elemdict = {
}
Editor._propdict = {
	'Flash_delay' : Flash_delay,
	'Dynamic_scroll' : Dynamic_scroll,
	'Balance' : Balance,
	'Use_Drag__26__Drop_Editing' : Use_Drag__26__Drop_Editing,
	'Save_on_update' : Save_on_update,
	'Sort_Function_Popup' : Sort_Function_Popup,
	'Use_Multiple_Undo' : Use_Multiple_Undo,
	'Remember_font' : Remember_font,
	'Remember_selection' : Remember_selection,
	'Remember_window' : Remember_window,
	'Main_Text_Color' : Main_Text_Color,
	'Background_Color' : Background_Color,
	'Context_Popup_Delay' : Context_Popup_Delay,
}
Editor._elemdict = {
}
Syntax_Coloring._propdict = {
	'Syntax_coloring' : Syntax_coloring,
	'Comment_color' : Comment_color,
	'Keyword_color' : Keyword_color,
	'String_color' : String_color,
	'Custom_color_1' : Custom_color_1,
	'Custom_color_2' : Custom_color_2,
	'Custom_color_3' : Custom_color_3,
	'Custom_color_4' : Custom_color_4,
}
Syntax_Coloring._elemdict = {
}
Custom_Keywords._propdict = {
	'Custom_color_1' : Custom_color_1,
	'Custom_color_2' : Custom_color_2,
	'Custom_color_3' : Custom_color_3,
	'Custom_color_4' : Custom_color_4,
}
Custom_Keywords._elemdict = {
}
Browser_Coloring._propdict = {
	'Browser_Keywords' : Browser_Keywords,
	'Classes_Color' : Classes_Color,
	'Constants_Color' : Constants_Color,
	'Enums_Color' : Enums_Color,
	'Functions_Color' : Functions_Color,
	'Globals_Color' : Globals_Color,
	'Macros_Color' : Macros_Color,
	'Templates_Color' : Templates_Color,
	'Typedefs_Color' : Typedefs_Color,
}
Browser_Coloring._elemdict = {
}
Error_Information._propdict = {
	'kind' : kind,
	'message' : message,
	'disk_file' : disk_file,
	'lineNumber' : lineNumber,
}
Error_Information._elemdict = {
}
Extras._propdict = {
	'Completion_sound' : Completion_sound,
	'Success_sound' : Success_sound,
	'Failure_sound' : Failure_sound,
	'Full_screen_zoom' : Full_screen_zoom,
	'External_reference' : External_reference,
	'Use_Script_Menu' : Use_Script_Menu,
	'Use_Editor_Extensions' : Use_Editor_Extensions,
	'Use_External_Editor' : Use_External_Editor,
	'Honor_Projector_State_for_Projects' : Honor_Projector_State_for_Projects,
}
Extras._elemdict = {
}
Build_Extras._propdict = {
	'Browser_active' : Browser_active,
	'Modification_date_caching' : Modification_date_caching,
	'Multiprocessing_Compilation' : Multiprocessing_Compilation,
	'Show_ToolServer_Menu' : Show_ToolServer_Menu,
	'Enable_Automatic_Toolbar_Help' : Enable_Automatic_Toolbar_Help,
	'Include_File_Cache_Size__28_K_29_' : Include_File_Cache_Size__28_K_29_,
	'Recent_Documents' : Recent_Documents,
	'Recent_Projects' : Recent_Projects,
}
Build_Extras._elemdict = {
}
File_Mappings._propdict = {
	'mappings' : mappings,
}
File_Mappings._elemdict = {
}
Font._propdict = {
	'Auto_indent' : Auto_indent,
	'Tab_size' : Tab_size,
	'Text_font' : Text_font,
	'Text_size' : Text_size,
}
Font._elemdict = {
}
Function_Information._propdict = {
	'disk_file' : disk_file,
	'lineNumber' : lineNumber,
}
Function_Information._elemdict = {
}
Path_Information._propdict = {
	'name' : name,
	'recursive' : recursive,
	'origin' : origin,
}
Path_Information._elemdict = {
}
ProjectFile._propdict = {
	'filetype' : filetype,
	'name' : name,
	'disk_file' : disk_file,
	'codesize' : codesize,
	'datasize' : datasize,
	'up_to_date' : up_to_date,
	'symbols' : symbols,
	'weak_link' : weak_link,
	'initialize_before' : initialize_before,
	'includes' : includes,
}
ProjectFile._elemdict = {
}
Segment._propdict = {
	'name' : name,
	'filecount' : filecount,
	'preloaded' : preloaded,
	'protected' : protected,
	'locked' : locked,
	'purgeable' : purgeable,
	'system_heap' : system_heap,
}
Segment._elemdict = {
}
Target_Settings._propdict = {
	'Linker' : Linker,
	'Post_Linker' : Post_Linker,
	'Target_Name' : Target_Name,
	'Output_Directory_Path' : Output_Directory_Path,
	'Output_Directory_Origin' : Output_Directory_Origin,
}
Target_Settings._elemdict = {
}
File_Mapping._propdict = {
	'File_Type' : File_Type,
	'Extension' : Extension,
	'Precompiled' : Precompiled,
	'Resource_File' : Resource_File,
	'Launchable' : Launchable,
	'Ignored_by_Make' : Ignored_by_Make,
	'Compiler' : Compiler,
}
File_Mapping._elemdict = {
}
_class._propdict = {
	'name' : name,
	'language' : language,
	'declaration_file' : declaration_file,
	'declaration_start_offset' : declaration_start_offset,
	'declaration_end_offset' : declaration_end_offset,
	'subclasses' : subclasses,
	'all_subclasses' : all_subclasses,
}
_class._elemdict = {
	'base_class' : base_class,
	'member_function' : member_function,
	'data_member' : data_member,
}
member_function._propdict = {
	'name' : name,
	'access' : access,
	'virtual' : virtual,
	'static' : static,
	'declaration_file' : declaration_file,
	'declaration_start_offset' : declaration_start_offset,
	'declaration_end_offset' : declaration_end_offset,
	'implementation_file' : implementation_file,
	'implementation_start_offset' : implementation_start_offset,
	'implementation_end_offset' : implementation_end_offset,
}
member_function._elemdict = {
}
data_member._propdict = {
	'name' : name,
	'access' : access,
	'static' : static,
	'declaration_start_offset' : declaration_start_offset,
	'declaration_end_offset' : declaration_end_offset,
}
data_member._elemdict = {
}
base_class._propdict = {
	'_class' : _class,
	'access' : access,
	'virtual' : virtual,
}
base_class._elemdict = {
}
browser_catalog._propdict = {
}
browser_catalog._elemdict = {
	'_class' : _class,
}
VCS_Setup._propdict = {
	'VCS_Active' : VCS_Active,
	'Connection_Method' : Connection_Method,
	'Username' : Username,
	'Password' : Password,
	'Auto_Connect' : Auto_Connect,
	'Store_Password' : Store_Password,
	'Always_Prompt' : Always_Prompt,
	'Mount_Volume' : Mount_Volume,
	'Database_Path' : Database_Path,
	'Local_Root' : Local_Root,
}
VCS_Setup._elemdict = {
}
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
	'project_relative' : 'PRel',	# A path relative to the current project’s folder.
	'shell_relative' : 'SRel',	# A path relative to the CodeWarrioré folder.
	'system_relative' : 'YRel',	# A path relative to the system folder
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

_Enum_Inte = {
	'never_interact' : 'eNvr',	# Never allow user interactions
	'interact_with_self' : 'eInS',	# Allow user interaction only when an AppleEvent is sent from within CodeWarrior
	'interact_with_local' : 'eInL',	# Allow user interaction when AppleEvents are sent from applications on the same machine (default)
	'interact_with_all' : 'eInA',	# Allow user interaction from both local and remote AppleEvents
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'LXTR' : Build_Extras,
	'Seg ' : Segment,
	'BsCl' : base_class,
	'PATH' : Access_Paths,
	'mFNT' : Font,
	'BRKW' : Browser_Coloring,
	'FLMP' : File_Mappings,
	'Cata' : browser_catalog,
	'SNTX' : Syntax_Coloring,
	'CUKW' : Custom_Keywords,
	'PInf' : Path_Information,
	'TARG' : Target_Settings,
	'FDef' : Function_Information,
	'EDTR' : Editor,
	'VCSs' : VCS_Setup,
	'ErrM' : Error_Information,
	'Clas' : _class,
	'SrcF' : ProjectFile,
	'GXTR' : Extras,
	'MbFn' : member_function,
	'DtMb' : data_member,
	'FMap' : File_Mapping,
}

_propdeclarations = {
	'CSiz' : codesize,
	'DcEn' : declaration_end_offset,
	'FMps' : mappings,
	'VC02' : Connection_Method,
	'VC03' : Username,
	'TA02' : Extension,
	'UpTD' : up_to_date,
	'VC07' : Always_Prompt,
	'Virt' : virtual,
	'VC05' : Auto_Connect,
	'VC08' : Mount_Volume,
	'VC09' : Database_Path,
	'VC06' : Store_Password,
	'PA02' : Always_Full_Search,
	'PA03' : System_Paths,
	'Clas' : _class,
	'PA01' : User_Paths,
	'TA09' : Post_Linker,
	'Lang' : language,
	'VC10' : Local_Root,
	'EX09' : Browser_active,
	'GH08' : Custom_color_4,
	'DfFl' : implementation_file,
	'GH06' : Custom_color_2,
	'GH07' : Custom_color_3,
	'GH04' : String_color,
	'GH05' : Custom_color_1,
	'KW01' : Custom_color_1,
	'GH03' : Keyword_color,
	'KW03' : Custom_color_3,
	'KW02' : Custom_color_2,
	'BW02' : Constants_Color,
	'BW03' : Enums_Color,
	'BW00' : Browser_Keywords,
	'BW01' : Classes_Color,
	'BW06' : Macros_Color,
	'BW07' : Templates_Color,
	'BW04' : Functions_Color,
	'pnam' : name,
	'DfSt' : implementation_start_offset,
	'BW08' : Typedefs_Color,
	'PR04' : File_Type,
	'EX04' : Modification_date_caching,
	'EX07' : Full_screen_zoom,
	'PLck' : locked,
	'EX02' : Success_sound,
	'EX03' : Failure_sound,
	'TA06' : Ignored_by_Make,
	'TA07' : Compiler,
	'TA04' : Resource_File,
	'TA05' : Launchable,
	'EX08' : External_reference,
	'DSiz' : datasize,
	'TA01' : Linker,
	'VC04' : Password,
	'Bfor' : initialize_before,
	'SrcT' : filetype,
	'SysH' : system_heap,
	'GH01' : Syntax_coloring,
	'GH02' : Comment_color,
	'Prel' : preloaded,
	'Orig' : origin,
	'EX17' : Recent_Projects,
	'EX16' : Recent_Documents,
	'EX15' : Include_File_Cache_Size__28_K_29_,
	'EX14' : Multiprocessing_Compilation,
	'EX01' : Completion_sound,
	'EX12' : Use_Script_Menu,
	'EX11' : Use_External_Editor,
	'EX10' : Use_Editor_Extensions,
	'TA11' : Output_Directory_Path,
	'TA10' : Target_Name,
	'TA12' : Output_Directory_Origin,
	'EX19' : Enable_Automatic_Toolbar_Help,
	'EX18' : Show_ToolServer_Menu,
	'ErrT' : kind,
	'ptxf' : Text_font,
	'ErrS' : message,
	'SubA' : all_subclasses,
	'SubC' : subclasses,
	'ED08' : Remember_font,
	'ED09' : Remember_selection,
	'VC01' : VCS_Active,
	'ErrL' : lineNumber,
	'ED01' : Flash_delay,
	'ED02' : Dynamic_scroll,
	'ED03' : Balance,
	'ED04' : Use_Drag__26__Drop_Editing,
	'ED05' : Save_on_update,
	'ED06' : Sort_Function_Popup,
	'ED07' : Use_Multiple_Undo,
	'Recu' : recursive,
	'IncF' : includes,
	'file' : disk_file,
	'TA03' : Precompiled,
	'Weak' : weak_link,
	'DcSt' : declaration_start_offset,
	'ptps' : Text_size,
	'Stat' : static,
	'ED10' : Remember_window,
	'EX13' : Honor_Projector_State_for_Projects,
	'DfEn' : implementation_end_offset,
	'BW05' : Globals_Color,
	'FN01' : Auto_indent,
	'Purg' : purgeable,
	'NumF' : filecount,
	'Acce' : access,
	'Prot' : protected,
	'DcFl' : declaration_file,
	'KW04' : Custom_color_4,
	'ED13' : Background_Color,
	'ED12' : Main_Text_Color,
	'SymG' : symbols,
	'FN02' : Tab_size,
	'ED14' : Context_Popup_Delay,
}

_compdeclarations = {
}

_enumdeclarations = {
	'ErrT' : _Enum_ErrT,
	'PPrm' : _Enum_PPrm,
	'RefP' : _Enum_RefP,
	'Acce' : _Enum_Acce,
	'SrcT' : _Enum_SrcT,
	'Mode' : _Enum_Mode,
	'Inte' : _Enum_Inte,
	'savo' : _Enum_savo,
	'Lang' : _Enum_Lang,
}
