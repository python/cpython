#!/usr/local/bin/python
#
# A class for wrapping the WWW Forms Common Gateway Interface (CGI) 
# Michael McLay, NIST  mclay@eeel.nist.gov  6/14/94
# 
# modified by Steve Majewski <sdm7g@Virginia.EDU> 12/5/94 
#

# Several classes to parse the name/value pairs that are passed to 
# a server's CGI by GET, POST or PUT methods by a WWW FORM. This 
# module is based on Mike McLay's original cgi.py after discussing
# changes with him and others on the comp.lang.python newsgroup, and
# at the NIST Python workshop. 
#
# The rationale for changes was:
#    The original FormContent class was almost, but not quite like
#    a dictionary object. Besides adding some extra access methods,
#    it had a values() method with different arguments and semantics
#    from the standard values() method of a mapping object. Also, 
#    it provided several different access methods that may be necessary
#    or useful, but made it a little more confusing to figure out how
#    to use. Also, we wanted to make the most typical cases the simplest
#    and most convenient access methods. ( Most form fields just return
#    a single value, and in practice, a lot of code was just assuming
#    a single value and ignoring all others. On the other hand, the 
#    protocol allows multiple values to be returned. 
#
#  The new base class (FormContentDict) is just like a dictionary.
#  In fact, if you just want a dictionary, all of the stuff that was
#  in __init__ has been extracted into a cgi.parse() function that will
#  return the "raw" dictionary, but having a class allows you to customize 
#  it further. 
#   Mike McLay's original FormContent class is reimplemented as a 
#  subclass of FormContentDict.
#   There are two additional sub-classes, but I'm not yet too sure 
#  whether they are what I want. 
# 

import string,regsub,sys,os,urllib
# since os.environ may often be used in cgi code, we name it in this module.
from os import environ


def parse():
	if environ['REQUEST_METHOD'] == 'POST':
		qs = sys.stdin.read(string.atoi(environ['CONTENT_LENGTH']))
		environ['QUERY_STRING'] = qs
	else:
		qs = environ['QUERY_STRING']
	name_value_pairs = string.splitfields(qs, '&')
	dict = {}
	for name_value in name_value_pairs:
		nv = string.splitfields(name_value, '=')
		if len(nv) != 2:
			continue
		name = nv[0]
		value = urllib.unquote(regsub.gsub('+',' ',nv[1]))
		if len(value):
			if dict.has_key (name):
				dict[name].append(value)
			else:
				dict[name] = [value]
	return dict



# The FormContent constructor creates a dictionary from the name/value pairs
# passed through the CGI interface.


#
#  form['key'] 
#  form.__getitem__('key') 
#  form.has_key('key')
#  form.keys()
#  form.values()
#  form.items()
#  form.dict

class FormContentDict:
	def __init__( self ):
		self.dict = parse()
		self.query_string = environ['QUERY_STRING']
	def __getitem__(self,key):
		return self.dict[key]
	def keys(self):
		return self.dict.keys()
	def has_key(self, key):
		return self.dict.has_key(key)
	def values(self):
		return self.dict.values()
	def items(self):
		return self.dict.items() 
	def __len__( self ):
		return len(self.dict)


# This is the "strict" single-value expecting version. 
# IF you only expect a single value for each field, then form[key]
# will return that single value ( the [0]-th ), and raise an 
# IndexError if that expectation is not true. 
# IF you expect a field to have possible multiple values, than you
# can use form.getlist( key ) to get all of the values. 
# values() and items() are a compromise: they return single strings
#  where there is a single value, and lists of strings otherwise. 

class SvFormContentDict(FormContentDict):
	def __getitem__( self, key ):
		if len( self.dict[key] ) > 1 : 
			raise IndexError, 'expecting a single value' 
		return self.dict[key][0]
	def getlist( self, key ):
		return self.dict[key]
	def values( self ):
		lis = []
		for each in self.dict.values() : 
			if len( each ) == 1 : 
				lis.append( each[0] )
			else: lis.append( each )
		return lis
	def items( self ):
		lis = []
		for key,value in self.dict.items():
			if len(value) == 1 :
				lis.append( (key,value[0]) )
			else:	lis.append( (key,value) )
		return lis


