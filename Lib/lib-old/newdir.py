# New dir() function


# This should be the new dir(), except that it should still list
# the current local name space by default

def listattrs(x):
	try:
		dictkeys = x.__dict__.keys()
	except (AttributeError, TypeError):
		dictkeys = []
	#
	try:
		methods = x.__methods__
	except (AttributeError, TypeError):
		methods = []
	#
	try:
		members = x.__members__
	except (AttributeError, TypeError):
		members = []
	#
	try:
		the_class = x.__class__
	except (AttributeError, TypeError):
		the_class = None
	#
	try:
		bases = x.__bases__
	except (AttributeError, TypeError):
		bases = ()
	#
	total = dictkeys + methods + members
	if the_class:
		# It's a class instace; add the class's attributes
		# that are functions (methods)...
		class_attrs = listattrs(the_class)
		class_methods = []
		for name in class_attrs:
			if is_function(getattr(the_class, name)):
				class_methods.append(name)
		total = total + class_methods
	elif bases:
		# It's a derived class; add the base class attributes
		for base in bases:
			base_attrs = listattrs(base)
			total = total + base_attrs
	total.sort()
	return total
	i = 0
	while i+1 < len(total):
		if total[i] == total[i+1]:
			del total[i+1]
		else:
			i = i+1
	return total


# Helper to recognize functions

def is_function(x):
	return type(x) == type(is_function)


# Approximation of builtin dir(); but note that this lists the user's
# variables by default, not the current local name space.

def dir(x = None):
	if x is not None:
		return listattrs(x)
	else:
		import __main__
		return listattrs(__main__)
