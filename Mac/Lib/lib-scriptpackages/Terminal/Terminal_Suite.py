"""Suite Terminal Suite: Terms and Events for controlling the Terminal application
Level 1, version 1

Generated from /Applications/Utilities/Terminal.app/Contents/Resources/Terminal.rsrc
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'trmx'

class Terminal_Suite_Events:

	def run(self, _no_object=None, _attributes={}, **_arguments):
		"""run: Run the Terminal application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'oapp'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def quit(self, _no_object=None, _attributes={}, **_arguments):
		"""quit: Quit the Terminal application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'quit'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def count(self, _object=None, _attributes={}, **_arguments):
		"""count: Return the number of elements of a particular class within an object
		Required argument: a reference to the objects to be counted
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the number of objects counted
		"""
		_code = 'core'
		_subcode = 'cnte'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_do_script = {
		'with_command' : 'cmnd',
	}

	def do_script(self, _no_object=None, _attributes={}, **_arguments):
		"""do script: Run a UNIX shell script or command
		Keyword argument with_command: data to be passed to the Terminal application as the command line
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'dosc'

		aetools.keysubst(_arguments, self._argmap_do_script)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class application(aetools.ComponentItem):
	"""application - The Terminal program """
	want = 'capp'
class name(aetools.NProperty):
	"""name - the name of the application """
	which = 'pnam'
	want = 'TEXT'
class version(aetools.NProperty):
	"""version - the version of the application """
	which = 'vers'
	want = 'vers'
class frontmost(aetools.NProperty):
	"""frontmost - Is this the active application? """
	which = 'pisf'
	want = 'bool'
#        element 'cwin' as ['name', 'indx']

applications = application

class window(aetools.ComponentItem):
	"""window - A Terminal window """
	want = 'cwin'
class index(aetools.NProperty):
	"""index - the number of the window """
	which = 'pidx'
	want = 'long'
class visible(aetools.NProperty):
	"""visible - Is the window visible? """
	which = 'pvis'
	want = 'bool'
class has_close_box(aetools.NProperty):
	"""has close box - Does the window have a close box? """
	which = 'hclb'
	want = 'bool'
class has_title_bar(aetools.NProperty):
	"""has title bar - Does the window have a title bar? """
	which = 'ptit'
	want = 'bool'
class floating(aetools.NProperty):
	"""floating - Does the window float? """
	which = 'isfl'
	want = 'bool'
class modal(aetools.NProperty):
	"""modal - Is the window modal? """
	which = 'pmod'
	want = 'bool'
class resizable(aetools.NProperty):
	"""resizable - Is the window resizable? """
	which = 'prsz'
	want = 'bool'
class zoomable(aetools.NProperty):
	"""zoomable - Is the window zoomable? """
	which = 'iszm'
	want = 'bool'
class zoomed(aetools.NProperty):
	"""zoomed - Is the window zoomed? """
	which = 'pzum'
	want = 'bool'
class miniaturizable(aetools.NProperty):
	"""miniaturizable - Is the window miniaturizable? """
	which = 'mini'
	want = 'bool'
class miniaturized(aetools.NProperty):
	"""miniaturized - Is the window miniaturized? """
	which = 'mina'
	want = 'bool'
