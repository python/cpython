"Customize this file to change the default client etc."

import string

# These defaults don't belong here -- they should be taken from the
# environment or from a hidden file in the current directory

HOST = 'voorn.cwi.nl'
PORT = 4127
VERBOSE = 1

def openrcsclient(opts = []):
	"open an RCSProxy client based on a list of options returned by getopt"
	import RCSProxy
	host = HOST
	port = PORT
	verbose = VERBOSE
	directory = None
	for o, a in opts:
		if o == '-h':
			host = a
			if ':' in host:
				i = string.find(host, ':')
				host, p = host[:i], host[i+1:]
				if p:
					port = string.atoi(p)
		if o == '-p':
			port = string.atoi(a)
		if o == '-d':
			directory = a
		if o == '-v':
			verbose = verbose + 1
		if o == '-q':
			verbose = 0
	address = (host, port)
	x = RCSProxy.RCSProxyClient(address, verbose)
	if not directory:
		try:
			directory = open("CVS/Repository").readline()
		except IOError:
			pass
		else:
			if directory[-1] == '\n':
				directory = directory[:-1]
	if directory:
		x.cd(directory)
	return x
