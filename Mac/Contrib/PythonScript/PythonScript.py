"""
Python script a module to comunicate with apple events

v 0.1a2
v.0.2 16 april 1998 


"""
import sys
import getaete
import baetools
import baetypes
import AE
import AppleEvents
import macfs
from types import *
#from aetypes import InstanceType
from aepack import AEDescType

ordinal = {
'every': 'all ',
'first' : 'firs',
'last' : 'last',
'any' : 'any ',
'middle' : 'midd'}


Error = 'PythonScript.Error'


class PsEvents:
	pass


class PsClasses:

	
	def __getattr__(self, name):
		try:
			return DCItem(name, self)
		except:
			pass
		
	def __repr__(self):
		if self.form != 'prop':
			t = type(self.seld)
			if t == StringType:
				self.form = 'name'
			elif baetypes.IsRange(self.seld):
				self.form = 'rang'
			elif baetypes.IsComparison(self.seld) or baetypes.IsLogical(self.seld):
				self.form = 'test'
			elif t == TupleType:
			# Breakout: specify both form and seld in a tuple
			# (if you want ID or rele or somesuch)
				self.form, self.seld = self.seld
			elif t == IntType:
				self.form = 'indx'
			else:
				pass

		if self.seld in ordinal.keys():
			self.seld = baetypes.Ordinal(ordinal[self.seld])
			self.form = 'indx'

		s = "baetypes.ObjectSpecifier(%s, %s, %s" % (`self.want`, `self.form`, `self.seld`)
		if `self.fr`:
			s = s + ", %s)" % `self.fr`
		else:
			s = s + ")"
		return s
		
	def __str__(self):
		return self.want
		
		
def template(self, seld=None, fr=None):
	self.seld = seld
	self.fr = fr
		
def template1(self, which, fr=None):
	self.want = 'prop'
	self.form = 'prop'
	self.fr = fr
	
class DCItem:
	def __init__(self, comp, fr):
		self.compclass = comp
		self.fr = fr
		
	def __call__(self, which=None):
		if which:
			self.compclass = eval('PsClass.%s' % self.compclass)
		else:
			try:
				self.compclass = eval('PsProperties.%s' % self.compclass)
			except AttributeError:
				self.compclass = eval('PsClass.%s' % self.compclass)
		return self.compclass(which, self.fr)

class PsClass:
	pass

class PsProperties:
	pass				


class PsEnumerations:
	pass

			
def PsScript(sig=None, Timeout=0, Ignoring=0):
	elements = {}
	if sig:
		target, sig = Signature(sig)
		pyscript = getaete.Getaete(sig)
	else:
		target, sig = Signature('Pyth')
		pyscript = getaete.Getaete()
	setattr(PyScript, 'timeout', Timeout)
	setattr(PyScript, 'ignoring', Ignoring)
	setattr(PyScript, 'target', target)
	for key, value in pyscript[0].items():
		setattr(PsEvents, key, value)
	for key, value in pyscript[1].items():
		CreateClass(key, 'PsClasses', value)
		for val in value[2]:
			CreateProperty(val[0], 'PsClasses', `val[1]`)
			
		if value[3]:
			for val in value[3]:
				if val[0] not in elements.keys():
					elements[val[0]] = val[1]
				elif len(val[1]) > len(elements[val[0]]):
					elements[val[0]] = val[1]

	for key, value in pyscript[2].items():
		for val in value:
			setattr(PsEnumerations, val[0], val[1])

def CreateClass(newClassName, superClassName, value):
	parentDict = PsClass.__dict__
	exec "class %s(%s): pass" % (newClassName, superClassName) in \
			globals(), parentDict
	newClassObj = parentDict[newClassName]
	newClassObj.__init__ = template
	exec "setattr(newClassObj, 'want', %s)" % `value[0]`
	if value[2] and value[2][0][0] == 'every':
		exec "setattr(newClassObj, 'plur', 1)"

def CreateProperty(newClassName, superClassName, value):
	parentDict = PsProperties.__dict__
	exec "class %s(%s): pass" % (newClassName, superClassName) in \
			globals(), parentDict
	newClassObj = parentDict[newClassName]
	if newClassName == 'Every':
		value = "baetypes.mkOrdinal('every')"
	newClassObj.__init__ = template1
	exec "setattr(newClassObj, 'seld', %s)" % value

