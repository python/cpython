# -*- Mode: Python; tab-width: 4 -*-
#   Id: asyncore.py,v 2.40 1999/05/27 04:08:25 rushing Exp 
#	Author: Sam Rushing <rushing@nightmare.com>

# ======================================================================
# Copyright 1996 by Sam Rushing
# 
#                         All Rights Reserved
# 
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby
# granted, provided that the above copyright notice appear in all
# copies and that both that copyright notice and this permission
# notice appear in supporting documentation, and that the name of Sam
# Rushing not be used in advertising or publicity pertaining to
# distribution of the software without specific, written prior
# permission.
# 
# SAM RUSHING DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
# NO EVENT SHALL SAM RUSHING BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# ======================================================================

"""Basic infrastructure for asynchronous socket service clients and servers.

There are only two ways to have a program on a single processor do "more
than one thing at a time".  Multi-threaded programming is the simplest and 
most popular way to do it, but there is another very different technique,
that lets you have nearly all the advantages of multi-threading, without
actually using multiple threads. it's really only practical if your program
is largely I/O bound. If your program is CPU bound, then pre-emptive
scheduled threads are probably what you really need. Network servers are
rarely CPU-bound, however. 

If your operating system supports the select() system call in its I/O 
library (and nearly all do), then you can use it to juggle multiple
communication channels at once; doing other work while your I/O is taking
place in the "background."  Although this strategy can seem strange and
complex, especially at first, it is in many ways easier to understand and
control than multi-threaded programming. The module documented here solves
many of the difficult problems for you, making the task of building
sophisticated high-performance network servers and clients a snap. 
"""

import select
import socket
import string
import sys

import os
if os.name == 'nt':
	EWOULDBLOCK	= 10035
	EINPROGRESS	= 10036
	EALREADY	= 10037
	ECONNRESET  = 10054
	ENOTCONN	= 10057
	ESHUTDOWN	= 10058
else:
	from errno import EALREADY, EINPROGRESS, EWOULDBLOCK, ECONNRESET, ENOTCONN, ESHUTDOWN

socket_map = {}

def poll (timeout=0.0):
	if socket_map:
		r = []; w = []; e = []
		for s in socket_map.keys():
			if s.readable():
				r.append (s)
			if s.writable():
				w.append (s)

		(r,w,e) = select.select (r,w,e, timeout)

		for x in r:
			try:
				x.handle_read_event()
			except:
				x.handle_error()
		for x in w:
			try:
				x.handle_write_event()
			except:
				x.handle_error()

def poll2 (timeout=0.0):
	import poll
	# timeout is in milliseconds
	timeout = int(timeout*1000)
	if socket_map:
		fd_map = {}
		for s in socket_map.keys():
			fd_map[s.fileno()] = s
		l = []
		for fd, s in fd_map.items():
			flags = 0
			if s.readable():
				flags = poll.POLLIN
			if s.writable():
				flags = flags | poll.POLLOUT
			if flags:
				l.append (fd, flags)
		r = poll.poll (l, timeout)
		for fd, flags in r:
			s = fd_map[fd]
			try:
				if (flags & poll.POLLIN):
					s.handle_read_event()
				if (flags & poll.POLLOUT):
					s.handle_write_event()
				if (flags & poll.POLLERR):
					s.handle_expt_event()
			except:
				s.handle_error()


def loop (timeout=30.0, use_poll=0):

	if use_poll:
		poll_fun = poll2
	else:
		poll_fun = poll

	while socket_map:
		poll_fun (timeout)

