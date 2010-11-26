.. _tut-classes:

*******
Classes
*******

Compared with other programming languages, Python's class mechanism adds classes
with a minimum of new syntax and semantics.  It is a mixture of the class
mechanisms found in C++ and Modula-3.  Python classes provide all the standard
features of Object Oriented Programming: the class inheritance mechanism allows
multiple base classes, a derived class can override any methods of its base
class or classes, and a method can call the method of a base class with the same
name.  Objects can contain arbitrary amounts and kinds of data.  As is true for
modules, classes partake of the dynamic nature of Python: they are created at
runtime, and can be modified further after creation.

In C++ terminology, normally class members (including the data members) are
*public* (except see below :ref:`tut-private`), and all member functions are
*virtual*.  As in Modula-3, there are no shorthands for referencing the object's
members from its methods: the method function is declared with an explicit first
argument representing the object, which is provided implicitly by the call.  As
in Smalltalk, classes themselves are objects.  This provides semantics for
importing and renaming.  Unlike C++ and Modula-3, built-in types can be used as
base classes for extension by the user.  Also, like in C++, most built-in
operators with special syntax (arithmetic operators, subscripting etc.) can be
redefined for class instances.

(Lacking universally accepted terminology to talk about classes, I will make
occasional use of Smalltalk and C++ terms.  I would use Modula-3 terms, since
its object-oriented semantics are closer to those of Python than C++, but I
expect that few readers have heard of it.)


.. _tut-object:

A Word About Names and Objects
==============================

Objects have individuality, and multiple names (in multiple scopes) can be bound
to the same object.  This is known as aliasing in other languages.  This is
usually not appreciated on a first glance at Python, and can be safely ignored
when dealing with immutable basic types (numbers, strings, tuples).  However,
aliasing has a possibly surprising effect on the semantics of Python code
involving mutable objects such as lists, dictionaries, and most other types.
This is usually used to the benefit of the program, since aliases behave like
pointers in some respects.  For example, passing an object is cheap since only a
pointer is passed by the implementation; and if a function modifies an object
passed as an argument, the caller will see the change --- this eliminates the
need for two different argument passing mechanisms as in Pascal.


.. _tut-scopes:

Python Scopes and Namespaces
============================

Before introducing classes, I first have to tell you something about Python's
scope rules.  Class definitions play some neat tricks with namespaces, and you
need to know how scopes and namespaces work to fully understand what's going on.
Incidentally, knowledge about this subject is useful for any advanced Python
programmer.

Let's begin with some definitions.

A *namespace* is a mapping from names to objects.  Most namespaces are currently
implemented as Python dictionaries, but that's normally not noticeable in any
way (except for performance), and it may change in the future.  Examples of
namespaces are: the set of built-in names (containing functions such as :func:`abs`, and
built-in exception names); the global names in a module; and the local names in
a function invocation.  In a sense the set of attributes of an object also form
a namespace.  The important thing to know about namespaces is that there is
absolutely no relation between names in different namespaces; for instance, two
different modules may both define a function ``maximize`` without confusion ---
users of the modules must prefix it with the module name.

