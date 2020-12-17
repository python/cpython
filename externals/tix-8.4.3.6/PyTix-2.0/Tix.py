#
# $Id: Tix.py,v 1.5 2000/12/08 01:14:24 idiscovery Exp $
#
# Tix.py -- Tix widget wrappers, part of PyTix.
#
#	- Sudhir Shenoy (sshenoy@gol.com), Dec. 1995.
#	  based on an idea (and a little code !!) of Jean-Marc Lugrin
#						     (lugrin@ms.com)
#
# NOTE: In order to minimize changes to Tkinter.py, some of the code here
#	(TixWidget.__init__) has been taken from Tkinter (Widget.__init__)
#	and will break if there are major changes in Tkinter.
#
# The Tix widgets are represented by a class hierarchy in python with proper
# inheritance of base classes.
#
# As a result after creating a 'w = StdButtonBox', I can write
#		w.ok['text'] = 'Who Cares'
#    or		w.ok['bg'] = w['bg']
# or even	w.ok.invoke()
# etc.
#
# Compare the demo tixwidgets.py to the original Tcl program and you will
# appreciate the advantages.
#

import string
from Tkinter import *
from Tkinter import _flatten, _cnfmerge, _default_root

# WARNING - TkVersion is a limited precision floating point number
if TkVersion < 3.999:
    raise ImportError, "This version of Tix.py requires Tk 4.0 or higher"

import _tkinter # If this fails your Python may not be configured for Tk
TixVersion = string.atof(tkinter.TIX_VERSION) # If this fails your Python may not be configured for Tix
# WARNING - TixVersion is a limited precision floating point number

# Some more constants (for consistency with Tkinter)
WINDOW = 'window'
TEXT = 'text'
IMAGETEXT = 'imagetext'

# BEWARE - this is implemented by copying some code from the Widget class
#          in Tkinter (to override Widget initialization) and is therefore
#          liable to break.
class TixWidget(Widget):
    """A TixWidget class is used to package all (or most) Tix widgets.

    Widget initialization is extended in two ways:
    	1) It is possible to give a list of options which must be part of
	the creation command (so called Tix 'static' options). These cannot be
	given as a 'config' command later.
	2) It is possible to give the name of an existing TK widget. These are
	child widgets created automatically by a Tix mega-widget. The Tk call
	to create these widgets is therefore bypassed in TixWidget.__init__

    Both options are for use by subclasses only.
    """
    def __init__ (self, master=None, widgetName=None,
		  static_options=None, cnf={}, kw={}):
	# Merge keywords and dictionary arguments
	if kw:
	    cnf = _cnfmerge((cnf, kw))
	else:
	    cnf = _cnfmerge(cnf)

	# Move static options into extra. static_options must be
	# a list of keywords (or None).
	extra=()
	if static_options:
	    for k,v in cnf.items()[:]:
		if k in static_options:
		    extra = extra + ('-' + k, v)
		    del cnf[k]

	self.widgetName = widgetName
	Widget._setup(self, master, cnf)

	# If widgetName is None, this is a dummy creation call where the
	# corresponding Tk widget has already been created by Tix
	if widgetName:
	    apply(self.tk.call, (widgetName, self._w) + extra)

	# Non-static options - to be done via a 'config' command
	if cnf:
	    Widget.config(self, cnf)

	# Dictionary to hold subwidget names for easier access. We can't
	# use the children list because the public Tix names may not be the
	# same as the pathname component
	self.subwidget_list = {}

    # We set up an attribute access function so that it is possible to
    # do w.ok['text'] = 'Hello' rather than w.subwidget('ok')['text'] = 'Hello'
    # when w is a StdButtonBox.
    # We can even do w.ok.invoke() because w.ok is subclassed from the
    # Button class if you go through the proper constructors
    def __getattr__(self, name):
	if self.subwidget_list.has_key(name):
	    return self.subwidget_list[name]
	raise AttributeError, name

    # Set a variable without calling its action routine
    def set_silent(self, value):
	self.tk.call('tixSetSilent', self._w, value)

    # Return the named subwidget (which must have been created by
    # the sub-class).
    def subwidget(self, name):
	n = self._subwidget_name(name)
	if not n:
	    raise TclError, "Subwidget " + name + " not child of " + self._name
	# Remove header of name and leading dot
	n = n[len(self._w)+1:]
	return self._nametowidget(n)

    # Return all subwidgets
    def subwidgets_all(self):
	names = self._subwidget_names()
	if not names:
	    return []
	retlist = []
	for name in names:
	    name = name[len(self._w)+1:]
	    try:
		retlist.append(self._nametowidget(name))
	    except:
		# some of the widgets are unknown e.g. border in LabelFrame
		pass
	return retlist

    # Get a subwidget name (returns a String, not a Widget !)
    def _subwidget_name(self,name):
	try:
	    return self.tk.call(self._w, 'subwidget', name)
	except TclError:
	    return None

    # Return the name of all subwidgets
    def _subwidget_names(self):
	try:
	    x = self.tk.call(self._w, 'subwidgets', '-all')
	    return self.tk.split(x)
	except TclError:
	    return None

    # Set configuration options for all subwidgets (and self)
    def config_all(self, option, value):
	if option == '':
	    return
	elif type(option) != type(''):
	    option = `option`
	if type(value) != type(''):
	    value = `value`
	names = self._subwidget_names()
	for name in names:
	    self.tk.call(name, 'configure', '-' + option, value)


