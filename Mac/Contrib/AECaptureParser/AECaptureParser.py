'''
AECaptureParser makes a brave attempt to convert the text output
of the very handy Lasso Capture AE control panel
into close-enough executable python code.

In a roundabout way AECaptureParser offers the way to write lines of AppleScript
and convert them to python code. Once Pythonised, the code can be made prettier,
and it can run without Capture or Script Editor being open.

You need Lasso Capture AE from Blueworld:
ftp://ftp.blueworld.com/Lasso251/LassoCaptureAE.hqx

Lasso Capture AE prints structured ascii representations in a small window.
As these transcripts can be very complex, cut and paste to AECaptureParser, it parses and writes
python code that will, when executed, cause the same events to happen.
It's been tested with some household variety events, I'm sure there will be tons that
don't work.

All objects are converted to standard aetypes.ObjectSpecifier instances.

How to use:
	1. Start the Capture window
	2. Cause the desired appleevent to happen
		- by writing a line of applescript in Script Editor and running it (!)
		- by recording some action in Script Editor and running it
	3. Find the events in Capture:
		- make sure you get the appropriate events, cull if necessary
		- sometimes Capture barfs, just quit and start Capture again, run events again
		- AECaptureParser can process multiple events - it will just make more code.
	4. Copy and paste in this script and execute
	5. It will print python code that, when executed recreates the events.

Example:
	For instance the following line of AppleScript in Script Editor
			tell application "Finder"
				return application processes
			end tell
	will result in the following transcript:
			[event: target="Finder", class=core, id=getd]
			'----':obj {form:indx, want:type(pcap), seld:abso(«616C6C20»), from:'null'()}
			[/event]
	Feed a string with this (and perhaps more) events to AECaptureParser
	
Some mysteries:
	* 	what is '&subj' - it is sent in an activate event:	&subj:'null'()
		The activate event works when this is left out. A possibility?
	*	needs to deal with embedded aliasses


'''
__version__ = '0.002'
__author__ = 'evb'


import string 

opentag = '{'
closetag = '}'



import aetools
import aetypes

class eventtalker(aetools.TalkTo):
	pass

def processes():
	'''Helper function to get the list of current processes and their creators
	This code was mostly written by AECaptureParser! It ain't pretty, but that's not python's fault!'''
	talker = eventtalker('MACS')
	_arguments = {}
	_attributes = {}
	p = []
	names = []
	creators = []
	results = []
	# first get the list of process names
	_arguments['----'] = aetypes.ObjectSpecifier(want=aetypes.Type('pcap'),
			form="indx", seld=aetypes.Unknown('abso', "all "), fr=None)
	_reply, _arguments, _attributes = talker.send('core', 'getd', _arguments, _attributes)
	if _arguments.has_key('errn'):
		raise aetools.Error, aetools.decodeerror(_arguments)
	if _arguments.has_key('----'):
		p = _arguments['----']
	for proc in p:
		names.append(proc.seld)
	# then get the list of process creators
	_arguments = {}
	_attributes = {}
	AEobject_00 = aetypes.ObjectSpecifier(want=aetypes.Type('pcap'), form="indx", seld=aetypes.Unknown('abso', "all "), fr=None)
	AEobject_01 = aetypes.ObjectSpecifier(want=aetypes.Type('prop'), form="prop", seld=aetypes.Type('fcrt'), fr=AEobject_00)
	_arguments['----'] = AEobject_01
	_reply, _arguments, _attributes = talker.send('core', 'getd', _arguments, _attributes)
	if _arguments.has_key('errn'):
		raise aetools.Error, aetools.decodeerror(_arguments)
	if _arguments.has_key('----'):
		p = _arguments['----']
	for proc in p:
		creators.append(proc.type)
	# then put the lists together
	for i in range(len(names)):
		results.append((names[i], creators[i]))
	return results

		