By the way, I use the word *attribute* for any name following a dot --- for
example, in the expression ``z.real``, ``real`` is an attribute of the object
``z``.  Strictly speaking, references to names in modules are attribute
references: in the expression ``modname.funcname``, ``modname`` is a module
object and ``funcname`` is an attribute of it.  In this case there happens to be
a straightforward mapping between the module's attributes and the global names
defined in the module: they share the same namespace!  [#]_

Attributes may be read-only or writable.  In the latter case, assignment to
attributes is possible.  Module attributes are writable: you can write
``modname.the_answer = 42``.  Writable attributes may also be deleted with the
:keyword:`del` statement.  For example, ``del modname.the_answer`` will remove
the attribute :attr:`the_answer` from the object named by ``modname``.

Namespaces are created at different moments and have different lifetimes.  The
namespace containing the built-in names is created when the Python interpreter
starts up, and is never deleted.  The global namespace for a module is created
when the module definition is read in; normally, module namespaces also last
until the interpreter quits.  The statements executed by the top-level
invocation of the interpreter, either read from a script file or interactively,
are considered part of a module called :mod:`__main__`, so they have their own
global namespace.  (The built-in names actually also live in a module; this is
called :mod:`__builtin__`.)

The local namespace for a function is created when the function is called, and
deleted when the function returns or raises an exception that is not handled
within the function.  (Actually, forgetting would be a better way to describe
what actually happens.)  Of course, recursive invocations each have their own
local namespace.

A *scope* is a textual region of a Python program where a namespace is directly
accessible.  "Directly accessible" here means that an unqualified reference to a
name attempts to find the name in the namespace.

Although scopes are determined statically, they are used dynamically. At any
time during execution, there are at least three nested scopes whose namespaces
are directly accessible:

* the innermost scope, which is searched first, contains the local names
* the scopes of any enclosing functions, which are searched starting with the
  nearest enclosing scope, contains non-local, but also non-global names
* the next-to-last scope contains the current module's global names
* the outermost scope (searched last) is the namespace containing built-in names

If a name is declared global, then all references and assignments go directly to
the middle scope containing the module's global names. Otherwise, all variables
found outside of the innermost scope are read-only (an attempt to write to such
a variable will simply create a *new* local variable in the innermost scope,
leaving the identically named outer variable unchanged).

Usually, the local scope references the local names of the (textually) current
function.  Outside functions, the local scope references the same namespace as
the global scope: the module's namespace. Class definitions place yet another
namespace in the local scope.

It is important to realize that scopes are determined textually: the global
scope of a function defined in a module is that module's namespace, no matter
from where or by what alias the function is called.  On the other hand, the
actual search for names is done dynamically, at run time --- however, the
language definition is evolving towards static name resolution, at "compile"
time, so don't rely on dynamic name resolution!  (In fact, local variables are
already determined statically.)

A special quirk of Python is that -- if no :keyword:`global` statement is in
effect -- assignments to names always go into the innermost scope.  Assignments
do not copy data --- they just bind names to objects.  The same is true for
deletions: the statement ``del x`` removes the binding of ``x`` from the
namespace referenced by the local scope.  In fact, all operations that introduce
new names use the local scope: in particular, :keyword:`import` statements and
function definitions bind the module or function name in the local scope.  (The
:keyword:`global` statement can be used to indicate that particular variables
live in the global scope.)


.. _tut-firstclasses:

A First Look at Classes
=======================

Classes introduce a little bit of new syntax, three new object types, and some
new semantics.


.. _tut-classdefinition:

Class Definition Syntax
-----------------------

The simplest form of class definition looks like this::

   class ClassName:
       <statement-1>
       .
       .
       .
       <statement-N>

Class definitions, like function definitions (:keyword:`def` statements) must be
executed before they have any effect.  (You could conceivably place a class
definition in a branch of an :keyword:`if` statement, or inside a function.)

In practice, the statements inside a class definition will usually be function
definitions, but other statements are allowed, and sometimes useful --- we'll
come back to this later.  The function definitions inside a class normally have
a peculiar form of argument list, dictated by the calling conventions for
methods --- again, this is explained later.

When a class definition is entered, a new namespace is created, and used as the
local scope --- thus, all assignments to local variables go into this new
namespace.  In particular, function definitions bind the name of the new
function here.

When a class definition is left normally (via the end), a *class object* is
created.  This is basically a wrapper around the contents of the namespace
created by the class definition; we'll learn more about class objects in the
next section.  The original local scope (the one in effect just before the class
definition was entered) is reinstated, and the class object is bound here to the
class name given in the class definition header (:class:`ClassName` in the
example).


.. _tut-classobjects:

Class Objects
-------------

Class objects support two kinds of operations: attribute references and
instantiation.