# Subwidgets are child widgets created automatically by mega-widgets.
# In python, we have to create these subwidgets manually to mirror their
# existence in Tk/Tix.
class TixSubWidget(TixWidget):
    """Subwidget class.

    This is used to mirror child widgets automatically created
    by Tix/Tk as part of a mega-widget in Python (which is not informed
    of this)"""

    def __init__(self, master, name,
		 destroy_physically=1, check_intermediate=1):
	if check_intermediate:
	    path = master._subwidget_name(name)
	    try:
		path = path[len(master._w)+1:]
		plist = string.splitfields(path, '.')
	    except:
		plist = []

	if (not check_intermediate) or len(plist) < 2:
	    # immediate descendant
	    TixWidget.__init__(self, master, None, None, {'name' : name})
	else:
	    # Ensure that the intermediate widgets exist
	    parent = master
	    for i in range(len(plist) - 1):
		n = string.joinfields(plist[:i+1], '.')
		try:
		    w = master._nametowidget(n)
		    parent = w
		except KeyError:
		    # Create the intermediate widget
		    parent = TixSubWidget(parent, plist[i],
					  destroy_physically=0,
					  check_intermediate=0)
	    TixWidget.__init__(self, parent, None, None, {'name' : name})
	self.destroy_physically = destroy_physically

    def destroy(self):
	# For some widgets e.g., a NoteBook, when we call destructors,
	# we must be careful not to destroy the frame widget since this
	# also destroys the parent NoteBook thus leading to an exception
	# in Tkinter when it finally calls Tcl to destroy the NoteBook
	for c in self.children.values(): c.destroy()
	if self.master.children.has_key(self._name):
	    del self.master.children[self._name]
	if self.master.subwidget_list.has_key(self._name):
	    del self.master.subwidget_list[self._name]
	if self.destroy_physically:
	    # This is bypassed only for a few widgets
	    self.tk.call('destroy', self._w)


# Useful func. to split Tcl lists and return as a dict. From Tkinter.py
def _lst2dict(lst):
    dict = {}
    for x in lst:
	dict[x[0][1:]] = (x[0][1:],) + x[1:]
    return dict

# Useful class to create a display style - later shared by many items.
# Contributed by Steffen Kremser
class DisplayStyle:
    """DisplayStyle - handle configuration options shared by
    (multiple) Display Items"""

    def __init__(self, itemtype, cnf={}, **kw ):
 	master = _default_root		# global from Tkinter
 	if not master and cnf.has_key('refwindow'): master=cnf['refwindow']
 	elif not master and kw.has_key('refwindow'):  master= kw['refwindow']
 	elif not master: raise RuntimeError, "Too early to create display style: no root window"
	self.tk = master.tk
 	self.stylename = apply(self.tk.call, ('tixDisplayStyle', itemtype) +
			       self._options(cnf,kw) )

    def __str__(self):
	return self.stylename
 
    def _options(self, cnf, kw ):
	if kw and cnf:
	    cnf = _cnfmerge((cnf, kw))
	elif kw:
	    cnf = kw
	opts = ()
	for k, v in cnf.items():
	    opts = opts + ('-'+k, v)
	return opts
 
    def delete(self):
	self.tk.call(self.stylename, 'delete')
	del(self)
 
    def __setitem__(self,key,value):
	self.tk.call(self.stylename, 'configure', '-%s'%key, value)
 
    def config(self, cnf={}, **kw):
	return _lst2dict(
	    self.tk.split(
		apply(self.tk.call,
		      (self.stylename, 'configure') + self._options(cnf,kw))))
 
    def __getitem__(self,key):
	return self.tk.call(self.stylename, 'cget', '-%s'%key, value)


