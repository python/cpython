#! /usr/bin/env python

'''Mimification and unmimification of mail messages.

decode quoted-printable parts of a mail message or encode using
quoted-printable.

Usage:
	mimify(input, output)
	unmimify(input, output, decode_base64 = 0)
to encode and decode respectively.  Input and output may be the name
of a file or an open file object.  Only a readline() method is used
on the input file, only a write() method is used on the output file.
When using file names, the input and output file names may be the
same.

Interactive usage:
	mimify.py -e [infile [outfile]]
	mimify.py -d [infile [outfile]]
to encode and decode respectively.  Infile defaults to standard
input and outfile to standard output.
'''

# Configure
MAXLEN = 200	# if lines longer than this, encode as quoted-printable
CHARSET = 'ISO-8859-1'	# default charset for non-US-ASCII mail
QUOTE = '> '		# string replies are quoted with
# End configure

import regex, regsub, string

qp = regex.compile('^content-transfer-encoding:[ \t]*quoted-printable',
		   regex.casefold)
base64_re = regex.compile('^content-transfer-encoding:[ \t]*base64',
		       regex.casefold)
mp = regex.compile('^content-type:[\000-\377]*multipart/[\000-\377]*boundary="?\\([^;"\n]*\\)',
		   regex.casefold)
chrset = regex.compile('^\\(content-type:.*charset="\\)\\(us-ascii\\|iso-8859-[0-9]+\\)\\("[\000-\377]*\\)',
		       regex.casefold)
he = regex.compile('^-*$')
mime_code = regex.compile('=\\([0-9a-f][0-9a-f]\\)', regex.casefold)
mime_head = regex.compile('=\\?iso-8859-1\\?q\\?\\([^? \t\n]+\\)\\?=',
			  regex.casefold)
repl = regex.compile('^subject:[ \t]+re: ', regex.casefold)

class File:
	'''A simple fake file object that knows about limited
	   read-ahead and boundaries.
	   The only supported method is readline().'''

	def __init__(self, file, boundary):
		self.file = file
		self.boundary = boundary
		self.peek = None

	def readline(self):
		if self.peek is not None:
			return ''
		line = self.file.readline()
		if not line:
			return line
		if self.boundary:
			if line == self.boundary + '\n':
				self.peek = line
				return ''
			if line == self.boundary + '--\n':
				self.peek = line
				return ''
		return line

class HeaderFile:
	def __init__(self, file):
		self.file = file
		self.peek = None

	def readline(self):
		if self.peek is not None:
			line = self.peek
			self.peek = None
		else:
			line = self.file.readline()
		if not line:
			return line
		if he.match(line) >= 0:
			return line
		while 1:
			self.peek = self.file.readline()
			if len(self.peek) == 0 or \
			   (self.peek[0] != ' ' and self.peek[0] != '\t'):
				return line
			line = line + self.peek
			self.peek = None

def mime_decode(line):
	'''Decode a single line of quoted-printable text to 8bit.'''
	newline = ''
	while 1:
		i = mime_code.search(line)
		if i < 0:
			break
		newline = newline + line[:i] + \
			  chr(string.atoi(mime_code.group(1), 16))
		line = line[i+3:]
	return newline + line

def mime_decode_header(line):
	'''Decode a header line to 8bit.'''
	newline = ''
	while 1:
		i = mime_head.search(line)
		if i < 0:
			break
		match0, match1 = mime_head.group(0, 1)
		# convert underscores to spaces (before =XX conversion!)
		match1 = string.join(string.split(match1, '_'), ' ')
		newline = newline + line[:i] + mime_decode(match1)
		line = line[i + len(match0):]
	return newline + line

def unmimify_part(ifile, ofile, decode_base64 = 0):
	'''Convert a quoted-printable part of a MIME mail message to 8bit.'''
	multipart = None
	quoted_printable = 0
	is_base64 = 0
	is_repl = 0
	if ifile.boundary and ifile.boundary[:2] == QUOTE:
		prefix = QUOTE
	else:
		prefix = ''

	# read header
	hfile = HeaderFile(ifile)
	while 1:
		line = hfile.readline()
		if not line:
			return
		if prefix and line[:len(prefix)] == prefix:
			line = line[len(prefix):]
			pref = prefix
		else:
			pref = ''
		line = mime_decode_header(line)
		if qp.match(line) >= 0:
			quoted_printable = 1
			continue	# skip this header
		if decode_base64 and base64_re.match(line) >= 0:
			is_base64 = 1
			continue
		ofile.write(pref + line)
		if not prefix and repl.match(line) >= 0:
			# we're dealing with a reply message
			is_repl = 1
		if mp.match(line) >= 0:
			multipart = '--' + mp.group(1)
		if he.match(line) >= 0:
			break
	if is_repl and (quoted_printable or multipart):
		is_repl = 0

	# read body
	while 1:
		line = ifile.readline()
		if not line:
			return
		line = regsub.gsub(mime_head, '\\1', line)
		if prefix and line[:len(prefix)] == prefix:
			line = line[len(prefix):]
			pref = prefix
		else:
			pref = ''