class position(aetools.NProperty):
	"""position - the upper left coordinates of window """
	which = 'ppos'
	want = 'QDpt'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window """
	which = 'pbnd'
	want = 'qdrt'
class title_displays_device_name(aetools.NProperty):
	"""title displays device name - Does the title for the window contain the device name? """
	which = 'tddn'
	want = 'bool'
class title_displays_shell_path(aetools.NProperty):
	"""title displays shell path - Does the title for the window contain the shell path? """
	which = 'tdsp'
	want = 'bool'
class title_displays_window_size(aetools.NProperty):
	"""title displays window size - Does the title for the window contain the window size? """
	which = 'tdws'
	want = 'bool'
class title_displays_file_name(aetools.NProperty):
	"""title displays file name - Does the title for the window contain the file name? """
	which = 'tdfn'
	want = 'bool'
class title_displays_custom_title(aetools.NProperty):
	"""title displays custom title - Does the title for the window contain a custom title? """
	which = 'tdct'
	want = 'bool'
class custom_title(aetools.NProperty):
	"""custom title - the custom title for the window """
	which = 'titl'
	want = 'TEXT'
class contents(aetools.NProperty):
	"""contents - the currently visible contents of the window """
	which = 'pcnt'
	want = 'TEXT'
class history(aetools.NProperty):
	"""history - the contents of the entire scrolling buffer of the window """
	which = 'hist'
	want = 'TEXT'
class number_of_rows(aetools.NProperty):
	"""number of rows - the number of rows in the window """
	which = 'crow'
	want = 'long'
class number_of_columns(aetools.NProperty):
	"""number of columns - the number of columns in the window """
	which = 'ccol'
	want = 'long'
class cursor_color(aetools.NProperty):
	"""cursor color - the cursor color for the window """
	which = 'pcuc'
	want = 'TEXT'
class background_color(aetools.NProperty):
	"""background color - the background color for the window """
	which = 'pbcl'
	want = 'TEXT'
class normal_text_color(aetools.NProperty):
	"""normal text color - the normal text color for the window """
	which = 'ptxc'
	want = 'TEXT'
class bold_text_color(aetools.NProperty):
	"""bold text color - the bold text color for the window """
	which = 'pbtc'
	want = 'TEXT'

windows = window
application._propdict = {
	'name' : name,
	'version' : version,
	'frontmost' : frontmost,
}
application._elemdict = {
	'window' : window,
}
window._propdict = {
	'name' : name,
	'index' : index,
	'visible' : visible,
	'has_close_box' : has_close_box,
	'has_title_bar' : has_title_bar,
	'floating' : floating,
	'modal' : modal,
	'resizable' : resizable,
	'zoomable' : zoomable,
	'zoomed' : zoomed,
	'miniaturizable' : miniaturizable,
	'miniaturized' : miniaturized,
	'position' : position,
	'bounds' : bounds,
	'title_displays_device_name' : title_displays_device_name,
	'title_displays_shell_path' : title_displays_shell_path,
	'title_displays_window_size' : title_displays_window_size,
	'title_displays_file_name' : title_displays_file_name,
	'title_displays_custom_title' : title_displays_custom_title,
	'custom_title' : custom_title,
	'contents' : contents,
	'history' : history,
	'number_of_rows' : number_of_rows,
	'number_of_columns' : number_of_columns,
	'cursor_color' : cursor_color,
	'background_color' : background_color,
	'normal_text_color' : normal_text_color,
	'bold_text_color' : bold_text_color,
}
window._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cwin' : window,
	'capp' : application,
}

_propdeclarations = {
	'tdfn' : title_displays_file_name,
	'mini' : miniaturizable,
	'vers' : version,
	'pidx' : index,
	'prsz' : resizable,
	'pbnd' : bounds,
	'mina' : miniaturized,
	'pbcl' : background_color,
	'tddn' : title_displays_device_name,
	'iszm' : zoomable,
	'hclb' : has_close_box,
	'isfl' : floating,
	'pcnt' : contents,
	'ppos' : position,
	'ptxc' : normal_text_color,
	'pcuc' : cursor_color,
	'tdsp' : title_displays_shell_path,
	'pvis' : visible,
	'tdct' : title_displays_custom_title,
	'pmod' : modal,
	'titl' : custom_title,
	'pisf' : frontmost,
	'hist' : history,
	'pzum' : zoomed,
	'crow' : number_of_rows,
	'pnam' : name,
	'ccol' : number_of_columns,
	'tdws' : title_displays_window_size,
	'pbtc' : bold_text_color,
	'ptit' : has_title_bar,
}

_compdeclarations = {
}

_enumdeclarations = {
}
