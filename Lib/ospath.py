# ospath.py is to {posix,mac}path.py what os.py is to modules {posix,mac}

try:
	import posix
	name = 'posix'
	del posix
except ImportError:
	import mac
	name = 'mac'
	del mac

if name == 'posix':
	from posixpath import *
elif name == 'mac':
	from macpath import *