*Attribute references* use the standard syntax used for all attribute references
in Python: ``obj.name``.  Valid attribute names are all the names that were in
the class's namespace when the class object was created.  So, if the class
definition looked like this::

   class MyClass:
       """A simple example class"""
       i = 12345
       def f(self):
           return 'hello world'

then ``MyClass.i`` and ``MyClass.f`` are valid attribute references, returning
an integer and a function object, respectively. Class attributes can also be
assigned to, so you can change the value of ``MyClass.i`` by assignment.
:attr:`__doc__` is also a valid attribute, returning the docstring belonging to
the class: ``"A simple example class"``.

Class *instantiation* uses function notation.  Just pretend that the class
object is a parameterless function that returns a new instance of the class.
For example (assuming the above class)::

   x = MyClass()

creates a new *instance* of the class and assigns this object to the local
variable ``x``.

The instantiation operation ("calling" a class object) creates an empty object.
Many classes like to create objects with instances customized to a specific
initial state. Therefore a class may define a special method named
:meth:`__init__`, like this::

   def __init__(self):
       self.data = []

When a class defines an :meth:`__init__` method, class instantiation
automatically invokes :meth:`__init__` for the newly-created class instance.  So
in this example, a new, initialized instance can be obtained by::

   x = MyClass()

Of course, the :meth:`__init__` method may have arguments for greater
flexibility.  In that case, arguments given to the class instantiation operator
are passed on to :meth:`__init__`.  For example, ::

   >>> class Complex:
   ...     def __init__(self, realpart, imagpart):
   ...         self.r = realpart
   ...         self.i = imagpart
   ...
   >>> x = Complex(3.0, -4.5)
   >>> x.r, x.i
   (3.0, -4.5)


.. _tut-instanceobjects:

Instance Objects
----------------

Now what can we do with instance objects?  The only operations understood by
instance objects are attribute references.  There are two kinds of valid
attribute names, data attributes and methods.

*data attributes* correspond to "instance variables" in Smalltalk, and to "data
members" in C++.  Data attributes need not be declared; like local variables,
they spring into existence when they are first assigned to.  For example, if
``x`` is the instance of :class:`MyClass` created above, the following piece of
code will print the value ``16``, without leaving a trace::

   x.counter = 1
   while x.counter < 10:
       x.counter = x.counter * 2
   print x.counter
   del x.counter

The other kind of instance attribute reference is a *method*. A method is a
function that "belongs to" an object.  (In Python, the term method is not unique
to class instances: other object types can have methods as well.  For example,
list objects have methods called append, insert, remove, sort, and so on.
However, in the following discussion, we'll use the term method exclusively to
mean methods of class instance objects, unless explicitly stated otherwise.)

.. index:: object: method

Valid method names of an instance object depend on its class.  By definition,
all attributes of a class that are function  objects define corresponding
methods of its instances.  So in our example, ``x.f`` is a valid method
reference, since ``MyClass.f`` is a function, but ``x.i`` is not, since
``MyClass.i`` is not.  But ``x.f`` is not the same thing as ``MyClass.f`` --- it
is a *method object*, not a function object.


.. _tut-methodobjects:

Method Objects
--------------

Usually, a method is called right after it is bound::

   x.f()

In the :class:`MyClass` example, this will return the string ``'hello world'``.
However, it is not necessary to call a method right away: ``x.f`` is a method
object, and can be stored away and called at a later time.  For example::

   xf = x.f
   while True:
       print xf()

will continue to print ``hello world`` until the end of time.

What exactly happens when a method is called?  You may have noticed that
``x.f()`` was called without an argument above, even though the function
definition for :meth:`f` specified an argument.  What happened to the argument?
Surely Python raises an exception when a function that requires an argument is
called without any --- even if the argument isn't actually used...

Actually, you may have guessed the answer: the special thing about methods is
that the object is passed as the first argument of the function.  In our
example, the call ``x.f()`` is exactly equivalent to ``MyClass.f(x)``.  In
general, calling a method with a list of *n* arguments is equivalent to calling
the corresponding function with an argument list that is created by inserting
the method's object before the first argument.

