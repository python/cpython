# CLASS values (section 3.2.4)

IN = 1		# the Internet
CS = 2		# the CSNET class (Obsolete - used only for examples in
		# some obsolete RFCs)
CH = 3		# the CHAOS class
HS = 4		# Hesiod [Dyer 87]

# QCLASS values (section 3.2.5)

ANY = 255	# any class


# Construct reverse mapping dictionary

_names = dir()
classmap = {}
for _name in _names:
	if _name[0] != '_': classmap[eval(_name)] = _name

def classstr(klass):
	if classmap.has_key(klass): return classmap[klass]
	else: return `klass`
