"""Suite Web Browser Suite: Class of events which are sent to Web Browser applications
Level 1, version 1

Generated from Macintosh HD:Internet:Internet-programma's:Internet Explorer 4.5-map:Internet Explorer 4.5
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'WWW!'

class Web_Browser_Suite_Events:

	_argmap_OpenURL = {
		'to' : 'INTO',
		'toWindow' : 'WIND',
		'Flags' : 'FLGS',
		'FormData' : 'POST',
		'MIMEType' : 'MIME',
		'ProgressApp' : 'PROG',
		'ResultApp' : 'RSLT',
	}

	def OpenURL(self, _object, _attributes={}, **_arguments):
		"""OpenURL: Retrieves URL off the Web.
		Required argument: Fully specified URL
		Keyword argument to: File to save downloaded data into.
		Keyword argument toWindow: Window to open this URL into. (Use -1 for top window, 0 for new window)
		Keyword argument Flags: Valid Flags settings are: 1-Ignore the document cache; 2-Ignore the image cache; 4-Operate in background mode.
		Keyword argument FormData: Posting of forms of a given MIMEType.
		Keyword argument MIMEType: MIME type for the FormData.  
		Keyword argument ProgressApp: If specified, ProgressApp can be named to handle the user interface for process messages. 
		Keyword argument ResultApp: When the requested URL has been accessed and all associated documents loaded, the Web browser will issue an OpenURLResult to the ResultApp.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: TransactionID
		"""
		_code = 'WWW!'
		_subcode = 'OURL'

		aetools.keysubst(_arguments, self._argmap_OpenURL)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_ShowFile = {
		'MIME_type' : 'MIME',
		'Window_ID' : 'WIND',
		'URL' : 'URL ',
		'ResultApp' : 'RSLT',
	}

	def ShowFile(self, _object, _attributes={}, **_arguments):
		"""ShowFile: Passes FileSpec containing data of a given MIME type to be rendered in a given WindowID.
		Required argument: The file to show.
		Keyword argument MIME_type: MIME type
		Keyword argument Window_ID: ID of the window to open the file into. (Can use -1 for top window)
		Keyword argument URL: A URL which allows this document to be reloaded if necessary.
		Keyword argument ResultApp: When the requested URL has been accessed and all associated documents loaded, the Web browser will issue a ShowFileResult to the ResultApp.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: TransactionID
		"""
		_code = 'WWW!'
		_subcode = 'SHWF'

		aetools.keysubst(_arguments, self._argmap_ShowFile)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def CancelTransaction(self, _object, _attributes={}, **_arguments):
		"""CancelTransaction: Tells the Web browser to cancel a TransactionID is progress which the application has initiated via an OpenURL or ShowFile command.
		Required argument: TransactionID
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'CANT'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_QueryVersion = {
		'Major_Version' : 'MAJV',
		'Minor_Version' : 'MINV',
	}

	def QueryVersion(self, _no_object=None, _attributes={}, **_arguments):
		"""QueryVersion: Tells the Web browser that an application which wishes to communicate with it supports a specific version (major.minor) of this SDI specification
		Keyword argument Major_Version: Major version of the SDI specification the sending application supports. 
		Keyword argument Minor_Version: Minor version of the SDI specification the sending application supports. 
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns:      
		"""
		_code = 'WWW!'
		_subcode = 'QVER'

		aetools.keysubst(_arguments, self._argmap_QueryVersion)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def CloseAllWindows(self, _no_object=None, _attributes={}, **_arguments):
		"""CloseAllWindows: Tells the Web browser to close all windows
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Success
		"""
		_code = 'WWW!'
		_subcode = 'CLSA'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_CloseWindow = {
		'ID' : 'WIND',
		'Title' : 'TITL',
	}

	def CloseWindow(self, _no_object=None, _attributes={}, **_arguments):
		"""CloseWindow: Tells the Web browser to close the window specified either by Window ID or Title. If no parameters are specified, the top window will be closed.
		Keyword argument ID: ID of the window to close. (Can use -1 for top window)
		Keyword argument Title: Title of the window to close.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Success
		"""
		_code = 'WWW!'
		_subcode = 'CLOS'

		aetools.keysubst(_arguments, self._argmap_CloseWindow)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Activate = {
		'Flags' : 'FLGS',
	}

	def Activate(self, _object=None, _attributes={}, **_arguments):
		"""Activate: Tells the Web browser to bring itself to the front and show WindowID. (Can use -1 for top window)
		Required argument: WindowID
		Keyword argument Flags: Reserved for future use 
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: WindowID of the front window
		"""
		_code = 'WWW!'
		_subcode = 'ACTV'

		aetools.keysubst(_arguments, self._argmap_Activate)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def ListWindows(self, _no_object=None, _attributes={}, **_arguments):
		"""ListWindows: Return a list of WindowIDs representing each windows currently being used by the Web browser.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'list'
		"""
		_code = 'WWW!'
		_subcode = 'LSTW'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def GetWindowInfo(self, _object, _attributes={}, **_arguments):
		"""GetWindowInfo: Returns a window info record (URL/Title) for the specified window. 
		Required argument: WindowID of the window to get info about
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns:   
		"""
		_code = 'WWW!'
		_subcode = 'WNFO'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_ParseAnchor = {
		'withURL' : 'RELA',
	}

	def ParseAnchor(self, _object, _attributes={}, **_arguments):
		"""ParseAnchor: Combine a base URL and a relative URL to produce a fully-specified URL
		Required argument: MainURL.The base URL.
		Keyword argument withURL: RelativeURL, which, when combined with the MainURL (in the direct object), is used to produce a fully-specified URL.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The Fully specified URL
		"""
		_code = 'WWW!'
		_subcode = 'PRSA'

		aetools.keysubst(_arguments, self._argmap_ParseAnchor)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_BeginProgress = {
		'with_Message' : 'PMSG',
	}

	def BeginProgress(self, _object, _attributes={}, **_arguments):
		"""BeginProgress: Initialize a progress indicator.
		Required argument: TransactionID
		Keyword argument with_Message: Message to display with the progress indicator.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Success 
		"""
		_code = 'WWW!'
		_subcode = 'PRBG'

		aetools.keysubst(_arguments, self._argmap_BeginProgress)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_SetProgressRange = {
		'Max' : 'MAXV',
	}

	def SetProgressRange(self, _object, _attributes={}, **_arguments):
		"""SetProgressRange: Sets a max value for the progress indicator associated with TransactionID
		Required argument: TransactionID
		Keyword argument Max: Max value for this progress indicator
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'PRSR'

		aetools.keysubst(_arguments, self._argmap_SetProgressRange)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_MakingProgress = {
		'with_message' : 'PMSG',
		'current_setting' : 'CURR',
	}

	def MakingProgress(self, _object, _attributes={}, **_arguments):
		"""MakingProgress: Updates the progress indicator associated with TransactionID
		Required argument: TransactionID
		Keyword argument with_message: Message to display in the progress indicator
		Keyword argument current_setting: Current value of the progress indicator
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Cancel
		"""
		_code = 'WWW!'
		_subcode = 'PRMK'

		aetools.keysubst(_arguments, self._argmap_MakingProgress)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def EndProgress(self, _object, _attributes={}, **_arguments):
		"""EndProgress: Nortifies the application that the progress indicator associated with TransactionID is no longer needed.
		Required argument: TransactionID
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'PREN'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def RegisterDone(self, _object, _attributes={}, **_arguments):
		"""RegisterDone: Signals that all processing initiated by the RegisteNow event associated by TransactionID has finished.
		Required argument: TransactionID
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: 0 = failure; 1 = success; 2 = sending application needs more time to complete operation.
		"""
		_code = 'WWW!'
		_subcode = 'RGDN'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_RegisterProtocol = {
		'_for' : 'PROT',
	}

	def RegisterProtocol(self, _object, _attributes={}, **_arguments):
		"""RegisterProtocol: Notifies that the sending application is able to handle all URLs for the specified protocol.
		Required argument: application
		Keyword argument _for: Protocol, such as NEWS, MAILTO, etc... 
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Success
		"""
		_code = 'WWW!'
		_subcode = 'RGPR'

		aetools.keysubst(_arguments, self._argmap_RegisterProtocol)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_UnRegisterProtocol = {
		'_for' : 'PROT',
	}

	def UnRegisterProtocol(self, _object, _attributes={}, **_arguments):
		"""UnRegisterProtocol: Notifies that the sending application is no longer wishes to handle URLs for the specified protocol.
		Required argument: application
		Keyword argument _for: Protocol, such as NEWS, MAILTO, etc... 
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'UNRP'

		aetools.keysubst(_arguments, self._argmap_UnRegisterProtocol)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_RegisterViewer = {
		'_for' : 'MIME',
		'as' : 'FTYP',
		'Flags' : 'MTHD',
	}

	def RegisterViewer(self, _object, _attributes={}, **_arguments):
		"""RegisterViewer: Notifies that the sending application is able to handle all documents for the specified MIMEType.
		Required argument: application
		Keyword argument _for: MIMEType
		Keyword argument as: File type for saved documents
		Keyword argument Flags: undocumented, typecode 'shor'
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'RGVW'

		aetools.keysubst(_arguments, self._argmap_RegisterViewer)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_UnRegisterViewer = {
		'_for' : 'MIME',
	}

	def UnRegisterViewer(self, _object, _attributes={}, **_arguments):
		"""UnRegisterViewer: Notifies that the sending application is no longer wishes to handle documents of the specified MIMEType.
		Required argument: application
		Keyword argument _for: MIMEType
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'UNVW'

		aetools.keysubst(_arguments, self._argmap_UnRegisterViewer)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def RegisterURLEcho(self, _object, _attributes={}, **_arguments):
		"""RegisterURLEcho: Notifies that the sending application would like to receive EchoURL events.
		Required argument: application
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Success
		"""
		_code = 'WWW!'
		_subcode = 'RGUE'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def UnRegisterURLEcho(self, _object, _attributes={}, **_arguments):
		"""UnRegisterURLEcho: Notifies that the sending application would no longer like to receive EchoURL events.
		Required argument: application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'UNRU'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def RegisterWindowClose(self, _object, _attributes={}, **_arguments):
		"""RegisterWindowClose: Notifies that the sending application would like to receive WindowClose events.
		Required argument: application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'RGWC'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def UnRegisterWindowClose(self, _object, _attributes={}, **_arguments):
		"""UnRegisterWindowClose: Notifies that the sending application would no longer like to receive WindowClose events.
		Required argument: application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'UNRC'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def RegisterAppClose(self, _object, _attributes={}, **_arguments):
		"""RegisterAppClose: Notifies that the sending application would like to receive AppClose events.
		Required argument: application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'RGAC'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def UnRegisterAppClose(self, _object, _attributes={}, **_arguments):
		"""UnRegisterAppClose: Notifies that the sending application would no longer like to receive AppClose events.
		Required argument: application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'WWW!'
		_subcode = 'UNRA'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#
# Indices of types declared in this module
#
_classdeclarations = {
}

_propdeclarations = {
}

_compdeclarations = {
}

_enumdeclarations = {
}
