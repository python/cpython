"""\
Pickling Algorithm
------------------

This module implements a basic but powerful algorithm for "pickling" (a.k.a.
serializing, marshalling or flattening) nearly arbitrary Python objects.
This is a more primitive notion than persistency -- although pickle
reads and writes file objects, it does not handle the issue of naming
persistent objects, nor the (even more complicated) area of concurrent
access to persistent objects.  The pickle module can transform a complex
object into a byte stream and it can transform the byte stream into
an object with the same internal structure.  The most obvious thing to
do with these byte streams is to write them onto a file, but it is also
conceivable to send them across a network or store them in a database.

Unlike the built-in marshal module, pickle handles the following correctly:

- recursive objects
- pointer sharing
- classes and class instances

Pickle is Python-specific.  This has the advantage that there are no
restrictions imposed by external standards such as CORBA (which probably
can't represent pointer sharing or recursive objects); however it means
that non-Python programs may not be able to reconstruct pickled Python
objects.

Pickle uses a printable ASCII representation.  This is slightly more
voluminous than a binary representation.  However, small integers actually
take *less* space when represented as minimal-size decimal strings than
when represented as 32-bit binary numbers, and strings are only much longer
if they contain control characters or 8-bit characters.  The big advantage
of using printable ASCII (and of some other characteristics of pickle's
representation) is that for debugging or recovery purposes it is possible
for a human to read the pickled file with a standard text editor.  (I could
have gone a step further and used a notation like S-expressions, but the
parser would have been considerably more complicated and slower, and the
files would probably have become much larger.)

Pickle doesn't handle code objects, which marshal does.
I suppose pickle could, and maybe it should, but there's probably no
great need for it right now (as long as marshal continues to be used
for reading and writing code objects), and at least this avoids
the possibility of smuggling Trojan horses into a program.

For the benefit of persistency modules written using pickle, it supports
the notion of a reference to an object outside the pickled data stream.
Such objects are referenced by a name, which is an arbitrary string of
printable ASCII characters.  The resolution of such names is not defined
by the pickle module -- the persistent object module will have to implement
a method "persistent_load".  To write references to persistent objects,
the persistent module must define a method "persistent_id" which returns
either None or the persistent ID of the object.

There are some restrictions on the pickling of class instances.

First of all, the class must be defined at the top level in a module.

Next, it must normally be possible to create class instances by calling
the class without arguments.  If this is undesirable, the class can
define a method __getinitargs__ (XXX not a pretty name!), which should
return a *tuple* containing the arguments to be passed to the class
constructor.

Classes can influence how their instances are pickled -- if the class defines
the method __getstate__, it is called and the return state is pickled
as the contents for the instance, and if the class defines the
method __setstate__, it is called with the unpickled state.  (Note
that these methods can also be used to implement copying class instances.)
If there is no __getstate__ method, the instance's __dict__
is pickled.  If there is no __setstate__ method, the pickled object
must be a dictionary and its items are assigned to the new instance's
dictionary.  (If a class defines both __getstate__ and __setstate__,
the state object needn't be a dictionary -- these methods can do what they
want.)

Note that when class instances are pickled, their class's code and data
is not pickled along with them.  Only the instance data is pickled.
This is done on purpose, so you can fix bugs in a class or add methods and
still load objects that were created with an earlier version of the
class.  If you plan to have long-lived objects that will see many versions
of a class, it may be worth to put a version number in the objects so
that suitable conversions can be made by the class's __setstate__ method.

The interface is as follows:

To pickle an object x onto a file f, open for writing:

	p = pickle.Pickler(f)
	p.dump(x)

To unpickle an object x from a file f, open for reading:

	u = pickle.Unpickler(f)
	x = u.load()

The Pickler class only calls the method f.write with a string argument
(XXX possibly the interface should pass f.write instead of f).
The Unpickler calls the methods f.read(with an integer argument)
and f.readline(without argument), both returning a string.
It is explicitly allowed to pass non-file objects here, as long as they
have the right methods.

The following types can be pickled:

- None
- integers, long integers, floating point numbers
- strings
- tuples, lists and dictionaries containing only picklable objects
- class instances whose __dict__ or __setstate__() is picklable
- classes

Attempts to pickle unpicklable objects will raise an exception
after having written an unspecified number of bytes to the file argument.

It is possible to make multiple calls to Pickler.dump() or to
Unpickler.load(), as long as there is a one-to-one correspondence
between pickler and Unpickler objects and between dump and load calls
for any pair of corresponding Pickler and Unpicklers.  WARNING: this
is intended for pickleing multiple objects without intervening modifications
to the objects or their parts.  If you modify an object and then pickle
it again using the same Pickler instance, the object is not pickled
again -- a reference to it is pickled and the Unpickler will return
the old value, not the modified one.  (XXX There are two problems here:
(a) detecting changes, and (b) marshalling a minimal set of changes.
I have no answers.  Garbage Collection may also become a problem here.)
"""

__version__ = "1.5"			# Code version

from types import *
import string

format_version = "1.1"			# File format version we write
compatible_formats = ["1.0"]		# Old format versions we can read