If you still don't understand how methods work, a look at the implementation can
perhaps clarify matters.  When an instance attribute is referenced that isn't a
data attribute, its class is searched.  If the name denotes a valid class
attribute that is a function object, a method object is created by packing
(pointers to) the instance object and the function object just found together in
an abstract object: this is the method object.  When the method object is called
with an argument list, a new argument list is constructed from the instance
object and the argument list, and the function object is called with this new
argument list.


.. _tut-remarks:

Random Remarks
==============

.. These should perhaps be placed more carefully...

Data attributes override method attributes with the same name; to avoid
accidental name conflicts, which may cause hard-to-find bugs in large programs,
it is wise to use some kind of convention that minimizes the chance of
conflicts.  Possible conventions include capitalizing method names, prefixing
data attribute names with a small unique string (perhaps just an underscore), or
using verbs for methods and nouns for data attributes.

Data attributes may be referenced by methods as well as by ordinary users
("clients") of an object.  In other words, classes are not usable to implement
pure abstract data types.  In fact, nothing in Python makes it possible to
enforce data hiding --- it is all based upon convention.  (On the other hand,
the Python implementation, written in C, can completely hide implementation
details and control access to an object if necessary; this can be used by
extensions to Python written in C.)

Clients should use data attributes with care --- clients may mess up invariants
maintained by the methods by stamping on their data attributes.  Note that
clients may add data attributes of their own to an instance object without
affecting the validity of the methods, as long as name conflicts are avoided ---
again, a naming convention can save a lot of headaches here.

There is no shorthand for referencing data attributes (or other methods!) from
within methods.  I find that this actually increases the readability of methods:
there is no chance of confusing local variables and instance variables when
glancing through a method.

Often, the first argument of a method is called ``self``.  This is nothing more
than a convention: the name ``self`` has absolutely no special meaning to
Python.  Note, however, that by not following the convention your code may be
less readable to other Python programmers, and it is also conceivable that a
*class browser* program might be written that relies upon such a convention.

Any function object that is a class attribute defines a method for instances of
that class.  It is not necessary that the function definition is textually
enclosed in the class definition: assigning a function object to a local
variable in the class is also ok.  For example::

   # Function defined outside the class
   def f1(self, x, y):
       return min(x, x+y)

   class C:
       f = f1
       def g(self):
           return 'hello world'
       h = g

Now ``f``, ``g`` and ``h`` are all attributes of class :class:`C` that refer to
function objects, and consequently they are all methods of instances of
:class:`C` --- ``h`` being exactly equivalent to ``g``.  Note that this practice
usually only serves to confuse the reader of a program.

Methods may call other methods by using method attributes of the ``self``
argument::

   class Bag:
       def __init__(self):
           self.data = []
       def add(self, x):
           self.data.append(x)
       def addtwice(self, x):
           self.add(x)
           self.add(x)

Methods may reference global names in the same way as ordinary functions.  The
global scope associated with a method is the module containing the class
definition.  (The class itself is never used as a global scope.)  While one
rarely encounters a good reason for using global data in a method, there are
many legitimate uses of the global scope: for one thing, functions and modules
imported into the global scope can be used by methods, as well as functions and
classes defined in it.  Usually, the class containing the method is itself
defined in this global scope, and in the next section we'll find some good
reasons why a method would want to reference its own class.

Each value is an object, and therefore has a *class* (also called its *type*).
It is stored as ``object.__class__``.


.. _tut-inheritance:

Inheritance
===========

Of course, a language feature would not be worthy of the name "class" without
supporting inheritance.  The syntax for a derived class definition looks like
this::

   class DerivedClassName(BaseClassName):
       <statement-1>
       .
       .
       .
       <statement-N>