class AECaptureParser:
	'''convert a captured appleevent-description into executable python code'''
	def __init__(self, aetext):
		self.aetext = aetext
		self.events = []
		self.arguments = {}
		self.objectindex = 0
		self.varindex = 0
		self.currentevent =  {'variables':{}, 'arguments':{}, 'objects':{}}
		self.parse()
	
	def parse(self):
		self.lines = string.split(self.aetext, '\n')
		for l in self.lines:
			if l[:7] == '[event:':
				self.eventheader(l)
			elif l[:7] == '[/event':
				if len(self.currentevent)<>0:
					self.events.append(self.currentevent)
					self.currentevent = {'variables':{}, 'arguments':{}, 'objects':{}}
					self.objectindex = 0
			else:
				self.line(l)
	
	def line(self, value):
		'''interpret literals, variables, lists etc.'''
		# stuff in [  ], l ists
		varstart = string.find(value, '[')
		varstop = string.find(value, ']')
		if varstart <> -1 and varstop <> -1 and varstop>varstart:
			variable = value[varstart:varstop+1]
			name = 'aevar_'+string.zfill(self.varindex, 2)
			self.currentevent['variables'][name] = variable
			value = value[:varstart]+name+value[varstop+1:]
			self.varindex = self.varindex + 1
		# stuff in «  »
		# these are 'ordinal' descriptors of 4 letter codes, so translate
		varstart = string.find(value, '«')
		varstop = string.find(value, '»')
		if varstart <> -1 and varstop <> -1 and varstop>varstart:
			variable = value[varstart+1:varstop]
			t = ''
			for i in range(0, len(variable), 2):
				c = eval('0x'+variable[i : i+2])
				t = t + chr(c)
			
			name = 'aevar_'+string.zfill(self.varindex, 2)
			self.currentevent['variables'][name] = '"' + t + '"'
			value = value[:varstart]+name+value[varstop+1:]
			self.varindex = self.varindex + 1
		pos = string.find(value, ':')
		if pos==-1:return
		ok = 1
		while ok <> None:
			value, ok = self.parseobject(value)
		self.currentevent['arguments'].update(self.splitparts(value, ':'))
		
		# remove the &subj argument?
		if self.currentevent['arguments'].has_key('&subj'):
			del self.currentevent['arguments']['&subj']
			
		# check for arguments len(a) < 4, and pad with spaces
		for k in self.currentevent['arguments'].keys():
			if len(k)<4:
				newk = k + (4-len(k))*' '
				self.currentevent['arguments'][newk] = self.currentevent['arguments'][k]
				del self.currentevent['arguments'][k]

	def parseobject(self, obj):
		a, b = self.findtag(obj)
		stuff = None
		if a<>None and b<>None:
			stuff = obj[a:b]
			name = 'AEobject_'+string.zfill(self.objectindex, 2)
			self.currentevent['objects'][name] = self.splitparts(stuff, ':')
			obj = obj[:a-5] + name + obj[b+1:]
			self.objectindex = self.objectindex +1
		return obj, stuff
		
	def nextopen(self, pos, text):
		return string.find(text, opentag, pos)
		
	def nextclosed(self, pos, text):
		return string.find(text, closetag, pos)
	
	def nexttag(self, pos, text):
		start = self.nextopen(pos, text)
		stop = self.nextclosed(pos, text)
		if start == -1:
			if stop == -1:
				return -1, -1
			return 0, stop
		if start < stop and start<>-1:
			return 1, start
		else:
			return 0, stop
					
	def findtag(self, text):
		p = -1
		last = None,None
		while 1:
			kind, p = self.nexttag(p+1, text)
			if last[0]==1 and kind==0:
				return last[1]+len(opentag), p
			if (kind, p) == (-1, -1):
				break
			last=kind, p
		return None, None
	
	def splitparts(self, txt, splitter):
		res = {}
		parts = string.split(txt, ', ')
		for p in parts:
			pos = string.find(p, splitter)
			key = string.strip(p[:pos])
			value = string.strip(p[pos+1:])
			res[key] = self.map(value)
		return res
		
	def eventheader(self, hdr):
		self.currentevent['event'] = self.splitparts(hdr[7:-1], '=')
	
	def printobject(self, d):
		'''print one object as python code'''
		t = []
		obj = {}
		obj.update(d)
		t.append("aetypes.ObjectSpecifier(")
		if obj.has_key('want'):
			t.append('want=' + self.map(obj['want']))
			del obj['want']
			t.append(', ')
		if obj.has_key('form'):
			t.append('form=' + addquotes(self.map(obj['form'])))
			del obj['form']
			t.append(', ')
		if obj.has_key('seld'):
			t.append('seld=' + self.map(obj['seld']))
			del obj['seld']
			t.append(', ')
		if obj.has_key('from'):
			t.append('fr=' + self.map(obj['from']))
			del obj['from']
		if len(obj.keys()) > 0:
			print '# ', `obj`			
		t.append(")")
		return string.join(t, '')
	
	def map(self, t):
		'''map some Capture syntax to python
		matchstring : [(old, new), ... ]
		'''
		m = {
				'type(': [('type(', "aetypes.Type('"), (')', "')")],
				"'null'()": [("'null'()", "None")],
				'abso(': [('abso(', "aetypes.Unknown('abso', ")],
				'–':	[('–', '"')],
				'”':	[('”', '"')],
				'[':	[('[', '('), (', ', ',')],
				']':	[(']', ')')],
				'«':	[('«', "«")],
				'»':	[('»', "»")],
				
			}
		for k in m.keys():
			if string.find(t, k) <> -1:
				for old, new in m[k]:
					p = string.split(t, old)
					t = string.join(p, new)
		return t
		
	def printevent(self, i):
		'''print the entire captured sequence as python'''
		evt = self.events[i]
		code = []
		code.append('\n# start event ' + `i` + ', talking to ' + evt['event']['target'])
		# get the signature for the target application
		code.append('talker = eventtalker("'+self.gettarget(evt['event']['target'])+'")')
		code.append("_arguments = {}")
		code.append("_attributes = {}")
		# write the variables
		for key, value in evt['variables'].items():
			value = evt['variables'][key]
			code.append(key + ' = ' + value)
		# write the object in the right order
		objkeys = evt['objects'].keys()
		objkeys.sort()
		for key in objkeys:
			value = evt['objects'][key]
			code.append(key + ' = ' + self.printobject(value))
		# then write the arguments
		for key, value in evt['arguments'].items():
			code.append("_arguments[" + addquotes(key) + "] = " + value )
		code.append('_reply, _arguments, _attributes = talker.send("'+
				evt['event']['class']+'", "'+evt['event']['id']+'", _arguments, _attributes)')
		code.append("if _arguments.has_key('errn'):")
		code.append('\traise aetools.Error, aetools.decodeerror(_arguments)')	
		code.append("if _arguments.has_key('----'):")
		code.append("\tprint _arguments['----']")
		code.append('# end event ' + `i`)
		return string.join(code, '\n')
	
	def gettarget(self, target):
		'''get the signature for the target application'''
		target = target[1:-1]
		if target == 'Finder':
			return "MACS"
		apps = processes()
		for name, creator in apps:
			if name == target:
				return creator
		return '****'
			
	def makecode(self):
		code = []
		code.append("\n\n")
		code.append("# code generated by AECaptureParser v " + __version__)
		code.append("# imports, definitions for all events")
		code.append("import aetools")
		code.append("import aetypes")
		code.append("class eventtalker(aetools.TalkTo):")
		code.append("\tpass")
		code.append("# the events")
		# print the events
		for i in range(len(self.events)):
			code.append(self.printevent(i))
		code.append("# end code")
		return string.join(code, '\n')

def addquotes(txt):
	quotes = ['"', "'"]
	if not txt[0] in quotes and not txt[-1] in quotes:
		return '"'+txt+'"'
	return txt 
	
	




# ------------------------------------------
#	the factory
# ------------------------------------------

# for instance, this event was captured from the Script Editor asking the Finder for a list of active processes.

eventreceptacle = """

[event: target="Finder", class=core, id=setd]
'----':obj {form:prop, want:type(prop), seld:type(posn), from:obj {form:name, want:type(cfol), seld:–MoPar:Data:DevDev:Python:Python 1.5.2c1:Extensions”, from:'null'()}}, data:[100, 10]
[/event]

"""

aet = AECaptureParser(eventreceptacle)
print aet.makecode()