PicklingError = "pickle.PicklingError"

AtomicTypes = [NoneType, IntType, FloatType, StringType]

def safe(object):
	t = type(object)
	if t in AtomicTypes:
		return 1
	if t is TupleType:
		for item in object:
			if not safe(item): return 0
		return 1
	return 0

MARK = '('
POP = '0'
DUP = '2'
STOP = '.'
TUPLE = 't'
LIST = 'l'
DICT = 'd'
INST = 'i'
CLASS = 'c'
GET = 'g'
PUT = 'p'
APPEND = 'a'
SETITEM = 's'
BUILD = 'b'
NONE = 'N'
INT = 'I'
LONG = 'L'
FLOAT = 'F'
STRING = 'S'
PERSID = 'P'
AtomicKeys = [NONE, INT, LONG, FLOAT, STRING]
AtomicMap = {
	NoneType: NONE,
	IntType: INT,
	LongType: LONG,
	FloatType: FLOAT,
	StringType: STRING,
}

class Pickler:

	def __init__(self, file):
		self.write = file.write
		self.memo = {}

	def dump(self, object):
		self.save(object)
		self.write(STOP)

	def save(self, object):
		pid = self.persistent_id(object)
		if pid:
			self.write(PERSID + str(pid) + '\n')
			return
		d = id(object)
		if self.memo.has_key(d):
			self.write(GET + `d` + '\n')
			return
		t = type(object)
		try:
			f = self.dispatch[t]
		except KeyError:
			raise PicklingError, \
			      "can't pickle %s objects" % `t.__name__`
		f(self, object)

	def persistent_id(self, object):
		return None

	dispatch = {}

	def save_none(self, object):
		self.write(NONE)
	dispatch[NoneType] = save_none

	def save_int(self, object):
		self.write(INT + `object` + '\n')
	dispatch[IntType] = save_int

	def save_long(self, object):
		self.write(LONG + `object` + '\n')
	dispatch[LongType] = save_long

	def save_float(self, object):
		self.write(FLOAT + `object` + '\n')
	dispatch[FloatType] = save_float

	def save_string(self, object):
		d = id(object)
		self.write(STRING + `object` + '\n')
		self.write(PUT + `d` + '\n')
		self.memo[d] = object
	dispatch[StringType] = save_string

	def save_tuple(self, object):
		d = id(object)
		self.write(MARK)
		n = len(object)
		for k in range(n):
			self.save(object[k])
			if self.memo.has_key(d):
				# Saving object[k] has saved us!
				while k >= 0:
					self.write(POP)
					k = k-1
				self.write(GET + `d` + '\n')
				break
		else:
			self.write(TUPLE + PUT + `d` + '\n')
			self.memo[d] = object
	dispatch[TupleType] = save_tuple

	def save_list(self, object):
		d = id(object)
		self.write(MARK)
		n = len(object)
		for k in range(n):
			item = object[k]
			if not safe(item):
				break
			self.save(item)
		else:
			k = n
		self.write(LIST + PUT + `d` + '\n')
		self.memo[d] = object
		for k in range(k, n):
			item = object[k]
			self.save(item)
			self.write(APPEND)
	dispatch[ListType] = save_list

	def save_dict(self, object):
		d = id(object)
		self.write(MARK)
		items = object.items()
		n = len(items)
		for k in range(n):
			key, value = items[k]
			if not safe(key) or not safe(value):
				break
			self.save(key)
			self.save(value)
		else:
			k = n
		self.write(DICT + PUT + `d` + '\n')
		self.memo[d] = object
		for k in range(k, n):
			key, value = items[k]
			self.save(key)
			self.save(value)
			self.write(SETITEM)
	dispatch[DictionaryType] = save_dict

	def save_inst(self, object):
		d = id(object)
		cls = object.__class__
		module = whichmodule(cls)
		name = cls.__name__
		if hasattr(object, '__getinitargs__'):
			args = object.__getinitargs__()
			len(args) # XXX Assert it's a sequence
		else:
			args = ()
		self.write(MARK)
		for arg in args:
			self.save(arg)
		self.write(INST + module + '\n' + name + '\n' +
			   PUT + `d` + '\n')
		self.memo[d] = object
		try:
			getstate = object.__getstate__
		except AttributeError:
			stuff = object.__dict__
		else:
			stuff = getstate()
		self.save(stuff)
		self.write(BUILD)
	dispatch[InstanceType] = save_inst

	def save_class(self, object):
		d = id(object)
		module = whichmodule(object)
		name = object.__name__
		self.write(CLASS + module + '\n' + name + '\n' + 
			   PUT + `d` + '\n')
	dispatch[ClassType] = save_class


classmap = {}

def whichmodule(cls):
	"""Figure out the module in which a class occurs.

	Search sys.modules for the module.
	Cache in classmap.
	Return a module name.
	If the class cannot be found, return __main__.
	"""
	if classmap.has_key(cls):
		return classmap[cls]
	import sys
	clsname = cls.__name__
	for name, module in sys.modules.items():
		if module.__name__ != '__main__' and \
		   hasattr(module, clsname) and \
		   getattr(module, clsname) is cls:
			break
	else:
		name = '__main__'
	classmap[cls] = name
	return name


