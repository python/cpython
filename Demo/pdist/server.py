"""RPC Server module."""

import sys
import socket
import pickle
from fnmatch import fnmatch
from repr import repr


# Default verbosity (0 = silent, 1 = print connections, 2 = print requests too)
VERBOSE = 1


class Server:
	
	"""RPC Server class.  Derive a class to implement a particular service."""
	
	def __init__(self, address, verbose = VERBOSE):
		if type(address) == type(0):
			address = ('', address)
		self._address = address
		self._verbose = verbose
		self._socket = None
		self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self._socket.bind(address)
		self._socket.listen(1)
		self._listening = 1
	
	def _setverbose(self, verbose):
		self._verbose = verbose
	
	def __del__(self):
		self._close()
	
	def _close(self):
		self._listening = 0
		if self._socket:
			self._socket.close()
		self._socket = None
	
	def _serverloop(self):
		while self._listening:
			self._serve()
	
	def _serve(self):
		if self._verbose: print "Wait for connection ..."
		conn, address = self._socket.accept()
		if self._verbose: print "Accepted connection from %s" % repr(address)
		if not self._verify(conn, address):
			print "*** Connection from %s refused" % repr(address)
			conn.close()
			return
		rf = conn.makefile('r')
		wf = conn.makefile('w')
		ok = 1
		while ok:
			wf.flush()
			if self._verbose > 1: print "Wait for next request ..."
			ok = self._dorequest(rf, wf)
	
	_valid = ['192.16.201.*', '192.16.197.*', '132.151.1.*', '129.6.64.*']
	
	def _verify(self, conn, address):
		host, port = address
		for pat in self._valid:
			if fnmatch(host, pat): return 1
		return 0
	
	def _dorequest(self, rf, wf):
		rp = pickle.Unpickler(rf)
		try:
			request = rp.load()
		except EOFError:
			return 0
		if self._verbose > 1: print "Got request: %s" % repr(request)
		try:
			methodname, args, id = request
			if '.' in methodname:
				reply = (None, self._special(methodname, args), id)
			elif methodname[0] == '_':
				raise NameError, "illegal method name %s" % repr(methodname)
			else:
				method = getattr(self, methodname)
				reply = (None, apply(method, args), id)
		except:
			reply = (sys.exc_type, sys.exc_value, id)
		if id < 0 and reply[:2] == (None, None):
			if self._verbose > 1: print "Suppress reply"
			return 1
		if self._verbose > 1: print "Send reply: %s" % repr(reply)
		wp = pickle.Pickler(wf)
		wp.dump(reply)
		return 1
	
	def _special(self, methodname, args):
		if methodname == '.methods':
			if not hasattr(self, '_methods'):
				self._methods = tuple(self._listmethods())
			return self._methods
		raise NameError, "unrecognized special method name %s" % repr(methodname)
	
	def _listmethods(self, cl=None):
		if not cl: cl = self.__class__
		names = cl.__dict__.keys()
		names = filter(lambda x: x[0] != '_', names)
		names.sort()
		for base in cl.__bases__:
			basenames = self._listmethods(base)
			basenames = filter(lambda x, names=names: x not in names, basenames)
			names[len(names):] = basenames
		return names


from security import Security


class SecureServer(Server, Security):

	def __init__(self, *args):
		apply(Server.__init__, (self,) + args)
		Security.__init__(self)

	def _verify(self, conn, address):
		import string
		challenge = self._generate_challenge()
		conn.send("%d\n" % challenge)
		response = ""
		while "\n" not in response and len(response) < 100:
			data = conn.recv(100)
			if not data:
				break
			response = response + data
		try:
			response = string.atol(string.strip(response))
		except string.atol_error:
			if self._verbose > 0:
				print "Invalid response syntax", `response`
			return 0
		if not self._compare_challenge_response(challenge, response):
			if self._verbose > 0:
				print "Invalid response value", `response`
			return 0
		if self._verbose > 1:
			print "Response matches challenge.  Go ahead!"
		return 1