class dispatcher:
	debug = 0
	connected = 0
	accepting = 0
	closing = 0
	addr = None

	def __init__ (self, sock=None):
		if sock:
			self.set_socket (sock)
			# I think it should inherit this anyway
			self.socket.setblocking (0)
			self.connected = 1

	def __repr__ (self):
		try:
			status = []
			if self.accepting and self.addr:
				status.append ('listening')
			elif self.connected:
				status.append ('connected')
			if self.addr:
				status.append ('%s:%d' % self.addr)
			return '<%s %s at %x>' % (
				self.__class__.__name__,
				string.join (status, ' '),
				id(self)
				)
		except:
			try:
				ar = repr(self.addr)
			except:
				ar = 'no self.addr!'
				
			return '<__repr__ (self) failed for object at %x (addr=%s)>' % (id(self),ar)

	def add_channel (self):
		if __debug__:
			self.log ('adding channel %s' % self)
		socket_map [self] = 1

	def del_channel (self):
		if socket_map.has_key (self):
			if __debug__:
				self.log ('closing channel %d:%s' % (self.fileno(), self))
			del socket_map [self]

	def create_socket (self, family, type):
		self.family_and_type = family, type
		self.socket = socket.socket (family, type)
		self.socket.setblocking(0)
		self.add_channel()

	def set_socket (self, socket):
		# This is done so we can be called safely from __init__
		self.__dict__['socket'] = socket
		self.add_channel()

	def set_reuse_addr (self):
		# try to re-use a server port if possible
		try:
			self.socket.setsockopt (
				socket.SOL_SOCKET, socket.SO_REUSEADDR,
				self.socket.getsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR) | 1
				)
		except:
			pass

	# ==================================================
	# predicates for select()
	# these are used as filters for the lists of sockets
	# to pass to select().
	# ==================================================

	def readable (self):
		return 1

	if os.name == 'mac':
		# The macintosh will select a listening socket for
		# write if you let it.  What might this mean?
		def writable (self):
			return not self.accepting
	else:
		def writable (self):
			return 1

	# ==================================================
	# socket object methods.
	# ==================================================

	def listen (self, num):
		self.accepting = 1
		if os.name == 'nt' and num > 5:
			num = 1
		return self.socket.listen (num)

	def bind (self, addr):
		self.addr = addr
		return self.socket.bind (addr)

	def connect (self, address):
		self.connected = 0
		try:
			self.socket.connect (address)
		except socket.error, why:
			if why[0] in (EINPROGRESS, EALREADY, EWOULDBLOCK):
				return
			else:
				raise socket.error, why
		self.connected = 1
		self.handle_connect()

	def accept (self):
		try:
			conn, addr = self.socket.accept()
			return conn, addr
		except socket.error, why:
			if why[0] == EWOULDBLOCK:
				pass
			else:
				raise socket.error, why

	def send (self, data):
		try:
			result = self.socket.send (data)
			return result
		except socket.error, why:
			if why[0] == EWOULDBLOCK:
				return 0
			else:
				raise socket.error, why
			return 0

	def recv (self, buffer_size):
		try:
			data = self.socket.recv (buffer_size)
			if not data:
				# a closed connection is indicated by signaling
				# a read condition, and having recv() return 0.
				self.handle_close()
				return ''
			else:
				return data
		except socket.error, why:
			# winsock sometimes throws ENOTCONN
			if why[0] in [ECONNRESET, ENOTCONN, ESHUTDOWN]:
				self.handle_close()
				return ''
			else:
				raise socket.error, why

	def close (self):
		self.del_channel()
		self.socket.close()

	# cheap inheritance, used to pass all other attribute
	# references to the underlying socket object.
	# NOTE: this may be removed soon for performance reasons.
	def __getattr__ (self, attr):
		return getattr (self.socket, attr)

	def log (self, message):
		print 'log:', message

	def handle_read_event (self):
		if self.accepting:
			# for an accepting socket, getting a read implies
			# that we are connected
			if not self.connected:
				self.connected = 1
			self.handle_accept()
		elif not self.connected:
			self.handle_connect()
			self.connected = 1
			self.handle_read()
		else:
			self.handle_read()

	def handle_write_event (self):
		# getting a write implies that we are connected
		if not self.connected:
			self.handle_connect()
			self.connected = 1
		self.handle_write()

	def handle_expt_event (self):
		self.handle_expt()

	def handle_error (self):
		(file,fun,line), t, v, tbinfo = compact_traceback()

		# sometimes a user repr method will crash.
		try:
			self_repr = repr (self)
		except:
			self_repr = '<__repr__ (self) failed for object at %0x>' % id(self)

		print (
			'uncaptured python exception, closing channel %s (%s:%s %s)' % (
				self_repr,
				t,
				v,
				tbinfo
				)
			)
		self.close()

	def handle_expt (self):
		if __debug__:
			self.log ('unhandled exception')

	def handle_read (self):
		if __debug__:
			self.log ('unhandled read event')

	def handle_write (self):
		if __debug__:
			self.log ('unhandled write event')

	def handle_connect (self):
		if __debug__:
			self.log ('unhandled connect event')

	def handle_accept (self):
		if __debug__:
			self.log ('unhandled accept event')

	def handle_close (self):
		if __debug__:
			self.log ('unhandled close event')
		self.close()

