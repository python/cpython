#! /usr/bin/env python

# A simple gopher client.
#
# Usage: gopher [ [selector] host [port] ]

import string
import sys
import os
import socket

# Default selector, host and port
DEF_SELECTOR = ''
DEF_HOST     = 'gopher.micro.umn.edu'
DEF_PORT     = 70

# Recognized file types
T_TEXTFILE  = '0'
T_MENU      = '1'
T_CSO       = '2'
T_ERROR     = '3'
T_BINHEX    = '4'
T_DOS       = '5'
T_UUENCODE  = '6'
T_SEARCH    = '7'
T_TELNET    = '8'
T_BINARY    = '9'
T_REDUNDANT = '+'
T_SOUND     = 's'

# Dictionary mapping types to strings
typename = {'0': '<TEXT>', '1': '<DIR>', '2': '<CSO>', '3': '<ERROR>', \
	'4': '<BINHEX>', '5': '<DOS>', '6': '<UUENCODE>', '7': '<SEARCH>', \
	'8': '<TELNET>', '9': '<BINARY>', '+': '<REDUNDANT>', 's': '<SOUND>'}

# Oft-used characters and strings
CRLF = '\r\n'
TAB = '\t'

# Open a TCP connection to a given host and port
def open_socket(host, port):
	if not port:
		port = DEF_PORT
	elif type(port) == type(''):
		port = string.atoi(port)
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((host, port))
	return s

# Send a selector to a given host and port, return a file with the reply
def send_request(selector, host, port):
	s = open_socket(host, port)
	s.send(selector + CRLF)
	s.shutdown(1)
	return s.makefile('r')

# Get a menu in the form of a list of entries
def get_menu(selector, host, port):
	f = send_request(selector, host, port)
	list = []
	while 1:
		line = f.readline()
		if not line:
			print '(Unexpected EOF from server)'
			break
		if line[-2:] == CRLF:
			line = line[:-2]
		elif line[-1:] in CRLF:
			line = line[:-1]
		if line == '.':
			break
		if not line:
			print '(Empty line from server)'
			continue
		typechar = line[0]
		parts = string.splitfields(line[1:], TAB)
		if len(parts) < 4:
			print '(Bad line from server:', `line`, ')'
			continue
		if len(parts) > 4:
			print '(Extra info from server:', parts[4:], ')'
		parts.insert(0, typechar)
		list.append(parts)
	f.close()
	return list

# Get a text file as a list of lines, with trailing CRLF stripped
def get_textfile(selector, host, port):
	list = []
	get_alt_textfile(selector, host, port, list.append)
	return list

# Get a text file and pass each line to a function, with trailing CRLF stripped
def get_alt_textfile(selector, host, port, func):
	f = send_request(selector, host, port)
	while 1:
		line = f.readline()
		if not line:
			print '(Unexpected EOF from server)'
			break
		if line[-2:] == CRLF:
			line = line[:-2]
		elif line[-1:] in CRLF:
			line = line[:-1]
		if line == '.':
			break
		if line[:2] == '..':
			line = line[1:]
		func(line)
	f.close()

# Get a binary file as one solid data block
def get_binary(selector, host, port):
	f = send_request(selector, host, port)
	data = f.read()
	f.close()
	return data

# Get a binary file and pass each block to a function
def get_alt_binary(selector, host, port, func, blocksize):
	f = send_request(selector, host, port)
	while 1:
		data = f.read(blocksize)
		if not data:
			break
		func(data)

# A *very* simple interactive browser

# Browser main command, has default arguments
def browser(*args):
	selector = DEF_SELECTOR
	host = DEF_HOST
	port = DEF_PORT
	n = len(args)
	if n > 0 and args[0]:
		selector = args[0]
	if n > 1 and args[1]:
		host = args[1]
	if n > 2 and args[2]:
		port = args[2]
	if n > 3:
		raise RuntimeError, 'too many args'
	try:
		browse_menu(selector, host, port)
	except socket.error, msg:
		print 'Socket error:', msg
		sys.exit(1)
	except KeyboardInterrupt:
		print '\n[Goodbye]'

# Browse a menu
def browse_menu(selector, host, port):
	list = get_menu(selector, host, port)
	while 1:
		print '----- MENU -----'
		print 'Selector:', `selector`
		print 'Host:', host, ' Port:', port
		print
		for i in range(len(list)):
			item = list[i]
			typechar, description = item[0], item[1]
			print string.rjust(`i+1`, 3) + ':', description,
			if typename.has_key(typechar):
				print typename[typechar]
			else:
				print '<TYPE=' + `typechar` + '>'
		print
		while 1:
			try:
				str = raw_input('Choice [CR == up a level]: ')
			except EOFError:
				print
				return
			if not str:
				return
			try:
				choice = string.atoi(str)
			except string.atoi_error:
				print 'Choice must be a number; try again:'
				continue
			if not 0 < choice <= len(list):
				print 'Choice out of range; try again:'
				continue
			break
		item = list[choice-1]
		typechar = item[0]
		[i_selector, i_host, i_port] = item[2:5]
		if typebrowser.has_key(typechar):
			browserfunc = typebrowser[typechar]
			try:
				browserfunc(i_selector, i_host, i_port)
			except (IOError, socket.error):
				print '***', sys.exc_type, ':', sys.exc_value
		else:
			print 'Unsupported object type'

