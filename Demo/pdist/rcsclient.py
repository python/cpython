"Customize this file to change the default client etc."

import string

HOST = 'voorn.cwi.nl'
PORT = 4127
DIRECTORY = '/ufs/guido/voorn/python-RCS/Demo/pdist'
VERBOSE = 1

def openrcsclient(opts = []):
	"open an RCSProxy client based on a list of options returned by getopt"
	import RCSProxy
	host = HOST
	port = PORT
	directory = DIRECTORY
	verbose = VERBOSE
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
	if directory:
		x.cd(directory)
	return x