##		if is_repl and len(line) >= 4 and line[:4] == QUOTE+'--' and line[-3:] != '--\n':
##			multipart = line[:-1]
		while multipart:
			if line == multipart + '--\n':
				ofile.write(pref + line)
				multipart = None
				line = None
				break
			if line == multipart + '\n':
				ofile.write(pref + line)
				nifile = File(ifile, multipart)
				unmimify_part(nifile, ofile, decode_base64)
				line = nifile.peek
				continue
			# not a boundary between parts
			break
		if line and quoted_printable:
			while line[-2:] == '=\n':
				line = line[:-2]
				newline = ifile.readline()
				if newline[:len(QUOTE)] == QUOTE:
					newline = newline[len(QUOTE):]
				line = line + newline
			line = mime_decode(line)
		if line and is_base64 and not pref:
			import base64
			line = base64.decodestring(line)
		if line:
			ofile.write(pref + line)

def unmimify(infile, outfile, decode_base64 = 0):
	'''Convert quoted-printable parts of a MIME mail message to 8bit.'''
	if type(infile) == type(''):
		ifile = open(infile)
		if type(outfile) == type('') and infile == outfile:
			import os
			d, f = os.path.split(infile)
			os.rename(infile, os.path.join(d, ',' + f))
	else:
		ifile = infile
	if type(outfile) == type(''):
		ofile = open(outfile, 'w')
	else:
		ofile = outfile
	nifile = File(ifile, None)
	unmimify_part(nifile, ofile, decode_base64)
	ofile.flush()

mime_char = regex.compile('[=\240-\377]') # quote these chars in body
mime_header_char = regex.compile('[=?\240-\377]') # quote these in header

def mime_encode(line, header):
	'''Code a single line as quoted-printable.
	   If header is set, quote some extra characters.'''
	if header:
		reg = mime_header_char
	else:
		reg = mime_char
	newline = ''
	if len(line) >= 5 and line[:5] == 'From ':
		# quote 'From ' at the start of a line for stupid mailers
		newline = string.upper('=%02x' % ord('F'))
		line = line[1:]
	while 1:
		i = reg.search(line)
		if i < 0:
			break
		newline = newline + line[:i] + \
			  string.upper('=%02x' % ord(line[i]))
		line = line[i+1:]
	line = newline + line

	newline = ''
	while len(line) >= 75:
		i = 73
		while line[i] == '=' or line[i-1] == '=':
			i = i - 1
		i = i + 1
		newline = newline + line[:i] + '=\n'
		line = line[i:]
	return newline + line

mime_header = regex.compile('\\([ \t(]\\|^\\)\\([-a-zA-Z0-9_+]*[\240-\377][-a-zA-Z0-9_+\240-\377]*\\)\\([ \t)]\\|$\\)')

def mime_encode_header(line):
	'''Code a single header line as quoted-printable.'''
	newline = ''
	while 1:
		i = mime_header.search(line)
		if i < 0:
			break
		newline = newline + line[:i] + mime_header.group(1) + \
			  '=?' + CHARSET + '?Q?' + \
			  mime_encode(mime_header.group(2), 1) + \
			  '?=' + mime_header.group(3)
		line = line[i+len(mime_header.group(0)):]
	return newline + line

mv = regex.compile('^mime-version:', regex.casefold)
cte = regex.compile('^content-transfer-encoding:', regex.casefold)
iso_char = regex.compile('[\240-\377]')