The name :class:`BaseClassName` must be defined in a scope containing the
derived class definition.  In place of a base class name, other arbitrary
expressions are also allowed.  This can be useful, for example, when the base
class is defined in another module::

   class DerivedClassName(modname.BaseClassName):

Execution of a derived class definition proceeds the same as for a base class.
When the class object is constructed, the base class is remembered.  This is
used for resolving attribute references: if a requested attribute is not found
in the class, the search proceeds to look in the base class.  This rule is
applied recursively if the base class itself is derived from some other class.

There's nothing special about instantiation of derived classes:
``DerivedClassName()`` creates a new instance of the class.  Method references
are resolved as follows: the corresponding class attribute is searched,
descending down the chain of base classes if necessary, and the method reference
is valid if this yields a function object.

Derived classes may override methods of their base classes.  Because methods
have no special privileges when calling other methods of the same object, a
method of a base class that calls another method defined in the same base class
may end up calling a method of a derived class that overrides it.  (For C++
programmers: all methods in Python are effectively ``virtual``.)

An overriding method in a derived class may in fact want to extend rather than
simply replace the base class method of the same name. There is a simple way to
call the base class method directly: just call ``BaseClassName.methodname(self,
arguments)``.  This is occasionally useful to clients as well.  (Note that this
only works if the base class is accessible as ``BaseClassName`` in the global
scope.)

Python has two built-in functions that work with inheritance:

* Use :func:`isinstance` to check an instance's type: ``isinstance(obj, int)``
  will be ``True`` only if ``obj.__class__`` is :class:`int` or some class
  derived from :class:`int`.

* Use :func:`issubclass` to check class inheritance: ``issubclass(bool, int)``
  is ``True`` since :class:`bool` is a subclass of :class:`int`.  However,
  ``issubclass(unicode, str)`` is ``False`` since :class:`unicode` is not a
  subclass of :class:`str` (they only share a common ancestor,
  :class:`basestring`).



.. _tut-multiple:

Multiple Inheritance
--------------------

Python supports a limited form of multiple inheritance as well.  A class
definition with multiple base classes looks like this::

   class DerivedClassName(Base1, Base2, Base3):
       <statement-1>
       .
       .
       .
       <statement-N>

For old-style classes, the only rule is depth-first, left-to-right.  Thus, if an
attribute is not found in :class:`DerivedClassName`, it is searched in
:class:`Base1`, then (recursively) in the base classes of :class:`Base1`, and
only if it is not found there, it is searched in :class:`Base2`, and so on.

(To some people breadth first --- searching :class:`Base2` and :class:`Base3`
before the base classes of :class:`Base1` --- looks more natural.  However, this
would require you to know whether a particular attribute of :class:`Base1` is
actually defined in :class:`Base1` or in one of its base classes before you can
figure out the consequences of a name conflict with an attribute of
:class:`Base2`.  The depth-first rule makes no differences between direct and
inherited attributes of :class:`Base1`.)

For :term:`new-style class`\es, the method resolution order changes dynamically
to support cooperative calls to :func:`super`.  This approach is known in some
other multiple-inheritance languages as call-next-method and is more powerful
than the super call found in single-inheritance languages.

With new-style classes, dynamic ordering is necessary because all  cases of
multiple inheritance exhibit one or more diamond relationships (where one at
least one of the parent classes can be accessed through multiple paths from the
bottommost class).  For example, all new-style classes inherit from
:class:`object`, so any case of multiple inheritance provides more than one path
to reach :class:`object`.  To keep the base classes from being accessed more
than once, the dynamic algorithm linearizes the search order in a way that
preserves the left-to-right ordering specified in each class, that calls each
parent only once, and that is monotonic (meaning that a class can be subclassed
without affecting the precedence order of its parents).  Taken together, these
properties make it possible to design reliable and extensible classes with
multiple inheritance.  For more detail, see
http://www.python.org/download/releases/2.3/mro/.


.. _tut-private:

Private Variables
=================