######################################################
### The Tix Widget classes - in alphabetical order ###
######################################################

class Balloon(TixWidget):
    """Balloon help widget.

    Subwidget	Class
    ---------	-----
    label	Label
    message	Message"""

    def __init__(self, master=None, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixBalloon', ['options'], cnf, kw)
	self.subwidget_list['label'] = _dummyLabel(self, 'label',
						   destroy_physically=0)
	self.subwidget_list['message'] = _dummyLabel(self, 'message',
						     destroy_physically=0)

    def bind_widget(self, widget, cnf={}, **kw):
	"""Bind balloon widget to another.
	One balloon widget may be bound to several widgets at the same time"""
	apply(self.tk.call, 
	      (self._w, 'bind', widget._w) + self._options(cnf, kw))

    def unbind_widget(self, widget):
	self.tk.call(self._w, 'unbind', widget._w)

class ButtonBox(TixWidget):
    """ButtonBox - A container for pushbuttons"""
    def __init__(self, master=None, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixButtonBox',
			   ['orientation', 'options'], cnf, kw)

    def add(self, name, cnf={}, **kw):
	"""Add a button with given name to box."""

	btn = apply(self.tk.call,
		    (self._w, 'add', name) + self._options(cnf, kw))
	self.subwidget_list[name] = _dummyButton(self, name)
	return btn

    def invoke(self, name):
	if self.subwidget_list.has_key(name):
	    self.tk.call(self._w, 'invoke', name)

class ComboBox(TixWidget):
    """ComboBox - an Entry field with a dropdown menu

    Subwidget	Class
    ---------	-----
    entry	Entry
    arrow	Button
    slistbox	ScrolledListBox
    tick	Button }
    cross	Button } present if created with the fancy option"""

    def __init__ (self, master=None, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixComboBox', 
			   ['editable', 'dropdown', 'fancy', 'options'],
			   cnf, kw)
	self.subwidget_list['label'] = _dummyLabel(self, 'label')
	self.subwidget_list['entry'] = _dummyEntry(self, 'entry')
	self.subwidget_list['arrow'] = _dummyButton(self, 'arrow')
	self.subwidget_list['slistbox'] = _dummyScrolledListBox(self,
								'slistbox')
	try:
	    self.subwidget_list['tick'] = _dummyButton(self, 'tick')
	    self.subwidget_list['cross'] = _dummyButton(self, 'cross')
	except TypeError:
	    # unavailable when -fancy not specified
	    pass

    def add_history(self, str):
	self.tk.call(self._w, 'addhistory', str)

    def append_history(self, str):
	self.tk.call(self._w, 'appendhistory', str)

    def insert(self, index, str):
	self.tk.call(self._w, 'insert', index, str)

    def pick(self, index):
	self.tk.call(self._w, 'pick', index)

class Control(TixWidget):
    """Control - An entry field with value change arrows.

    Subwidget	Class
    ---------	-----
    incr	Button
    decr	Button
    entry	Entry
    label	Label"""

    def __init__ (self, master=None, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixControl', ['options'], cnf, kw)
	self.subwidget_list['incr'] = _dummyButton(self, 'incr')
	self.subwidget_list['decr'] = _dummyButton(self, 'decr')
	self.subwidget_list['label'] = _dummyLabel(self, 'label')
	self.subwidget_list['entry'] = _dummyEntry(self, 'entry')

    def decrement(self):
	self.tk.call(self._w, 'decr')

    def increment(self):
	self.tk.call(self._w, 'incr')

    def invoke(self):
	self.tk.call(self._w, 'invoke')

    def update(self):
	self.tk.call(self._w, 'update')

class DirList(TixWidget):
    """DirList - Directory Listing.

    Subwidget	Class
    ---------	-----
    hlist	HList
    hsb		Scrollbar
    vsb		Scrollbar"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixDirList', ['options'], cnf, kw)
	self.subwidget_list['hlist'] = _dummyHList(self, 'hlist')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

    def chdir(self, dir):
	self.tk.call(self._w, 'chdir', dir)

class DirTree(TixWidget):
    """DirList - Directory Listing in a hierarchical view.

    Subwidget	Class
    ---------	-----
    hlist	HList
    hsb		Scrollbar
    vsb		Scrollbar"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixDirTree', ['options'], cnf, kw)
	self.subwidget_list['hlist'] = _dummyHList(self, 'hlist')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

    def chdir(self, dir):
	self.tk.call(self._w, 'chdir', dir)

class ExFileSelectBox(TixWidget):
    """ExFileSelectBox - MS Windows style file select box.

    Subwidget	Class
    ---------	-----
    cancel	Button
    ok		Button
    hidden	Checkbutton
    types	ComboBox
    dir		ComboBox
    file	ComboBox
    dirlist	ScrolledListBox
    filelist	ScrolledListBox"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixExFileSelectBox', ['options'], cnf, kw)
	self.subwidget_list['cancel'] = _dummyButton(self, 'cancel')
	self.subwidget_list['ok'] = _dummyButton(self, 'ok')
	self.subwidget_list['hidden'] = _dummyCheckbutton(self, 'hidden')
	self.subwidget_list['types'] = _dummyComboBox(self, 'types')
	self.subwidget_list['dir'] = _dummyComboBox(self, 'dir')
	self.subwidget_list['dirlist'] = _dummyDirList(self, 'dirlist')
	self.subwidget_list['file'] = _dummyComboBox(self, 'file')
	self.subwidget_list['filelist'] = _dummyScrolledListBox(self, 'filelist')

    def filter(self):
	self.tk.call(self._w, 'filter')

    def invoke(self):
	self.tk.call(self._w, 'invoke')

class ExFileSelectDialog(TixWidget):
    """ExFileSelectDialog - MS Windows style file select dialog.

    Subwidgets	Class
    ----------	-----
    fsbox	ExFileSelectBox"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixExFileSelectDialog',
			   ['options'], cnf, kw)
	self.subwidget_list['fsbox'] = _dummyExFileSelectBox(self, 'fsbox')

    def popup(self):
	self.tk.call(self._w, 'popup')

    def popdown(self):
	self.tk.call(self._w, 'popdown')

class FileSelectBox(TixWidget):
    """ExFileSelectBox - Motif style file select box.

    Subwidget	Class
    ---------	-----
    selection	ComboBox
    filter	ComboBox
    dirlist	ScrolledListBox
    filelist	ScrolledListBox"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixFileSelectBox', ['options'], cnf, kw)
	self.subwidget_list['dirlist'] = _dummyScrolledListBox(self, 'dirlist')
	self.subwidget_list['filelist'] = _dummyScrolledListBox(self, 'filelist')
	self.subwidget_list['filter'] = _dummyComboBox(self, 'filter')
	self.subwidget_list['selection'] = _dummyComboBox(self, 'selection')

    def apply_filter(self):		# name of subwidget is same as command
	self.tk.call(self._w, 'filter')

    def invoke(self):
	self.tk.call(self._w, 'invoke')

class FileSelectDialog(TixWidget):
    """FileSelectDialog - Motif style file select dialog.

    Subwidgets	Class
    ----------	-----
    btns	StdButtonBox
    fsbox	FileSelectBox"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixFileSelectDialog',
			   ['options'], cnf, kw)
	self.subwidget_list['btns'] = _dummyStdButtonBox(self, 'btns')
	self.subwidget_list['fsbox'] = _dummyFileSelectBox(self, 'fsbox')

    def popup(self):
	self.tk.call(self._w, 'popup')

    def popdown(self):
	self.tk.call(self._w, 'popdown')

