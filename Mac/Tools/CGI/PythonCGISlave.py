"""PythonCGISlave.py

This program can be used in two ways:
- As a Python CGI script server for web servers supporting "Actions", like WebStar.
- As a wrapper for a single Python CGI script, for any "compliant" Mac web server.

See CGI_README.txt for more details.
"""

#
# Written by Just van Rossum, but partly stolen from example code by Jack.
#


LONG_RUNNING = 1  # If true, don't quit after each request.


import MacOS
MacOS.SchedParams(0, 0)
from MiniAEFrame import AEServer, MiniApplication

import os
import string
import cStringIO
import sys
import traceback
import mimetools

__version__ = '3.2'


slave_dir = os.getcwd()


# log file for errors
sys.stderr = open(sys.argv[0] + ".errors", "a+")

def convertFSSpec(fss):
	return fss.as_pathname()


# AE -> os.environ mappings
ae2environ = {
	'kfor': 'QUERY_STRING',
	'Kcip': 'REMOTE_ADDR',
	'svnm': 'SERVER_NAME',
	'svpt': 'SERVER_PORT',
	'addr': 'REMOTE_HOST',
	'scnm': 'SCRIPT_NAME',
	'meth': 'REQUEST_METHOD',
	'ctyp': 'CONTENT_TYPE',
}


ERROR_MESSAGE = """\
Content-type: text/html

<html>
<head>
<title>Error response</title>
</head>
<body>
<h1>Error response</h1>
<p>Error code %d.
<p>Message: %s.
</body>
</html>
"""


def get_cgi_code():
	# If we're a CGI wrapper, the CGI code resides in a PYC resource.
	import Res, marshal
	try:
		code = Res.GetNamedResource('PYC ', "CGI_MAIN")
	except Res.Error:
		return None
	else:
		return marshal.loads(code.data[8:])



class PythonCGISlave(AEServer, MiniApplication):
	
	def __init__(self):
		self.crumblezone = 100000 * "\0"
		MiniApplication.__init__(self)
		AEServer.__init__(self)
		self.installaehandler('aevt', 'oapp', self.open_app)
		self.installaehandler('aevt', 'quit', self.quit)
		self.installaehandler('WWW\275', 'sdoc', self.cgihandler)
		
		self.code = get_cgi_code()
		self.long_running = LONG_RUNNING
		
		if self.code is None:
			print "%s version %s, ready to serve." % (self.__class__.__name__, __version__)
		else:
			print "%s, ready to serve." % os.path.basename(sys.argv[0])
		
		try:
			self.mainloop()
		except:
			self.crumblezone = None
			sys.stderr.write("- " * 30 + '\n')
			self.message("Unexpected exception")
			self.dump_environ()
			sys.stderr.write("%s: %s\n" % sys.exc_info()[:2])
	
	def getabouttext(self):
		if self.code is None:
			return "PythonCGISlave %s, written by Just van Rossum." % __version__
		else:
			return "Python CGI script, wrapped by BuildCGIApplet and " \
					"PythonCGISlave, version %s." % __version__
	
	def getaboutmenutext(self):
		return "About %s\311" % os.path.basename(sys.argv[0])
	
	def message(self, msg):
		import time
		sys.stderr.write("%s (%s)\n" % (msg, time.asctime(time.localtime(time.time()))))
	
	def dump_environ(self):
		sys.stderr.write("os.environ = {\n")
		keys = os.environ.keys()
		keys.sort()
		for key in keys:
			sys.stderr.write("  %s: %s,\n" % (repr(key), repr(os.environ[key])))
		sys.stderr.write("}\n")
	
	def quit(self, **args):
		self.quitting = 1
	
	def open_app(self, **args):
		pass
	
	def cgihandler(self, pathargs, **args):
		# We emulate the unix way of doing CGI: fill os.environ with stuff.
		environ = os.environ
		
		# First, find the document root. If we don't get a DIRE parameter,
		# we take the directory of this program, which may be wrong if
		# it doesn't live the actual http document root folder.
		if args.has_key('DIRE'):
			http_root = args['DIRE'].as_pathname()
			del args['DIRE']
		else:
			http_root = slave_dir
		environ['DOCUMENT_ROOT'] = http_root
		
		if self.code is None:
			# create a Mac pathname to the Python CGI script or applet
			script = string.replace(args['scnm'], '/', ':')
			script_path = os.path.join(http_root, script)
		else:
			script_path = sys.argv[0]
		
		if not os.path.exists(script_path):
			rv = "HTTP/1.0 404 Not found\n"
			rv = rv + ERROR_MESSAGE % (404, "Not found")
			return rv
		
		# Kfrq is the complete http request.
		infile = cStringIO.StringIO(args['Kfrq'])
		firstline = infile.readline()
		
		msg = mimetools.Message(infile, 0)
		
		uri, protocol = string.split(firstline)[1:3]
		environ['REQUEST_URI'] = uri
		environ['SERVER_PROTOCOL'] = protocol
		
		# Make all http headers available as HTTP_* fields.
		for key in msg.keys():
			environ['HTTP_' + string.upper(string.replace(key, "-", "_"))] = msg[key]
		
		# Translate the AE parameters we know of to the appropriate os.environ
		# entries. Make the ones we don't know available as AE_* fields.
		items = args.items()
		items.sort()
		for key, value in items:
			if key[0] == "_":
				continue
			if ae2environ.has_key(key):
				envkey = ae2environ[key]
				environ[envkey] = value
			else:
				environ['AE_' + string.upper(key)] = str(value)
		
		# Redirect stdout and stdin.
		saveout = sys.stdout
		savein = sys.stdin
		out = sys.stdout = cStringIO.StringIO()
		postdata = args.get('post', "")
		if postdata:
			environ['CONTENT_LENGTH'] = str(len(postdata))
			sys.stdin = cStringIO.StringIO(postdata)
		
		# Set up the Python environment
		script_dir = os.path.dirname(script_path)
		os.chdir(script_dir)
		sys.path.insert(0, script_dir)
		sys.argv[:] = [script_path]
		namespace = {"__name__": "__main__"}
		rv = "HTTP/1.0 200 OK\n"
		
		try:
			if self.code is None:
				# we're a Python script server
				execfile(script_path, namespace)
			else:
				# we're a CGI wrapper, self.code is the CGI code
				exec self.code in namespace
		except SystemExit:
			# We're not exiting dammit! ;-)
			pass
		except:
			self.crumblezone = None
			sys.stderr.write("- " * 30 + '\n')
			self.message("CGI exception")
			self.dump_environ()
			traceback.print_exc()
			sys.stderr.flush()
			self.quitting = 1
			# XXX we should return an error AE, but I don't know how to :-(
			rv = "HTTP/1.0 500 Internal error\n"
		
		# clean up
		namespace.clear()
		environ.clear()
		sys.path.remove(script_dir)
		sys.stdout = saveout
		sys.stdin = savein
		
		if not self.long_running:
			# quit after each request
			self.quitting = 1
		
		return rv + out.getvalue()


PythonCGISlave()