"Private" instance variables that cannot be accessed except from inside an
object don't exist in Python.  However, there is a convention that is followed
by most Python code: a name prefixed with an underscore (e.g. ``_spam``) should
be treated as a non-public part of the API (whether it is a function, a method
or a data member).  It should be considered an implementation detail and subject
to change without notice.

Since there is a valid use-case for class-private members (namely to avoid name
clashes of names with names defined by subclasses), there is limited support for
such a mechanism, called :dfn:`name mangling`.  Any identifier of the form
``__spam`` (at least two leading underscores, at most one trailing underscore)
is textually replaced with ``_classname__spam``, where ``classname`` is the
current class name with leading underscore(s) stripped.  This mangling is done
without regard to the syntactic position of the identifier, as long as it
occurs within the definition of a class.

Note that the mangling rules are designed mostly to avoid accidents; it still is
possible to access or modify a variable that is considered private.  This can
even be useful in special circumstances, such as in the debugger.

Notice that code passed to ``exec``, ``eval()`` or ``execfile()`` does not
consider the classname of the invoking  class to be the current class; this is
similar to the effect of the  ``global`` statement, the effect of which is
likewise restricted to  code that is byte-compiled together.  The same
restriction applies to ``getattr()``, ``setattr()`` and ``delattr()``, as well
as when referencing ``__dict__`` directly.


.. _tut-odds:

Odds and Ends
=============

Sometimes it is useful to have a data type similar to the Pascal "record" or C
"struct", bundling together a few named data items.  An empty class definition
will do nicely::

   class Employee:
       pass

   john = Employee() # Create an empty employee record

   # Fill the fields of the record
   john.name = 'John Doe'
   john.dept = 'computer lab'
   john.salary = 1000

A piece of Python code that expects a particular abstract data type can often be
passed a class that emulates the methods of that data type instead.  For
instance, if you have a function that formats some data from a file object, you
can define a class with methods :meth:`read` and :meth:`readline` that get the
data from a string buffer instead, and pass it as an argument.

.. (Unfortunately, this technique has its limitations: a class can't define
   operations that are accessed by special syntax such as sequence subscripting
   or arithmetic operators, and assigning such a "pseudo-file" to sys.stdin will
   not cause the interpreter to read further input from it.)

Instance method objects have attributes, too: ``m.im_self`` is the instance
object with the method :meth:`m`, and ``m.im_func`` is the function object
corresponding to the method.


.. _tut-exceptionclasses:

Exceptions Are Classes Too
==========================

User-defined exceptions are identified by classes as well.  Using this mechanism
it is possible to create extensible hierarchies of exceptions.

There are two new valid (semantic) forms for the :keyword:`raise` statement::

   raise Class, instance

   raise instance

In the first form, ``instance`` must be an instance of :class:`Class` or of a
class derived from it.  The second form is a shorthand for::

   raise instance.__class__, instance

A class in an :keyword:`except` clause is compatible with an exception if it is
the same class or a base class thereof (but not the other way around --- an
except clause listing a derived class is not compatible with a base class).  For
example, the following code will print B, C, D in that order::

   class B:
       pass
   class C(B):
       pass
   class D(C):
       pass

   for c in [B, C, D]:
       try:
           raise c()
       except D:
           print "D"
       except C:
           print "C"
       except B:
           print "B"

Note that if the except clauses were reversed (with ``except B`` first), it
would have printed B, B, B --- the first matching except clause is triggered.

When an error message is printed for an unhandled exception, the exception's
class name is printed, then a colon and a space, and finally the instance
converted to a string using the built-in function :func:`str`.


.. _tut-iterators:

Iterators
=========

By now you have probably noticed that most container objects can be looped over
using a :keyword:`for` statement::

   for element in [1, 2, 3]:
       print element
   for element in (1, 2, 3):
       print element
   for key in {'one':1, 'two':2}:
       print key
   for char in "123":
       print char
   for line in open("myfile.txt"):
       print line

