# Format and print Python stack traces

import linecache
import string
import sys
import types

def _print(file, str='', terminator='\n'):
	file.write(str+terminator)


def print_list(extracted_list, file=None):
	if not file:
		file = sys.stderr
	for filename, lineno, name, line in extracted_list:
		_print(file,
		       '  File "%s", line %d, in %s' % (filename,lineno,name))
		if line:
			_print(file, '    %s' % string.strip(line))

def format_list(extracted_list):
	list = []
	for filename, lineno, name, line in extracted_list:
		item = '  File "%s", line %d, in %s\n' % (filename,lineno,name)
		if line:
			item = item + '    %s\n' % string.strip(line)
		list.append(item)
	return list
	

def print_tb(tb, limit=None, file=None):
	if not file:
		file = sys.stderr
	if limit is None:
		if hasattr(sys, 'tracebacklimit'):
			limit = sys.tracebacklimit
	n = 0
	while tb is not None and (limit is None or n < limit):
		f = tb.tb_frame
		lineno = tb_lineno(tb)
		co = f.f_code
		filename = co.co_filename
		name = co.co_name
		_print(file,
		       '  File "%s", line %d, in %s' % (filename,lineno,name))
		line = linecache.getline(filename, lineno)
		if line: _print(file, '    ' + string.strip(line))
		tb = tb.tb_next
		n = n+1

def format_tb(tb, limit = None):
	return format_list(extract_tb(tb, limit))

def extract_tb(tb, limit = None):
	if limit is None:
		if hasattr(sys, 'tracebacklimit'):
			limit = sys.tracebacklimit
	list = []
	n = 0
	while tb is not None and (limit is None or n < limit):
		f = tb.tb_frame
		lineno = tb_lineno(tb)
		co = f.f_code
		filename = co.co_filename
		name = co.co_name
		line = linecache.getline(filename, lineno)
		if line: line = string.strip(line)
		else: line = None
		list.append((filename, lineno, name, line))
		tb = tb.tb_next
		n = n+1
	return list


def print_exception(etype, value, tb, limit=None, file=None):
	if not file:
		file = sys.stderr
	if tb:
		_print(file, 'Traceback (innermost last):')
		print_tb(tb, limit, file)
	lines = format_exception_only(etype, value)
	for line in lines[:-1]:
		_print(file, line, ' ')
	_print(file, lines[-1], '')

def format_exception(etype, value, tb, limit = None):
	if tb:
		list = ['Traceback (innermost last):\n']
		list = list + format_tb(tb, limit)
	list = list + format_exception_only(etype, value)
	return list

def format_exception_only(etype, value):
	list = []
	if type(etype) == types.ClassType:
		stype = etype.__name__
	else:
		stype = etype
	if value is None:
		list.append(str(stype) + '\n')
	else:
		if etype is SyntaxError:
			try:
				msg, (filename, lineno, offset, line) = value
			except:
				pass
			else:
				if not filename: filename = "<string>"
				list.append('  File "%s", line %d\n' %
					    (filename, lineno))
				i = 0
				while i < len(line) and \
				      line[i] in string.whitespace:
					i = i+1
				list.append('    %s\n' % string.strip(line))
				s = '    '
				for c in line[i:offset-1]:
					if c in string.whitespace:
						s = s + c
					else:
						s = s + ' '
				list.append('%s^\n' % s)
				value = msg
		list.append('%s: %s\n' % (str(stype), str(value)))
	return list


def print_exc(limit=None, file=None):
	if not file:
		file = sys.stderr
	try:
	    etype, value, tb = sys.exc_info()
	    print_exception(etype, value, tb, limit, file)
	finally:
	    etype = value = tb = None

def print_last(limit=None, file=None):
	if not file:
		file = sys.stderr
	print_exception(sys.last_type, sys.last_value, sys.last_traceback,
			limit, file)


def print_stack(f=None, limit=None, file=None):
	if f is None:
		try:
			raise ZeroDivisionError
		except ZeroDivisionError:
			f = sys.exc_info()[2].tb_frame.f_back
	print_list(extract_stack(f, limit), file)

def format_stack(f=None, limit=None):
	if f is None:
		try:
			raise ZeroDivisionError
		except ZeroDivisionError:
			f = sys.exc_info()[2].tb_frame.f_back
	return format_list(extract_stack(f, limit))

def extract_stack(f=None, limit = None):
	if f is None:
		try:
			raise ZeroDivisionError
		except ZeroDivisionError:
			f = sys.exc_info()[2].tb_frame.f_back
	if limit is None:
		if hasattr(sys, 'tracebacklimit'):
			limit = sys.tracebacklimit
	list = []
	n = 0
	while f is not None and (limit is None or n < limit):
		lineno = f.f_lineno	# XXX Too bad if -O is used
		co = f.f_code
		filename = co.co_filename
		name = co.co_name
		line = linecache.getline(filename, lineno)
		if line: line = string.strip(line)
		else: line = None
		list.append((filename, lineno, name, line))
		f = f.f_back
		n = n+1
	list.reverse()
	return list

# Calculate the correct line number of the traceback given in tb (even
# with -O on).
# Coded by Marc-Andre Lemburg from the example of PyCode_Addr2Line()
# in compile.c.

def tb_lineno(tb):
	f = tb.tb_frame
	try:
		c = f.f_code
		tab = c.co_lnotab
		line = c.co_firstlineno
		stopat = tb.tb_lasti
	except AttributeError:
		return f.f_lineno
	addr = 0
	for i in range(0, len(tab), 2):
		addr = addr + ord(tab[i])
		if addr > stopat:
			break
		line = line + ord(tab[i+1])
	return line