# And this sub-class is similar to the above, but it will attempt to 
# interpret numerical values. This is here as mostly as an example,
# but I think the real way to handle typed-data from a form may be
# to make an additional table driver parsing stage that has a table
# of allowed input patterns and the output conversion types - it 
# would signal type-errors on parse, not on access. 
class InterpFormContentDict(SvFormContentDict):
	def __getitem__( self, key ):
		v = SvFormContentDict.__getitem__( self, key )
		if v[0] in string.digits+'+-.' : 
			try:  return  string.atoi( v ) 
			except ValueError:
				try:	return string.atof( v )
				except ValueError: pass
		return string.strip(v)
	def values( self ):
		lis = [] 
		for key in self.keys():
			try:
				lis.append( self[key] )
			except IndexError:
				lis.append( self.dict[key] )
		return lis
	def items( self ):
		lis = [] 
		for key in self.keys():
			try:
				lis.append( (key, self[key]) )
			except IndexError:
				lis.append( (key, self.dict[key]) )
		return lis


# class FormContent parses the name/value pairs that are passed to a
# server's CGI by GET, POST, or PUT methods by a WWW FORM. several 
# specialized FormContent dictionary access methods have been added 
# for convenience.

# function                   return value
#
# form.keys()                     all keys in dictionary
# form.has_key('key')             test keys existance
# form[key]                       returns list associated with key
# form.values('key')              key's list (same as form.[key])
# form.indexed_value('key' index) nth element in key's value list
# form.value(key)                 key's unstripped value 
# form.length(key)                number of elements in key's list
# form.stripped(key)              key's value with whitespace stripped
# form.pars()                     full dictionary 



class FormContent(FormContentDict):
# This is the original FormContent semantics of values,
# not the dictionary like semantics. 
	def values(self,key):
		if self.dict.has_key(key):return self.dict[key]
		else: return None
	def indexed_value(self,key, location):
		if self.dict.has_key(key):
			if len (self.dict[key]) > location:
				return self.dict[key][location]
			else: return None
		else: return None
	def value(self,key):
		if self.dict.has_key(key):return self.dict[key][0]
		else: return None
	def length(self,key):
		return len (self.dict[key])
	def stripped(self,key):
		if self.dict.has_key(key):return string.strip(self.dict[key][0])
		else: return None
	def pars(self):
		return self.dict






def print_environ_usage():
	print """
<H3>These operating system environment variables could have been 
set:</H3> <UL>
<LI>AUTH_TYPE
<LI>CONTENT_LENGTH
<LI>CONTENT_TYPE
<LI>DATE_GMT
<LI>DATE_LOCAL
<LI>DOCUMENT_NAME
<LI>DOCUMENT_ROOT
<LI>DOCUMENT_URI
<LI>GATEWAY_INTERFACE
<LI>LAST_MODIFIED
<LI>PATH
<LI>PATH_INFO
<LI>PATH_TRANSLATED
<LI>QUERY_STRING
<LI>REMOTE_ADDR
<LI>REMOTE_HOST
<LI>REMOTE_IDENT
<LI>REMOTE_USER
<LI>REQUEST_METHOD
<LI>SCRIPT_NAME
<LI>SERVER_NAME
<LI>SERVER_PORT
<LI>SERVER_PROTOCOL
<LI>SERVER_ROOT
<LI>SERVER_SOFTWARE
</UL>
"""

def print_environ():
	skeys = environ.keys()
	skeys.sort()
	print '<h3> The following environment variables ' \
	      'were set by the CGI script: </h3>'
	print '<dl>'
	for key in skeys:
		print '<dt>', escape(key), '<dd>', escape(environ[key])
	print '</dl>' 

def print_form( form ):
	skeys = form.keys()
	skeys.sort()
	print '<h3> The following name/value pairs ' \
	      'were entered in the form: </h3>'
	print '<dl>'
	for key in skeys:
		print '<dt>', escape(key), ':',
		print '<i>', escape(`type(form[key])`), '</i>',
		print '<dd>', escape(form[key])
	print '</dl>'

def escape( s ):
	s = regsub.gsub('&', '&amp;') # Must be done first
	s = regsub.gsub('<', '&lt;')
	s = regsub.gsub('>', '&gt;')
	return s

def test( what ):
	label = escape(str(what))
	print 'Content-type: text/html\n\n'
	print '<HEADER>\n<TITLE>' + label + '</TITLE>\n</HEADER>\n'
	print '<BODY>\n' 
	print "<H1>" + label +"</H1>\n"
	form = what()
	print_form( form )
	print_environ()
	print_environ_usage() 
	print '</body>'

if __name__ == '__main__' : 
	test_classes = ( FormContent, FormContentDict, SvFormContentDict, InterpFormContentDict )
	test( test_classes[0] )	# by default, test compatibility with 
				# old version, change index to test others.
