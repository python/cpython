"""Suite Terminal Suite: Terms and Events for controlling the Terminal application
Level 1, version 1

Generated from /Applications/Utilities/Terminal.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'trmx'

class Terminal_Suite_Events:

	def GetURL(self, _object, _attributes={}, **_arguments):
		"""GetURL: Opens a telnet: URL
		Required argument: the object for the command
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'GURL'
		_subcode = 'GURL'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_do_script = {
		'in_' : 'kfil',
		'with_command' : 'cmnd',
	}

	def do_script(self, _object, _attributes={}, **_arguments):
		"""do script: Run a UNIX shell script or command
		Required argument: the object for the command
		Keyword argument in_: the window in which to execute the command
		Keyword argument with_command: data to be passed to the Terminal application as the command line, deprecated, use direct parameter
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the reply for the command
		"""
		_code = 'core'
		_subcode = 'dosc'

		aetools.keysubst(_arguments, self._argmap_do_script)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class application(aetools.ComponentItem):
	"""application - The Terminal program """
	want = 'capp'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - All of the properties of the superclass. """
	which = 'c@#^'
	want = 'capp'
class properties(aetools.NProperty):
	"""properties - every property of the Terminal program """
	which = 'pALL'
	want = '****'
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test', 'ID  ']
#        element 'docu' as ['name', 'indx', 'rele', 'rang', 'test']

applications = application

class window(aetools.ComponentItem):
	"""window - A Terminal window """
	want = 'cwin'
class background_color(aetools.NProperty):
	"""background color - the background color for the window """
	which = 'pbcl'
	want = '****'
class bold_text_color(aetools.NProperty):
	"""bold text color - the bold text color for the window """
	which = 'pbtc'
	want = '****'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window, relative to the upper left corner of the screen """
	which = 'pbnd'
	want = '****'
class busy(aetools.NProperty):
	"""busy - Is the window busy running a process? """
	which = 'busy'
	want = 'bool'
class contents(aetools.NProperty):
	"""contents - the currently visible contents of the window """
	which = 'pcnt'
	want = 'utxt'
class cursor_color(aetools.NProperty):
	"""cursor color - the cursor color for the window """
	which = 'pcuc'
	want = '****'
class custom_title(aetools.NProperty):
	"""custom title - the custom title for the window """
	which = 'titl'
	want = 'utxt'
class frame(aetools.NProperty):
	"""frame - the origin and size of the window """
	which = 'pfra'
	want = '****'
class frontmost(aetools.NProperty):
	"""frontmost - Is the window in front of the other Terminal windows? """
	which = 'pisf'
	want = 'bool'
class history(aetools.NProperty):
	"""history - the contents of the entire scrolling buffer of the window """
	which = 'hist'
	want = 'utxt'
class normal_text_color(aetools.NProperty):
	"""normal text color - the normal text color for the window """
	which = 'ptxc'
	want = '****'
class number_of_columns(aetools.NProperty):
	"""number of columns - the number of columns in the window """
	which = 'ccol'
	want = 'long'
class number_of_rows(aetools.NProperty):
	"""number of rows - the number of rows in the window """
	which = 'crow'
	want = 'long'
class origin(aetools.NProperty):
	"""origin - the lower left coordinates of the window, relative to the lower left corner of the screen """
	which = 'pori'
	want = '****'
class position(aetools.NProperty):
	"""position - the upper left coordinates of the window, relative to the upper left corner of the screen """
	which = 'ppos'
	want = '****'
class processes(aetools.NProperty):
	"""processes - a list of the currently running processes """
	which = 'prcs'
	want = 'utxt'
class size(aetools.NProperty):
	"""size - the width and height of the window """
	which = 'psiz'
	want = '****'
class title_displays_custom_title(aetools.NProperty):
	"""title displays custom title - Does the title for the window contain a custom title? """
	which = 'tdct'
	want = 'bool'
class title_displays_device_name(aetools.NProperty):
	"""title displays device name - Does the title for the window contain the device name? """
	which = 'tddn'
	want = 'bool'
class title_displays_file_name(aetools.NProperty):
	"""title displays file name - Does the title for the window contain the file name? """
	which = 'tdfn'
	want = 'bool'
class title_displays_shell_path(aetools.NProperty):
	"""title displays shell path - Does the title for the window contain the shell path? """
	which = 'tdsp'
	want = 'bool'
class title_displays_window_size(aetools.NProperty):
	"""title displays window size - Does the title for the window contain the window size? """
	which = 'tdws'
	want = 'bool'

windows = window
application._superclassnames = []
import Standard_Suite
application._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'properties' : properties,
}
application._privelemdict = {
	'document' : Standard_Suite.document,
	'window' : window,
}
window._superclassnames = []
window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'background_color' : background_color,
	'bold_text_color' : bold_text_color,
	'bounds' : bounds,
	'busy' : busy,
	'contents' : contents,
	'cursor_color' : cursor_color,
	'custom_title' : custom_title,
	'frame' : frame,
	'frontmost' : frontmost,
	'history' : history,
	'normal_text_color' : normal_text_color,
	'number_of_columns' : number_of_columns,
	'number_of_rows' : number_of_rows,
	'origin' : origin,
	'position' : position,
	'processes' : processes,
	'properties' : properties,
	'size' : size,
	'title_displays_custom_title' : title_displays_custom_title,
	'title_displays_device_name' : title_displays_device_name,
	'title_displays_file_name' : title_displays_file_name,
	'title_displays_shell_path' : title_displays_shell_path,
	'title_displays_window_size' : title_displays_window_size,
}
window._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'capp' : application,
	'cwin' : window,
}

_propdeclarations = {
	'busy' : busy,
	'c@#^' : _3c_Inheritance_3e_,
	'ccol' : number_of_columns,
	'crow' : number_of_rows,
	'hist' : history,
	'pALL' : properties,
	'pbcl' : background_color,
	'pbnd' : bounds,
	'pbtc' : bold_text_color,
	'pcnt' : contents,
	'pcuc' : cursor_color,
	'pfra' : frame,
	'pisf' : frontmost,
	'pori' : origin,
	'ppos' : position,
	'prcs' : processes,
	'psiz' : size,
	'ptxc' : normal_text_color,
	'tdct' : title_displays_custom_title,
	'tddn' : title_displays_device_name,
	'tdfn' : title_displays_file_name,
	'tdsp' : title_displays_shell_path,
	'tdws' : title_displays_window_size,
	'titl' : custom_title,
}

_compdeclarations = {
}

_enumdeclarations = {
}