def mimify_part(ifile, ofile, is_mime):
	'''Convert an 8bit part of a MIME mail message to quoted-printable.'''
	has_cte = is_qp = is_base64 = 0
	multipart = None
	must_quote_body = must_quote_header = has_iso_chars = 0

	header = []
	header_end = ''
	message = []
	message_end = ''
	# read header
	hfile = HeaderFile(ifile)
	while 1:
		line = hfile.readline()
		if not line:
			break
		if not must_quote_header and iso_char.search(line) >= 0:
			must_quote_header = 1
		if mv.match(line) >= 0:
			is_mime = 1
		if cte.match(line) >= 0:
			has_cte = 1
			if qp.match(line) >= 0:
				is_qp = 1
			elif base64_re.match(line) >= 0:
				is_base64 = 1
		if mp.match(line) >= 0:
			multipart = '--' + mp.group(1)
		if he.match(line) >= 0:
			header_end = line
			break
		header.append(line)

	# read body
	while 1:
		line = ifile.readline()
		if not line:
			break
		if multipart:
			if line == multipart + '--\n':
				message_end = line
				break
			if line == multipart + '\n':
				message_end = line
				break
		if is_base64:
			message.append(line)
			continue
		if is_qp:
			while line[-2:] == '=\n':
				line = line[:-2]
				newline = ifile.readline()
				if newline[:len(QUOTE)] == QUOTE:
					newline = newline[len(QUOTE):]
				line = line + newline
			line = mime_decode(line)
		message.append(line)
		if not has_iso_chars:
			if iso_char.search(line) >= 0:
				has_iso_chars = must_quote_body = 1
		if not must_quote_body:
			if len(line) > MAXLEN:
				must_quote_body = 1

	# convert and output header and body
	for line in header:
		if must_quote_header:
			line = mime_encode_header(line)
		if chrset.match(line) >= 0:
			if has_iso_chars:
				# change us-ascii into iso-8859-1
				if string.lower(chrset.group(2)) == 'us-ascii':
					line = chrset.group(1) + \
					       CHARSET + chrset.group(3)
			else:
				# change iso-8859-* into us-ascii
				line = chrset.group(1) + 'us-ascii' + chrset.group(3)
		if has_cte and cte.match(line) >= 0:
			line = 'Content-Transfer-Encoding: '
			if is_base64:
				line = line + 'base64\n'
			elif must_quote_body:
				line = line + 'quoted-printable\n'
			else:
				line = line + '7bit\n'
		ofile.write(line)
	if (must_quote_header or must_quote_body) and not is_mime:
		ofile.write('Mime-Version: 1.0\n')
		ofile.write('Content-Type: text/plain; ')
		if has_iso_chars:
			ofile.write('charset="%s"\n' % CHARSET)
		else:
			ofile.write('charset="us-ascii"\n')
	if must_quote_body and not has_cte:
		ofile.write('Content-Transfer-Encoding: quoted-printable\n')
	ofile.write(header_end)

	for line in message:
		if must_quote_body:
			line = mime_encode(line, 0)
		ofile.write(line)
	ofile.write(message_end)

	line = message_end
	while multipart:
		if line == multipart + '--\n':
			# read bit after the end of the last part
			while 1:
				line = ifile.readline()
				if not line:
					return
				if must_quote_body:
					line = mime_encode(line, 0)
				ofile.write(line)
		if line == multipart + '\n':
			nifile = File(ifile, multipart)
			mimify_part(nifile, ofile, 1)
			line = nifile.peek
			ofile.write(line)
			continue

def mimify(infile, outfile):
	'''Convert 8bit parts of a MIME mail message to quoted-printable.'''
	if type(infile) == type(''):
		ifile = open(infile)
		if type(outfile) == type('') and infile == outfile:
			import os
			d, f = os.path.split(infile)
			os.rename(infile, os.path.join(d, ',' + f))
	else:
		ifile = infile
	if type(outfile) == type(''):
		ofile = open(outfile, 'w')
	else:
		ofile = outfile
	nifile = File(ifile, None)
	mimify_part(nifile, ofile, 0)
	ofile.flush()

import sys
if __name__ == '__main__' or (len(sys.argv) > 0 and sys.argv[0] == 'mimify'):
	import getopt
	usage = 'Usage: mimify [-l len] -[ed] [infile [outfile]]'

	decode_base64 = 0
	opts, args = getopt.getopt(sys.argv[1:], 'l:edb')
	if len(args) not in (0, 1, 2):
		print usage
		sys.exit(1)
	if (('-e', '') in opts) == (('-d', '') in opts) or \
	   ((('-b', '') in opts) and (('-d', '') not in opts)):
		print usage
		sys.exit(1)
	for o, a in opts:
		if o == '-e':
			encode = mimify
		elif o == '-d':
			encode = unmimify
		elif o == '-l':
			try:
				MAXLEN = string.atoi(a)
			except:
				print usage
				sys.exit(1)
		elif o == '-b':
			decode_base64 = 1
	if len(args) == 0:
		encode_args = (sys.stdin, sys.stdout)
	elif len(args) == 1:
		encode_args = (args[0], sys.stdout)
	else:
		encode_args = (args[0], args[1])
	if decode_base64:
		encode_args = encode_args + (decode_base64,)
	apply(encode, encode_args)