# ---------------------------------------------------------------------------
# adds simple buffered output capability, useful for simple clients.
# [for more sophisticated usage use asynchat.async_chat]
# ---------------------------------------------------------------------------

class dispatcher_with_send (dispatcher):
	def __init__ (self, sock=None):
		dispatcher.__init__ (self, sock)
		self.out_buffer = ''

	def initiate_send (self):
		num_sent = 0
		num_sent = dispatcher.send (self, self.out_buffer[:512])
		self.out_buffer = self.out_buffer[num_sent:]

	def handle_write (self):
		self.initiate_send()

	def writable (self):
		return (not self.connected) or len(self.out_buffer)

	def send (self, data):
		if self.debug:
			self.log ('sending %s' % repr(data))
		self.out_buffer = self.out_buffer + data
		self.initiate_send()

# ---------------------------------------------------------------------------
# used for debugging.
# ---------------------------------------------------------------------------

def compact_traceback ():
	t,v,tb = sys.exc_info()
	tbinfo = []
	while 1:
		tbinfo.append (
			tb.tb_frame.f_code.co_filename,
			tb.tb_frame.f_code.co_name,				
			str(tb.tb_lineno)
			)
		tb = tb.tb_next
		if not tb:
			break

	# just to be safe
	del tb

	file, function, line = tbinfo[-1]
	info = '[' + string.join (
		map (
			lambda x: string.join (x, '|'),
			tbinfo
			),
		'] ['
		) + ']'
	return (file, function, line), t, v, info

def close_all ():
	global socket_map
	for x in socket_map.keys():
		x.socket.close()
	socket_map.clear()

# Asynchronous File I/O:
#
# After a little research (reading man pages on various unixen, and
# digging through the linux kernel), I've determined that select()
# isn't meant for doing doing asynchronous file i/o.
# Heartening, though - reading linux/mm/filemap.c shows that linux
# supports asynchronous read-ahead.  So _MOST_ of the time, the data
# will be sitting in memory for us already when we go to read it.
#
# What other OS's (besides NT) support async file i/o?  [VMS?]
#
# Regardless, this is useful for pipes, and stdin/stdout...

import os
if os.name == 'posix':
	import fcntl
	import FCNTL

	class file_wrapper:
		# here we override just enough to make a file
		# look like a socket for the purposes of asyncore.
		def __init__ (self, fd):
			self.fd = fd

		def recv (self, *args):
			return apply (os.read, (self.fd,)+args)

		def write (self, *args):
			return apply (os.write, (self.fd,)+args)

		def close (self):
			return os.close (self.fd)

		def fileno (self):
			return self.fd

	class file_dispatcher (dispatcher):
		def __init__ (self, fd):
			dispatcher.__init__ (self)
			self.connected = 1
			# set it to non-blocking mode
			flags = fcntl.fcntl (fd, FCNTL.F_GETFL, 0)
			flags = flags | FCNTL.O_NONBLOCK
			fcntl.fcntl (fd, FCNTL.F_SETFL, flags)
			self.set_file (fd)

		def set_file (self, fd):
			self.socket = file_wrapper (fd)
			self.add_channel()