class FileEntry(TixWidget):
    """FileEntry - Entry field with button that invokes a FileSelectDialog

    Subwidgets	Class
    ----------	-----
    button	Button
    entry	Entry"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixFileEntry',
			   ['dialogtype', 'options'], cnf, kw)
	self.subwidget_list['button'] = _dummyButton(self, 'button')
	self.subwidget_list['entry'] = _dummyEntry(self, 'entry')

    def invoke(self):
	self.tk.call(self._w, 'invoke')

    def file_dialog(self):
	# XXX return python object
	pass

class HList(TixWidget):
    """HList - Hierarchy display.

    Subwidgets - None"""

    def __init__ (self,master=None,cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixHList',
			   ['columns', 'options'], cnf, kw)

    def add(self, entry, cnf={}, **kw):
	return apply(self.tk.call,
		     (self._w, 'add', entry) + self._options(cnf, kw))

    def add_child(self, parent=None, cnf={}, **kw):
	if not parent:
	    parent = ''
	return apply(self.tk.call,
		     (self._w, 'addchild', parent) + self._options(cnf, kw))

    def anchor_set(self, entry):
	self.tk.call(self._w, 'anchor', 'set', entry)

    def anchor_clear(self):
	self.tk.call(self._w, 'anchor', 'clear')

    def column_width(self, col=0, width=None, chars=None):
	if not chars:
	    return self.tk.call(self._w, 'column', 'width', col, width)
	else:
	    return self.tk.call(self._w, 'column', 'width', col,
				'-char', chars)

    def delete_all(self):
	self.tk.call(self._w, 'delete', 'all')

    def delete_entry(self, entry):
	self.tk.call(self._w, 'delete', 'entry', entry)

    def delete_offsprings(self, entry):
	self.tk.call(self._w, 'delete', 'offsprings', entry)

    def delete_siblings(self, entry):
	self.tk.call(self._w, 'delete', 'siblings', entry)

    def header_create(self, col, cnf={}, **kw):
 	apply(self.tk.call,
 	      (self._w, 'header', 'create', col) + self._options(cnf, kw))
 
    def header_configure(self, col, cnf={}, **kw):
	if cnf is None:
	    return _lst2dict(
		self.tk.split(
		    self.tk.call(self._w, 'header', 'configure', col)))
	apply(self.tk.call, (self._w, 'header', 'configure', col)
	      + self._options(cnf, kw))
 
    def header_cget(self,  col, opt):
	return self.tk.call(self._w, 'header', 'cget', col, opt)
 
    def header_exists(self,  col):
	return self.tk.call(self._w, 'header', 'exists', col)
 
    def header_delete(self, col):
 	self.tk.call(self._w, 'header', 'delete', col)
 
    def header_size(self, col):
 	return self.tk.call(self._w, 'header', 'size', col)
 
    def hide_entry(self, entry):
	self.tk.call(self._w, 'hide', 'entry', entry)

    def indicator_create(self, entry, cnf={}, **kw):
 	apply(self.tk.call,
 	      (self._w, 'indicator', 'create', entry) + self._options(cnf, kw))
 
    def indicator_configure(self, entry, cnf={}, **kw):
	if cnf is None:
	    return _lst2dict(
		self.tk.split(
		    self.tk.call(self._w, 'indicator', 'configure', entry)))
	apply(self.tk.call,
	      (self._w, 'indicator', 'configure', entry) + self._options(cnf, kw))
 
    def indicator_cget(self,  entry, opt):
	return self.tk.call(self._w, 'indicator', 'cget', entry, opt)
 
    def indicator_exists(self,  entry):
	return self.tk.call (self._w, 'indicator', 'exists', entry)
 
    def indicator_delete(self, entry):
 	self.tk.call(self._w, 'indicator', 'delete', entry)
 
    def indicator_size(self, entry):
 	return self.tk.call(self._w, 'indicator', 'size', entry)

    def info_anchor(self):
	return self.tk.call(self._w, 'info', 'anchor')

    def info_children(self, entry=None):
	c = self.tk.call(self._w, 'info', 'children', entry)
	return self.tk.splitlist(c)

    def info_data(self, entry):
	return self.tk.call(self._w, 'info', 'data', entry)

    def info_exists(self, entry):
	return self.tk.call(self._w, 'info', 'exists', entry)

    def info_hidden(self, entry):
	return self.tk.call(self._w, 'info', 'hidden', entry)

    def info_next(self, entry):
	return self.tk.call(self._w, 'info', 'next', entry)

    def info_parent(self, entry):
	return self.tk.call(self._w, 'info', 'parent', entry)

    def info_prev(self, entry):
	return self.tk.call(self._w, 'info', 'prev', entry)

    def info_selection(self):
	c = self.tk.call(self._w, 'info', 'selection')
	return self.tk.splitlist(c)

    def item_cget(self,  col, opt):
	return self.tk.call(self._w, 'item', 'cget', col, opt)
 
    def item_configure(self, entry, col, cnf={}, **kw):
	if cnf is None:
	    return _lst2dict(
		self.tk.split(
		    self.tk.call(self._w, 'item', 'configure', entry, col)))
	apply(self.tk.call, (self._w, 'item', 'configure', entry, col) +
	      self._options(cnf, kw))

    def item_create(self, entry, col, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'item', 'create', entry, col) + self._options(cnf, kw))

    def item_exists(self, entry, col):
	return self.tk.call(self._w, 'item', 'exists', entry, col)
 
    def item_delete(self, entry, col):
	self.tk.call(self._w, 'item', 'delete', entry, col)

    def nearest(self, y):
	return self.tk.call(self._w, 'nearest', y)

    def see(self, entry):
	self.tk.call(self._w, 'see', entry)

    def selection_clear(self, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'selection', 'clear') + self._options(cnf, kw))

    def selection_includes(self, entry):
	return self.tk.call(self._w, 'selection', 'includes', entry)

    def selectiom_set(self, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'selection', 'set') + self._options(cnf, kw))

    def show_entry(self, entry):
	return self.tk.call(self._w, 'show', 'entry', entry)

    def xview(self, *args):
	apply(self.tk.call, (self._w, 'xview') + args)

    def yview(self, *args):
	apply(self.tk.call, (self._w, 'yview') + args)

class InputOnly(TixWidget):
    """InputOnly - Invisible widget.

    Subwidgets - None"""

    def __init__ (self,master=None,cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixInputOnly', None, cnf, kw)

class LabelEntry(TixWidget):
    """LabelEntry - Entry field with label.

    Subwidgets	Class
    ----------	-----
    label	Label
    entry	Entry"""

    def __init__ (self,master=None,cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixLabelEntry',
			   ['labelside','options'], cnf, kw)
	self.subwidget_list['label'] = _dummyLabel(self, 'label')
	self.subwidget_list['entry'] = _dummyEntry(self, 'entry')

class LabelFrame(TixWidget):
    """LabelFrame - Labelled Frame container.

    Subwidgets	Class
    ----------	-----
    label	Label
    frame	Frame"""

    def __init__ (self,master=None,cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixLabelFrame',
			   ['labelside','options'], cnf, kw)
	self.subwidget_list['label'] = _dummyLabel(self, 'label')
	self.subwidget_list['frame'] = _dummyFrame(self, 'frame')

class Meter(TixWidget):   # Rozen
    """Meter - Widget suitable for displaying progress.

    No Subwidgets"""

    def __init__ (self,master=None,cnf={}, **kw):
        TixWidget.__init__(self,master,'tixMeter',['options'], cnf, kw)

class NoteBook(TixWidget):
    """NoteBook - Multi-page container widget (tabbed notebook metaphor).

    Subwidgets	Class
    ----------	-----
    nbframe	NoteBookFrame
    <pages>	g/p widgets added dynamically"""

    def __init__ (self,master=None,cnf={}, **kw):
	TixWidget.__init__(self,master,'tixNoteBook', ['options'], cnf, kw)
	self.subwidget_list['nbframe'] = TixSubWidget(self, 'nbframe',
						      destroy_physically=0)

    def add(self, name, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'add', name) + self._options(cnf, kw))
	self.subwidget_list[name] = TixSubWidget(self, name)
	return self.subwidget_list[name]

    def delete(self, name):
	self.tk.call(self._w, 'delete', name)

    def page(self, name):
	return self.subwidget(name)

    def pages(self):
	# Can't call subwidgets_all directly because we don't want .nbframe
	names = self.tk.split(self.tk.call(self._w, 'pages'))
	ret = []
	for x in names:
	    ret.append(self.subwidget(x))
	return ret

    def raise_page(self, name):		# raise is a python keyword
	self.tk.call(self._w, 'raise', name)

    def raised(self):
	return self.tk.call(self._w, 'raised')

class NoteBookFrame(TixWidget):
    """Will be added when Tix documentation is available !!!"""
    pass

class OptionMenu(TixWidget):
    """OptionMenu - Option menu widget.

    Subwidget	Class
    ---------	-----
    menubutton	Menubutton
    menu	Menu"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixOptionMenu', ['options'], cnf, kw)
	self.subwidget_list['menubutton'] = _dummyMenubutton(self, 'menubutton')
	self.subwidget_list['menu'] = _dummyMenu(self, 'menu')

    def add_command(self, name, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'add', 'command', name) + self._options(cnf, kw))

    def add_separator(self, name, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'add', 'separator', name) + self._options(cnf, kw))

    def delete(self, name):
	self.tk.call(self._w, 'delete', name)

    def disable(self, name):
	self.tk.call(self._w, 'disable', name)

    def enable(self, name):
	self.tk.call(self._w, 'enable', name)