This style of access is clear, concise, and convenient.  The use of iterators
pervades and unifies Python.  Behind the scenes, the :keyword:`for` statement
calls :func:`iter` on the container object.  The function returns an iterator
object that defines the method :meth:`next` which accesses elements in the
container one at a time.  When there are no more elements, :meth:`next` raises a
:exc:`StopIteration` exception which tells the :keyword:`for` loop to terminate.
This example shows how it all works::

   >>> s = 'abc'
   >>> it = iter(s)
   >>> it
   <iterator object at 0x00A1DB50>
   >>> it.next()
   'a'
   >>> it.next()
   'b'
   >>> it.next()
   'c'
   >>> it.next()

   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
       it.next()
   StopIteration

Having seen the mechanics behind the iterator protocol, it is easy to add
iterator behavior to your classes.  Define an :meth:`__iter__` method which
returns an object with a :meth:`next` method.  If the class defines
:meth:`next`, then :meth:`__iter__` can just return ``self``::

   class Reverse:
       "Iterator for looping over a sequence backwards"
       def __init__(self, data):
           self.data = data
           self.index = len(data)
       def __iter__(self):
           return self
       def next(self):
           if self.index == 0:
               raise StopIteration
           self.index = self.index - 1
           return self.data[self.index]

   >>> rev = Reverse('spam')
   >>> iter(rev)
   <__main__.Reverse object at 0x00A1DB50>
   >>> for char in rev:
   ...     print char
   ...
   m
   a
   p
   s


.. _tut-generators:

Generators
==========

:term:`Generator`\s are a simple and powerful tool for creating iterators.  They
are written like regular functions but use the :keyword:`yield` statement
whenever they want to return data.  Each time :meth:`next` is called, the
generator resumes where it left-off (it remembers all the data values and which
statement was last executed).  An example shows that generators can be trivially
easy to create::

   def reverse(data):
       for index in range(len(data)-1, -1, -1):
           yield data[index]

   >>> for char in reverse('golf'):
   ...     print char
   ...
   f
   l
   o
   g

Anything that can be done with generators can also be done with class based
iterators as described in the previous section.  What makes generators so
compact is that the :meth:`__iter__` and :meth:`next` methods are created
automatically.

Another key feature is that the local variables and execution state are
automatically saved between calls.  This made the function easier to write and
much more clear than an approach using instance variables like ``self.index``
and ``self.data``.

In addition to automatic method creation and saving program state, when
generators terminate, they automatically raise :exc:`StopIteration`. In
combination, these features make it easy to create iterators with no more effort
than writing a regular function.


.. _tut-genexps:

Generator Expressions
=====================

Some simple generators can be coded succinctly as expressions using a syntax
similar to list comprehensions but with parentheses instead of brackets.  These
expressions are designed for situations where the generator is used right away
by an enclosing function.  Generator expressions are more compact but less
versatile than full generator definitions and tend to be more memory friendly
than equivalent list comprehensions.

Examples::

   >>> sum(i*i for i in range(10))                 # sum of squares
   285

   >>> xvec = [10, 20, 30]
   >>> yvec = [7, 5, 3]
   >>> sum(x*y for x,y in zip(xvec, yvec))         # dot product
   260

   >>> from math import pi, sin
   >>> sine_table = dict((x, sin(x*pi/180)) for x in range(0, 91))

   >>> unique_words = set(word  for line in page  for word in line.split())

   >>> valedictorian = max((student.gpa, student.name) for student in graduates)

   >>> data = 'golf'
   >>> list(data[i] for i in range(len(data)-1,-1,-1))
   ['f', 'l', 'o', 'g']



.. rubric:: Footnotes

.. [#] Except for one thing.  Module objects have a secret read-only attribute called
   :attr:`__dict__` which returns the dictionary used to implement the module's
   namespace; the name :attr:`__dict__` is an attribute but not a global name.
   Obviously, using this violates the abstraction of namespace implementation, and
   should be restricted to things like post-mortem debuggers.

