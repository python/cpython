# Module 'panel'
#
# Support for the Panel library.
# Uses built-in module 'pnl'.
# Applications should use 'panel.function' instead of 'pnl.function';
# most 'pnl' functions are transparently exported by 'panel',
# but dopanel() is overridden and you have to use this version
# if you want to use callbacks.


import pnl


debug = 0


# Test if an object is a list.
#
def is_list(x):
	return type(x) == type([])


# Reverse a list.
#
def reverse(list):
	res = []
	for item in list:
		res.insert(0, item)
	return res


# Get an attribute of a list, which may itself be another list.
# Don't use 'prop' for name.
#
def getattrlist(list, name):
	for item in list:
		if item and is_list(item) and item[0] == name:
			return item[1:]
	return []


# Get a property of a list, which may itself be another list.
#
def getproplist(list, name):
	for item in list:
		if item and is_list(item) and item[0] == 'prop':
			if len(item) > 1 and item[1] == name:
				return item[2:]
	return []


# Test if an actuator description contains the property 'end-of-group'
#
def is_endgroup(list):
	x = getproplist(list, 'end-of-group')
	return (x and x[0] == '#t')


# Neatly display an actuator definition given as S-expression
# the prefix string is printed before each line.
#
def show_actuator(prefix, a):
	for item in a:
		if not is_list(item):
			print prefix, item
		elif item and item[0] == 'al':
			print prefix, 'Subactuator list:'
			for a in item[1:]:
				show_actuator(prefix + '    ', a)
		elif len(item) == 2:
			print prefix, item[0], '=>', item[1]
		elif len(item) == 3 and item[0] == 'prop':
			print prefix, 'Prop', item[1], '=>',
			print item[2]
		else:
			print prefix, '?', item


# Neatly display a panel.
#
def show_panel(prefix, p):
	for item in p:
		if not is_list(item):
			print prefix, item
		elif item and item[0] == 'al':
			print prefix, 'Actuator list:'
			for a in item[1:]:
				show_actuator(prefix + '    ', a)
		elif len(item) == 2:
			print prefix, item[0], '=>', item[1]
		elif len(item) == 3 and item[0] == 'prop':
			print prefix, 'Prop', item[1], '=>',
			print item[2]
		else:
			print prefix, '?', item


# Exception raised by build_actuator or build_panel.
#
panel_error = 'panel error'


# Dummy callback used to initialize the callbacks.
#
def dummy_callback(arg):
	pass


# Assign attributes to members of the target.
# Attribute names in exclist are ignored.
# The member name is the attribute name prefixed with the prefix.
#
def assign_members(target, attrlist, exclist, prefix):
	for item in attrlist:
		if is_list(item) and len(item) == 2 and item[0] not in exclist:
			name, value = item[0], item[1]
			ok = 1
			if value[0] in '-0123456789':
				value = eval(value)
			elif value[0] == '"':
				value = value[1:-1]
			elif value == 'move-then-resize':
				# Strange default set by Panel Editor...
				ok = 0
			else:
				print 'unknown value', value, 'for', name
				ok = 0
			if ok:
				lhs = 'target.' + prefix + name
				stmt = lhs + '=' + `value`
				if debug: print 'exec', stmt
				try:
					exec stmt + '\n'
				except KeyboardInterrupt: # Don't catch this!
					raise KeyboardInterrupt
				except:
					print 'assign failed:', stmt


# Build a real actuator from an actuator description.
# Return a pair (actuator, name).
#
def build_actuator(descr):
	namelist = getattrlist(descr, 'name')
	if namelist:
		# Assume it is a string
		actuatorname = namelist[0][1:-1]
	else:
		actuatorname = ''
	type = descr[0]
	if type[:4] == 'pnl_': type = type[4:]
	act = pnl.mkact(type)
	act.downfunc = act.activefunc = act.upfunc = dummy_callback
	#
	assign_members(act, descr[1:], ['al', 'data', 'name'], '')
	#
	# Treat actuator-specific data
	#
	datalist = getattrlist(descr, 'data')
	prefix = ''
	if type[-4:] == 'puck':
		prefix = 'puck_'
	elif type == 'mouse':
		prefix = 'mouse_'
	assign_members(act, datalist, [], prefix)
	#
	return act, actuatorname


# Build all sub-actuators and add them to the super-actuator.
# The super-actuator must already have been added to the panel.
# Sub-actuators with defined names are added as members to the panel
# so they can be referenced as p.name.
#
# Note: I have no idea how panel.endgroup() works when applied
# to a sub-actuator.
#
def build_subactuators(panel, super_act, al):
	#
	# This is nearly the same loop as below in build_panel(),
	# except a call is made to addsubact() instead of addact().
	#
	for a in al:
		act, name = build_actuator(a)
		act.addsubact(super_act)
		if name:
			stmt = 'panel.' + name + ' = act'
			if debug: print 'exec', stmt
			exec stmt + '\n'
		if is_endgroup(a):
			panel.endgroup()
		sub_al = getattrlist(a, 'al')
		if sub_al:
			build_subactuators(panel, act, sub_al)
	#
	# Fix the actuator to which whe just added subactuators.
	# This can't hurt (I hope) and is needed for the scroll actuator.
	#
	super_act.fixact()


# Build a real panel from a panel definition.
# Return a panel object p, where for each named actuator a, p.name is a
# reference to a.
#
def build_panel(descr):
	#
	# Sanity check
	#
	if (not descr) or descr[0] <> 'panel':
		raise panel_error, 'panel description must start with "panel"'
	#
	if debug: show_panel('', descr)
	#
	# Create an empty panel
	#
	panel = pnl.mkpanel()
	#
	# Assign panel attributes
	#
	assign_members(panel, descr[1:], ['al'], '')
	#
	# Look for actuator list
	#
	al = getattrlist(descr, 'al')
	#
	# The order in which actuators are created is important
	# because of the endgroup() operator.
	# Unfortunately the Panel Editor outputs the actuator list
	# in reverse order, so we reverse it here.
	#
	al = reverse(al)
	#
	for a in al:
		act, name = build_actuator(a)
		act.addact(panel)
		if name:
			stmt = 'panel.' + name + ' = act'
			exec stmt + '\n'
		if is_endgroup(a):
			panel.endgroup()
		sub_al = getattrlist(a, 'al')
		if sub_al:
			build_subactuators(panel, act, sub_al)
	#
	return panel


# Wrapper around pnl.dopanel() which calls call-back functions.
#
def my_dopanel():
	# Extract only the first 4 elements to allow for future expansion
	a, down, active, up = pnl.dopanel()[:4]
	if down:
		down.downfunc(down)
	if active:
		active.activefunc(active)
	if up:
		up.upfunc(up)
	return a


# Create one or more panels from a description file (S-expressions)
# generated by the Panel Editor.
# 
def defpanellist(file):
	import panelparser
	descrlist = panelparser.parse_file(open(file, 'r'))
	panellist = []
	for descr in descrlist:
		panellist.append(build_panel(descr))
	return panellist


# Import everything from built-in method pnl, so the user can always
# use panel.foo() instead of pnl.foo().
# This gives *no* performance penalty once this module is imported.
#
from pnl import *			# for export

dopanel = my_dopanel			# override pnl.dopanel
