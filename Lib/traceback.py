# Format and print Python stack traces

import linecache
import string
import sys

def print_tb(tb, limit = None):
	if limit is None:
		if hasattr(sys, 'tracebacklimit'):
			limit = sys.tracebacklimit
	n = 0
	while tb is not None and (limit is None or n < limit):
		f = tb.tb_frame
		lineno = tb.tb_lineno
		co = f.f_code
		filename = co.co_filename
		name = co.co_name
		print '  File "%s", line %d, in %s' % (filename, lineno, name)
		line = linecache.getline(filename, lineno)
		if line: print '    ' + string.strip(line)
		tb = tb.tb_next
		n = n+1

def extract_tb(tb, limit = None):
	if limit is None:
		if hasattr(sys, 'tracebacklimit'):
			limit = sys.tracebacklimit
	list = []
	n = 0
	while tb is not None and (limit is None or n < limit):
		f = tb.tb_frame
		lineno = tb.tb_lineno
		co = f.f_code
		filename = co.co_filename
		name = co.co_name
		line = linecache.getline(filename, lineno)
		if line: line = string.strip(line)
		else: line = None
		list.append(filename, lineno, name, line)
		tb = tb.tb_next
		n = n+1
	return list

def print_exception(type, value, tb, limit = None):
	if tb:
		print 'Traceback (innermost last):'
		print_tb(tb, limit)
	if value is None:
		print type
	else:
		if type is SyntaxError:
			try:
				msg, (filename, lineno, offset, line) = value
			except:
				pass
			else:
				if not filename: filename = "<string>"
				print '  File "%s", line %d' % (filename, lineno)
				i = 0
				while i < len(line) and line[i] in string.whitespace:
					i = i+1
				s = '    '
				print s + string.strip(line)
				for c in line[i:offset-1]:
					if c in string.whitespace:
						s = s + c
					else:
						s = s + ' '
				print s + '^'
				value = msg
		print '%s: %s' % (type, value)

def print_exc(limit = None):
	print_exception(sys.exc_type, sys.exc_value, sys.exc_traceback,
			limit)

def print_last(limit = None):
	print_exception(sys.last_type, sys.last_value, sys.last_traceback,
			limit)