class Unpickler:

	def __init__(self, file):
		self.readline = file.readline
		self.read = file.read
		self.memo = {}

	def load(self):
		self.mark = ['spam'] # Any new unique object
		self.stack = []
		try:
			while 1:
				key = self.read(1)
				self.dispatch[key](self)
		except STOP, value:
			return value

	def marker(self):
		k = len(self.stack)-1
		while self.stack[k] != self.mark: k = k-1
		return k

	dispatch = {}

	def load_eof(self):
		raise EOFError
	dispatch[''] = load_eof

	def load_persid(self):
		pid = self.readline()[:-1]
		self.stack.append(self.persisent_load(pid))
	dispatch[PERSID] = load_persid

	def load_none(self):
		self.stack.append(None)
	dispatch[NONE] = load_none

	def load_atomic(self):
		self.stack.append(eval(self.readline()[:-1]))
	dispatch[INT] = load_atomic
	dispatch[LONG] = load_atomic
	dispatch[FLOAT] = load_atomic
	dispatch[STRING] = load_atomic

	def load_tuple(self):
		k = self.marker()
		self.stack[k:] = [tuple(self.stack[k+1:])]
	dispatch[TUPLE] = load_tuple

	def load_list(self):
		k = self.marker()
		self.stack[k:] = [self.stack[k+1:]]
	dispatch[LIST] = load_list

	def load_dict(self):
		k = self.marker()
		d = {}
		items = self.stack[k+1:]
		for i in range(0, len(items), 2):
			key = items[i]
			value = items[i+1]
			d[key] = value
		self.stack[k:] = [d]
	dispatch[DICT] = load_dict

	def load_inst(self):
		k = self.marker()
		args = tuple(self.stack[k+1:])
		del self.stack[k:]
		module = self.readline()[:-1]
		name = self.readline()[:-1]
		klass = self.find_class(module, name)
		value = apply(klass, args)
		self.stack.append(value)
	dispatch[INST] = load_inst

	def load_class(self):
		module = self.readline()[:-1]
		name = self.readline()[:-1]
		klass = self.find_class(module, name)
		self.stack.append(klass)
		return klass
	dispatch[CLASS] = load_class

	def find_class(self, module, name):
		env = {}
		try:
			exec 'from %s import %s' % (module, name) in env
		except ImportError:
			raise SystemError, \
			      "Failed to import class %s from module %s" % \
			      (name, module)
		klass = env[name]
		if type(klass) != ClassType:
			raise SystemError, \
			 "Imported object %s from module %s is not a class" % \
			 (name, module)
		return klass

	def load_pop(self):
		del self.stack[-1]
	dispatch[POP] = load_pop

	def load_dup(self):
		stack.append(stack[-1])
	dispatch[DUP] = load_dup

	def load_get(self):
		self.stack.append(self.memo[string.atoi(self.readline()[:-1])])
	dispatch[GET] = load_get

	def load_put(self):
		self.memo[string.atoi(self.readline()[:-1])] = self.stack[-1]
	dispatch[PUT] = load_put

	def load_append(self):
		value = self.stack[-1]
		del self.stack[-1]
		list = self.stack[-1]
		list.append(value)
	dispatch[APPEND] = load_append

	def load_setitem(self):
		value = self.stack[-1]
		key = self.stack[-2]
		del self.stack[-2:]
		dict = self.stack[-1]
		dict[key] = value
	dispatch[SETITEM] = load_setitem

	def load_build(self):
		value = self.stack[-1]
		del self.stack[-1]
		inst = self.stack[-1]
		try:
			setstate = inst.__setstate__
		except AttributeError:
			for key in value.keys():
				inst.__dict__[key] = value[key]
		else:
			setstate(value)
	dispatch[BUILD] = load_build

	def load_mark(self):
		self.stack.append(self.mark)
	dispatch[MARK] = load_mark

	def load_stop(self):
		value = self.stack[-1]
		del self.stack[-1]
		raise STOP, value
	dispatch[STOP] = load_stop


# Shorthands

def dump(object, file):
	Pickler(file).dump(object)

def dumps(object):
	import StringIO
	file = StringIO.StringIO()
	Pickler(file).dump(object)
	return file.getvalue()

def load(file):
	return Unpickler(file).load()

def loads(str):
	import StringIO
	file = StringIO.StringIO(str)
	return Unpickler(file).load()


# The rest is used for testing only

class C:
	def __cmp__(self, other):
		return cmp(self.__dict__, other.__dict__)

def test():
	fn = 'pickle_tmp'
	c = C()
	c.foo = 1
	c.bar = 2
	x = [0,1,2,3]
	y = ('abc', 'abc', c, c)
	x.append(y)
	x.append(y)
	x.append(5)
	f = open(fn, 'w')
	F = Pickler(f)
	F.dump(x)
	f.close()
	f = open(fn, 'r')
	U = Unpickler(f)
	x2 = U.load()
	print x
	print x2
	print x == x2
	print map(id, x)
	print map(id, x2)
	print F.memo
	print U.memo

if __name__ == '__main__':
	test()