class PanedWindow(TixWidget):
    """PanedWindow - Multi-pane container widget. Panes are resizable.

    Subwidgets	Class
    ----------	-----
    <panes>	g/p widgets added dynamically"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixPanedWindow', ['orientation', 'options'], cnf, kw)

    def add(self, name, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'add', name) + self._options(cnf, kw))
	self.subwidget_list[name] = TixSubWidget(self, name,
						 check_intermediate=0)
	return self.subwidget_list[name]

    def panes(self):
	names = self.tk.call(self._w, 'panes')
	ret = []
	for x in names:
	    ret.append(self.subwidget(x))
	return ret

class PopupMenu(TixWidget):
    """PopupMenu widget.

    Subwidgets	Class
    ----------	-----
    menubutton	Menubutton
    menu	Menu"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixPopupMenu', ['options'], cnf, kw)
	self.subwidget_list['menubutton'] = _dummyMenubutton(self, 'menubutton')
	self.subwidget_list['menu'] = _dummyMenu(self, 'menu')

    def bind_widget(self, widget):
	self.tk.call(self._w, 'bind', widget._w)

    def unbind_widget(self, widget):
	self.tk.call(self._w, 'unbind', widget._w)

    def post_widget(self, widget, x, y):
	self.tk.call(self._w, 'post', widget._w, x, y)