# Browse a text file
def browse_textfile(selector, host, port):
	x = None
	try:
		p = os.popen('${PAGER-more}', 'w')
		x = SaveLines(p)
		get_alt_textfile(selector, host, port, x.writeln)
	except IOError, msg:
		print 'IOError:', msg
	if x:
		x.close()
	f = open_savefile()
	if not f:
		return
	x = SaveLines(f)
	try:
		get_alt_textfile(selector, host, port, x.writeln)
		print 'Done.'
	except IOError, msg:
		print 'IOError:', msg
	x.close()

# Browse a search index
def browse_search(selector, host, port):
	while 1:
		print '----- SEARCH -----'
		print 'Selector:', `selector`
		print 'Host:', host, ' Port:', port
		print
		try:
			query = raw_input('Query [CR == up a level]: ')
		except EOFError:
			print
			break
		query = string.strip(query)
		if not query:
			break
		if '\t' in query:
			print 'Sorry, queries cannot contain tabs'
			continue
		browse_menu(selector + TAB + query, host, port)

# "Browse" telnet-based information, i.e. open a telnet session
def browse_telnet(selector, host, port):
	if selector:
		print 'Log in as', `selector`
	if type(port) <> type(''):
		port = `port`
	sts = os.system('set -x; exec telnet ' + host + ' ' + port)
	if sts:
		print 'Exit status:', sts

# "Browse" a binary file, i.e. save it to a file
def browse_binary(selector, host, port):
	f = open_savefile()
	if not f:
		return
	x = SaveWithProgress(f)
	get_alt_binary(selector, host, port, x.write, 8*1024)
	x.close()

# "Browse" a sound file, i.e. play it or save it
def browse_sound(selector, host, port):
	browse_binary(selector, host, port)

# Dictionary mapping types to browser functions
typebrowser = {'0': browse_textfile, '1': browse_menu, \
	'4': browse_binary, '5': browse_binary, '6': browse_textfile, \
	'7': browse_search, \
	'8': browse_telnet, '9': browse_binary, 's': browse_sound}

# Class used to save lines, appending a newline to each line
class SaveLines:
	def __init__(self, f):
		self.f = f
	def writeln(self, line):
		self.f.write(line + '\n')
	def close(self):
		sts = self.f.close()
		if sts:
			print 'Exit status:', sts

# Class used to save data while showing progress
class SaveWithProgress:
	def __init__(self, f):
		self.f = f
	def write(self, data):
		sys.stdout.write('#')
		sys.stdout.flush()
		self.f.write(data)
	def close(self):
		print
		sts = self.f.close()
		if sts:
			print 'Exit status:', sts

# Ask for and open a save file, or return None if not to save
def open_savefile():
	try:
		savefile = raw_input( \
	    'Save as file [CR == don\'t save; |pipeline or ~user/... OK]: ')
	except EOFError:
		print
		return None
	savefile = string.strip(savefile)
	if not savefile:
		return None
	if savefile[0] == '|':
		cmd = string.strip(savefile[1:])
		try:
			p = os.popen(cmd, 'w')
		except IOError, msg:
			print `cmd`, ':', msg
			return None
		print 'Piping through', `cmd`, '...'
		return p
	if savefile[0] == '~':
		savefile = os.path.expanduser(savefile)
	try:
		f = open(savefile, 'w')
	except IOError, msg:
		print `savefile`, ':', msg
		return None
	print 'Saving to', `savefile`, '...'
	return f

# Test program
def test():
	if sys.argv[4:]:
		print 'usage: gopher [ [selector] host [port] ]'
		sys.exit(2)
	elif sys.argv[3:]:
		browser(sys.argv[1], sys.argv[2], sys.argv[3])
	elif sys.argv[2:]:
		try:
			port = string.atoi(sys.argv[2])
			selector = ''
			host = sys.argv[1]
		except string.atoi_error:
			selector = sys.argv[1]
			host = sys.argv[2]
			port = ''
		browser(selector, host, port)
	elif sys.argv[1:]:
		browser('', sys.argv[1])
	else:
		browser()

# Call the test program as a main program
test()