def Signature(signature):
	if type(signature) == AEDescType:
		target = signature
	elif type(signature) == InstanceType and hasattr(signature, '__aepack__'):
		target = signature.__aepack__()
	elif type(signature) == StringType:
		if len(signature) == 4:
			target = AE.AECreateDesc(AppleEvents.typeApplSignature, signature)
			target_signature = signature
		else:
			#This should ready be made persistant, so PythonScript 'remembered' where applications were
			fss, ok = macfs.PromptGetFile('Find the aplication %s' % signature, 'APPL')
			if ok:
				target_signature = fss.GetCreatorType()[0]
				target = AE.AECreateDesc(AppleEvents.typeApplSignature, target_signature)
	else:
		raise TypeError, "signature should be 4-char string or AEDesc"
	return target, target_signature

		

		
class PyScript(PsEvents):
	def __init__(self, name, obj=None,  **args):
		desc, code, subcode, rply, message, keywds = name
#		print 'code', code
#		print 'subcode', subcode
#		print 'rply', rply
#		print 'message', message
#		print 'keywds', keywds
#		print 'name', name
#		print 'obj', obj
#		print 'args', args
		self.code = code
		self.subcode = subcode
		self.attributes ={}
		self.arguments = {}
		if keywds:
			self.arguments = self.keyargs(keywds, args)
		self.arguments['----'] = self.keyfms(message[0], obj)

		##XXXX Eudora needs this XXXX##
		if self.arguments['----'] == None:
			del self.arguments['----']
#		print 'arguments', self.arguments
		if self.ignoring or rply[0] == 'null':
			self.send_flags = AppleEvents.kAENoReply
		else:
			self.send_flags = AppleEvents.kAEWaitReply
		self.send_priority = AppleEvents.kAENormalPriority	
		if self.timeout:
			self.send_timeout = self.timeout
		else:
			self.send_timeout = AppleEvents.kAEDefaultTimeout


	def keyargs(self, ats, args):
#		print 'keyargs', ats, args
		output = {}
		for arg in args.keys():
			for at in ats:
				if at[0] == arg:
					output[at[1]] = self.keyfms(at[2][0], args[arg])
		return output
				
	def keyfms(self, key, value):
#		print 'keyfms', 'key', key, `value`
		if key == 'obj ' or key == 'insl':
			return eval(`value`)
		elif key == 'TEXT':
			return value
		elif key == 'null':
			return 
		elif key == 'bool':
			return baetypes.mkboolean(value)
		elif key == 'type':
			try:
				val = eval('PsClass.%s()' % value)
				return baetypes.mktype(str(val))
			except:
				return baetypes.mktype(value)
		else:
			print "I don't know what to put here -- script.keyargs"
			print key, `value`
			sys.exit[1]
		
	def newevent(self, code, subcode, parameters = {}, attributes = {}):
		"""Create a complete structure for an apple event"""
#		print code, subcode, parameters, attributes
		event = AE.AECreateAppleEvent(code, subcode, self.target,
			  	  AppleEvents.kAutoGenerateReturnID, AppleEvents.kAnyTransactionID)
		baetools.packevent(event, parameters, attributes)
		return event
		
	def sendevent(self, event):
		"""Send a pre-created appleevent, await the reply and unpack it"""
		
		reply = event.AESend(self.send_flags, self.send_priority,
							  self.send_timeout)
		parameters, attributes = baetools.unpackevent(reply)
		return reply, parameters, attributes
		
	def send(self, code, subcode, parameters = {}, attributes = {}):
		"""Send an appleevent given code/subcode/pars/attrs and unpack the reply"""
#		print code, subcode, parameters, attributes
		return self.sendevent(self.newevent(code, subcode, parameters, attributes))
		
	def __str__(self):
		_reply, _arguments, _attributes = self.send(self.code, self.subcode, self.arguments, self.attributes)

		if _arguments.has_key('errn'):
			raise baetools.Error, baetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return str(_arguments['----'])
		else:
			return 

	
	
def test():
	Simp = 'Hermit:Applications:SimpleText'
	PsScript('MACS', Timeout=60*60*3)
#	PsScript('CSOm', Timeout=60*60*3)
#	PsScript('', Timeout=60*60*3)
#	PyScript('macsoup')
	ev = PsEvents
	ps = PsClass
#	print PsProperties.__dict__
#	y = script(ev.Open, File('Hermit:Desktop Folder:Lincolnshire Imp'), using=Application_file(Simp))
#	print baetypes.NProperty('prop', 'prop', 'pnam',  baetypes.ObjectSpecifier('cdis', 'indx', 1, None))
#	y = PyScript(ev.Get, Disk("Hermit").Folder(7).File(1).Name())
#	y = PyScript(ev.Get, Disk("Hermit").Size(), As='Integer')
#	y = PyScript(ev.Get, ps.Desktopobject(1).Startup_disk())
#	y = PyScript(ev.Get, Mailbox(1).File(), as='TEXT')
#	print 'y', y, type(y)

if __name__ == '__main__':
	test()
#	sys.exit(1)
	