class ResizeHandle(TixWidget):
    """Incomplete - no documentation in Tix for this !!!"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixResizeHandle',
			   ['options'], cnf, kw)

    def attach_widget(self, widget):
	self.tk.call(self._w, 'attachwidget', widget._w)

class ScrolledHList(TixWidget):
    """ScrolledHList - HList with scrollbars."""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixScrolledHList', ['options'],
			   cnf, kw)
	self.subwidget_list['hlist'] = _dummyHList(self, 'hlist')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

class ScrolledListBox(TixWidget):
    """ScrolledListBox - Listbox with scrollbars."""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixScrolledListBox', ['options'], cnf, kw)
	self.subwidget_list['listbox'] = _dummyListbox(self, 'listbox')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

class ScrolledText(TixWidget):
    """ScrolledText - Text with scrollbars."""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixScrolledText', ['options'], cnf, kw)
	self.subwidget_list['text'] = _dummyText(self, 'text')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

class ScrolledWindow(TixWidget):
    """ScrolledWindow - Window with scrollbars."""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixScrolledWindow', ['options'], cnf, kw)
	self.subwidget_list['window'] = _dummyFrame(self, 'window')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

class Select(TixWidget):
    """Select - Container for buttons. Can enforce radio buttons etc.

    Subwidgets are buttons added dynamically"""

    def __init__(self, master, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixSelect',
			   ['allowzero', 'radio', 'orientation', 'labelside',
			    'options'],
			   cnf, kw)
	self.subwidget_list['label'] = _dummyLabel(self, 'label')

    def add(self, name, cnf={}, **kw):
	apply(self.tk.call,
	      (self._w, 'add', name) + self._options(cnf, kw))
	self.subwidget_list[name] = _dummyButton(self, name)
	return self.subwidget_list[name]

    def invoke(self, name):
	self.tk.call(self._w, 'invoke', name)

class StdButtonBox(TixWidget):
    """StdButtonBox - Standard Button Box (OK, Apply, Cancel and Help) """

    def __init__(self, master=None, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixStdButtonBox',
			   ['orientation', 'options'], cnf, kw)
	self.subwidget_list['ok'] = _dummyButton(self, 'ok')
	self.subwidget_list['apply'] = _dummyButton(self, 'apply')
	self.subwidget_list['cancel'] = _dummyButton(self, 'cancel')
	self.subwidget_list['help'] = _dummyButton(self, 'help')

    def invoke(self, name):
	if self.subwidget_list.has_key(name):
	    self.tk.call(self._w, 'invoke', name)

class Tree(TixWidget):
    """Tree - The tixTree widget (general purpose DirList like widget)"""

    def __init__(self, master=None, cnf={}, **kw):
	TixWidget.__init__(self, master, 'tixTree',
			   ['options'], cnf, kw)
	self.subwidget_list['hlist'] = _dummyHList(self, 'hlist')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

    def autosetmode(self):
	self.tk.call(self._w, 'autosetmode')

    def close(self, entrypath):
	self.tk.call(self._w, 'close', entrypath)

    def getmode(self, entrypath):
	return self.tk.call(self._w, 'getmode', entrypath)

    def open(self, entrypath):
	self.tk.call(self._w, 'open', entrypath)

    def setmode(self, entrypath, mode='none'):
	self.tk.call(self._w, 'setmode', entrypath, mode)

###########################################################################
### The subclassing below is used to instantiate the subwidgets in each ###
### mega widget. This allows us to access their methods directly.       ###
###########################################################################

class _dummyButton(TixSubWidget, Button):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyCheckbutton(TixSubWidget, Checkbutton):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyEntry(TixSubWidget, Entry):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyFrame(TixSubWidget, Frame):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyLabel(TixSubWidget, Label):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyListbox(TixSubWidget, Listbox):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyMenu(TixSubWidget, Menu):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyMenubutton(TixSubWidget, Menubutton):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyScrollbar(TixSubWidget, Scrollbar):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyText(TixSubWidget, Text):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyScrolledListBox(TixSubWidget, ScrolledListBox):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)
	self.subwidget_list['listbox'] = _dummyListbox(self, 'listbox')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

class _dummyHList(TixSubWidget, HList):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)

class _dummyComboBox(TixSubWidget, ComboBox):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)
	self.subwidget_list['entry'] = _dummyEntry(self, 'entry')
	self.subwidget_list['arrow'] = _dummyButton(self, 'arrow')
	self.subwidget_list['slistbox'] = _dummyScrolledListBox(self,
								'slistbox')
	self.subwidget_list['listbox'] = _dummyListbox(self, 'listbox',
						       destroy_physically=0)

class _dummyDirList(TixSubWidget, DirList):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)
	self.subwidget_list['hlist'] = _dummyHList(self, 'hlist')
	self.subwidget_list['vsb'] = _dummyScrollbar(self, 'vsb')
	self.subwidget_list['hsb'] = _dummyScrollbar(self, 'hsb')

class _dummyExFileSelectBox(TixSubWidget, ExFileSelectBox):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)
	self.subwidget_list['cancel'] = _dummyButton(self, 'cancel')
	self.subwidget_list['ok'] = _dummyButton(self, 'ok')
	self.subwidget_list['hidden'] = _dummyCheckbutton(self, 'hidden')
	self.subwidget_list['types'] = _dummyComboBox(self, 'types')
	self.subwidget_list['dir'] = _dummyComboBox(self, 'dir')
	self.subwidget_list['dirlist'] = _dummyScrolledListBox(self, 'dirlist')
	self.subwidget_list['file'] = _dummyComboBox(self, 'file')
	self.subwidget_list['filelist'] = _dummyScrolledListBox(self, 'filelist')

class _dummyFileSelectBox(TixSubWidget, FileSelectBox):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)
	self.subwidget_list['dirlist'] = _dummyScrolledListBox(self, 'dirlist')
	self.subwidget_list['filelist'] = _dummyScrolledListBox(self, 'filelist')
	self.subwidget_list['filter'] = _dummyComboBox(self, 'filter')
	self.subwidget_list['selection'] = _dummyComboBox(self, 'selection')

class _dummyStdButtonBox(TixSubWidget, StdButtonBox):
    def __init__(self, master, name, destroy_physically=1):
	TixSubWidget.__init__(self, master, name, destroy_physically)
	self.subwidget_list['ok'] = _dummyButton(self, 'ok')
	self.subwidget_list['apply'] = _dummyButton(self, 'apply')
	self.subwidget_list['cancel'] = _dummyButton(self, 'cancel')
	self.subwidget_list['help'] = _dummyButton(self, 'help')

class _dummyNoteBookFrame(TixSubWidget, NoteBookFrame):
    def __init__(self, master, name, destroy_physically=0):
	TixSubWidget.__init__(self, master, name, destroy_physically)

########################
### Utility Routines ###
########################

# Returns the qualified path name for the widget. Normally used to set
# default options for subwidgets. See tixwidgets.py
def OptionName(widget):
    return widget.tk.call('tixOptionName', widget._w)

# Called with a dictionary argument of the form
# {'*.c':'C source files', '*.txt':'Text Files', '*':'All files'}
# returns a string which can be used to configure the fsbox file types
# in an ExFileSelectBox. i.e.,
# '{{*} {* - All files}} {{*.c} {*.c - C source files}} {{*.txt} {*.txt - Text Files}}'
def FileTypeList(dict):
    s = ''
    for type in dict.keys():
	s = s + '{{' + type + '} {' + type + ' - ' + dict[type] + '}} '
    return s


