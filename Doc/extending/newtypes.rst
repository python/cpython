.. highlightlang:: c


.. _defining-new-types:

******************
Defining New Types
******************

.. sectionauthor:: Michael Hudson <mwh@python.net>
.. sectionauthor:: Dave Kuhlman <dkuhlman@rexx.com>
.. sectionauthor:: Jim Fulton <jim@zope.com>


As mentioned in the last chapter, Python allows the writer of an extension
module to define new types that can be manipulated from Python code, much like
strings and lists in core Python.

This is not hard; the code for all extension types follows a pattern, but there
are some details that you need to understand before you can get started.


.. _dnt-basics:

The Basics
==========

The Python runtime sees all Python objects as variables of type
:c:type:`PyObject\*`, which serves as a "base type" for all Python objects.
:c:type:`PyObject` itself only contains the refcount and a pointer to the
object's "type object".  This is where the action is; the type object determines
which (C) functions get called when, for instance, an attribute gets looked
up on an object or it is multiplied by another object.  These C functions
are called "type methods".

So, if you want to define a new object type, you need to create a new type
object.

This sort of thing can only be explained by example, so here's a minimal, but
complete, module that defines a new type:

.. literalinclude:: ../includes/noddy.c


Now that's quite a bit to take in at once, but hopefully bits will seem familiar
from the last chapter.

The first bit that will be new is::

   typedef struct {
       PyObject_HEAD
   } noddy_NoddyObject;

This is what a Noddy object will contain---in this case, nothing more than what
every Python object contains---a field called ``ob_base`` of type
:c:type:`PyObject`.  :c:type:`PyObject` in turn, contains an ``ob_refcnt``
field and a pointer to a type object.  These can be accessed using the macros
:c:macro:`Py_REFCNT` and :c:macro:`Py_TYPE` respectively.  These are the fields
the :c:macro:`PyObject_HEAD` macro brings in.  The reason for the macro is to
standardize the layout and to enable special debugging fields in debug builds.

Note that there is no semicolon after the :c:macro:`PyObject_HEAD` macro;
one is included in the macro definition.  Be wary of adding one by
accident; it's easy to do from habit, and your compiler might not complain,
but someone else's probably will!  (On Windows, MSVC is known to call this an
error and refuse to compile the code.)

For contrast, let's take a look at the corresponding definition for standard
Python floats::

   typedef struct {
       PyObject_HEAD
       double ob_fval;
   } PyFloatObject;

Moving on, we come to the crunch --- the type object. ::

   static PyTypeObject noddy_NoddyType = {
       PyVarObject_HEAD_INIT(NULL, 0)
       "noddy.Noddy",             /* tp_name */
       sizeof(noddy_NoddyObject), /* tp_basicsize */
       0,                         /* tp_itemsize */
       0,                         /* tp_dealloc */
       0,                         /* tp_print */
       0,                         /* tp_getattr */
       0,                         /* tp_setattr */
       0,                         /* tp_as_async */
       0,                         /* tp_repr */
       0,                         /* tp_as_number */
       0,                         /* tp_as_sequence */
       0,                         /* tp_as_mapping */
       0,                         /* tp_hash  */
       0,                         /* tp_call */
       0,                         /* tp_str */
       0,                         /* tp_getattro */
       0,                         /* tp_setattro */
       0,                         /* tp_as_buffer */
       Py_TPFLAGS_DEFAULT,        /* tp_flags */
       "Noddy objects",           /* tp_doc */
   };

Now if you go and look up the definition of :c:type:`PyTypeObject` in
:file:`object.h` you'll see that it has many more fields that the definition
above.  The remaining fields will be filled with zeros by the C compiler, and
it's common practice to not specify them explicitly unless you need them.

This is so important that we're going to pick the top of it apart still
further::

   PyVarObject_HEAD_INIT(NULL, 0)

This line is a bit of a wart; what we'd like to write is::

   PyVarObject_HEAD_INIT(&PyType_Type, 0)

as the type of a type object is "type", but this isn't strictly conforming C and
some compilers complain.  Fortunately, this member will be filled in for us by
:c:func:`PyType_Ready`. ::

   "noddy.Noddy",              /* tp_name */

The name of our type.  This will appear in the default textual representation of
our objects and in some error messages, for example::

   >>> "" + noddy.new_noddy()
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   TypeError: cannot add type "noddy.Noddy" to string

Note that the name is a dotted name that includes both the module name and the
name of the type within the module. The module in this case is :mod:`noddy` and
the type is :class:`Noddy`, so we set the type name to :class:`noddy.Noddy`.
One side effect of using an undotted name is that the pydoc documentation tool
will not list the new type in the module documentation. ::

   sizeof(noddy_NoddyObject),  /* tp_basicsize */

This is so that Python knows how much memory to allocate when you call
:c:func:`PyObject_New`.

.. note::

   If you want your type to be subclassable from Python, and your type has the same
   :c:member:`~PyTypeObject.tp_basicsize` as its base type, you may have problems with multiple
   inheritance.  A Python subclass of your type will have to list your type first
   in its :attr:`~class.__bases__`, or else it will not be able to call your type's
   :meth:`__new__` method without getting an error.  You can avoid this problem by
   ensuring that your type has a larger value for :c:member:`~PyTypeObject.tp_basicsize` than its
   base type does.  Most of the time, this will be true anyway, because either your
   base type will be :class:`object`, or else you will be adding data members to
   your base type, and therefore increasing its size.

::

   0,                          /* tp_itemsize */

This has to do with variable length objects like lists and strings. Ignore this
for now.

Skipping a number of type methods that we don't provide, we set the class flags
to :const:`Py_TPFLAGS_DEFAULT`. ::

   Py_TPFLAGS_DEFAULT,        /* tp_flags */

All types should include this constant in their flags.  It enables all of the
members defined until at least Python 3.3.  If you need further members,
you will need to OR the corresponding flags.

We provide a doc string for the type in :c:member:`~PyTypeObject.tp_doc`. ::

   "Noddy objects",           /* tp_doc */

Now we get into the type methods, the things that make your objects different
from the others.  We aren't going to implement any of these in this version of
the module.  We'll expand this example later to have more interesting behavior.

For now, all we want to be able to do is to create new :class:`Noddy` objects.
To enable object creation, we have to provide a :c:member:`~PyTypeObject.tp_new` implementation.
In this case, we can just use the default implementation provided by the API
function :c:func:`PyType_GenericNew`.  We'd like to just assign this to the
:c:member:`~PyTypeObject.tp_new` slot, but we can't, for portability sake, On some platforms or
compilers, we can't statically initialize a structure member with a function
defined in another C module, so, instead, we'll assign the :c:member:`~PyTypeObject.tp_new` slot
in the module initialization function just before calling
:c:func:`PyType_Ready`::

   noddy_NoddyType.tp_new = PyType_GenericNew;
   if (PyType_Ready(&noddy_NoddyType) < 0)
       return;

All the other type methods are *NULL*, so we'll go over them later --- that's
for a later section!

Everything else in the file should be familiar, except for some code in
:c:func:`PyInit_noddy`::

   if (PyType_Ready(&noddy_NoddyType) < 0)
       return;

This initializes the :class:`Noddy` type, filing in a number of members,
including :attr:`ob_type` that we initially set to *NULL*. ::

   PyModule_AddObject(m, "Noddy", (PyObject *)&noddy_NoddyType);

This adds the type to the module dictionary.  This allows us to create
:class:`Noddy` instances by calling the :class:`Noddy` class::

   >>> import noddy
   >>> mynoddy = noddy.Noddy()

That's it!  All that remains is to build it; put the above code in a file called
:file:`noddy.c` and ::

   from distutils.core import setup, Extension
   setup(name="noddy", version="1.0",
         ext_modules=[Extension("noddy", ["noddy.c"])])

in a file called :file:`setup.py`; then typing

.. code-block:: shell-session

   $ python setup.py build

at a shell should produce a file :file:`noddy.so` in a subdirectory; move to
that directory and fire up Python --- you should be able to ``import noddy`` and
play around with Noddy objects.

That wasn't so hard, was it?

Of course, the current Noddy type is pretty uninteresting. It has no data and
doesn't do anything. It can't even be subclassed.


Adding data and methods to the Basic example
--------------------------------------------

Let's extend the basic example to add some data and methods.  Let's also make
the type usable as a base class. We'll create a new module, :mod:`noddy2` that
adds these capabilities:

.. literalinclude:: ../includes/noddy2.c


This version of the module has a number of changes.

We've added an extra include::

   #include <structmember.h>

This include provides declarations that we use to handle attributes, as
described a bit later.

The name of the :class:`Noddy` object structure has been shortened to
:class:`Noddy`.  The type object name has been shortened to :class:`NoddyType`.

The  :class:`Noddy` type now has three data attributes, *first*, *last*, and
*number*.  The *first* and *last* variables are Python strings containing first
and last names. The *number* attribute is an integer.

The object structure is updated accordingly::

   typedef struct {
       PyObject_HEAD
       PyObject *first;
       PyObject *last;
       int number;
   } Noddy;

Because we now have data to manage, we have to be more careful about object
allocation and deallocation.  At a minimum, we need a deallocation method::

   static void
   Noddy_dealloc(Noddy* self)
   {
       Py_XDECREF(self->first);
       Py_XDECREF(self->last);
       Py_TYPE(self)->tp_free((PyObject*)self);
   }

which is assigned to the :c:member:`~PyTypeObject.tp_dealloc` member::

   (destructor)Noddy_dealloc, /*tp_dealloc*/

This method decrements the reference counts of the two Python attributes. We use
:c:func:`Py_XDECREF` here because the :attr:`first` and :attr:`last` members
could be *NULL*.  It then calls the :c:member:`~PyTypeObject.tp_free` member of the object's type
to free the object's memory.  Note that the object's type might not be
:class:`NoddyType`, because the object may be an instance of a subclass.

We want to make sure that the first and last names are initialized to empty
strings, so we provide a new method::

   static PyObject *
   Noddy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
   {
       Noddy *self;

       self = (Noddy *)type->tp_alloc(type, 0);
       if (self != NULL) {
           self->first = PyUnicode_FromString("");
           if (self->first == NULL) {
               Py_DECREF(self);
               return NULL;
           }

           self->last = PyUnicode_FromString("");
           if (self->last == NULL) {
               Py_DECREF(self);
               return NULL;
           }

           self->number = 0;
       }

       return (PyObject *)self;
   }

and install it in the :c:member:`~PyTypeObject.tp_new` member::

   Noddy_new,                 /* tp_new */

The new member is responsible for creating (as opposed to initializing) objects
of the type.  It is exposed in Python as the :meth:`__new__` method.  See the
paper titled "Unifying types and classes in Python" for a detailed discussion of
the :meth:`__new__` method.  One reason to implement a new method is to assure
the initial values of instance variables.  In this case, we use the new method
to make sure that the initial values of the members :attr:`first` and
:attr:`last` are not *NULL*. If we didn't care whether the initial values were
*NULL*, we could have used :c:func:`PyType_GenericNew` as our new method, as we
did before.  :c:func:`PyType_GenericNew` initializes all of the instance variable
members to *NULL*.

The new method is a static method that is passed the type being instantiated and
any arguments passed when the type was called, and that returns the new object
created. New methods always accept positional and keyword arguments, but they
often ignore the arguments, leaving the argument handling to initializer
methods. Note that if the type supports subclassing, the type passed may not be
the type being defined.  The new method calls the :c:member:`~PyTypeObject.tp_alloc` slot to
allocate memory. We don't fill the :c:member:`~PyTypeObject.tp_alloc` slot ourselves. Rather
:c:func:`PyType_Ready` fills it for us by inheriting it from our base class,
which is :class:`object` by default.  Most types use the default allocation.

.. note::

   If you are creating a co-operative :c:member:`~PyTypeObject.tp_new` (one that calls a base type's
   :c:member:`~PyTypeObject.tp_new` or :meth:`__new__`), you must *not* try to determine what method
   to call using method resolution order at runtime.  Always statically determine
   what type you are going to call, and call its :c:member:`~PyTypeObject.tp_new` directly, or via
   ``type->tp_base->tp_new``.  If you do not do this, Python subclasses of your
   type that also inherit from other Python-defined classes may not work correctly.
   (Specifically, you may not be able to create instances of such subclasses
   without getting a :exc:`TypeError`.)

We provide an initialization function::

   static int
   Noddy_init(Noddy *self, PyObject *args, PyObject *kwds)
   {
       PyObject *first=NULL, *last=NULL, *tmp;

       static char *kwlist[] = {"first", "last", "number", NULL};

       if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist,
                                         &first, &last,
                                         &self->number))
           return -1;

       if (first) {
           tmp = self->first;
           Py_INCREF(first);
           self->first = first;
           Py_XDECREF(tmp);
       }

       if (last) {
           tmp = self->last;
           Py_INCREF(last);
           self->last = last;
           Py_XDECREF(tmp);
       }

       return 0;
   }

by filling the :c:member:`~PyTypeObject.tp_init` slot. ::

   (initproc)Noddy_init,         /* tp_init */

The :c:member:`~PyTypeObject.tp_init` slot is exposed in Python as the :meth:`__init__` method. It
is used to initialize an object after it's created. Unlike the new method, we
can't guarantee that the initializer is called.  The initializer isn't called
when unpickling objects and it can be overridden.  Our initializer accepts
arguments to provide initial values for our instance. Initializers always accept
positional and keyword arguments. Initializers should return either 0 on
success or -1 on error.

Initializers can be called multiple times.  Anyone can call the :meth:`__init__`
method on our objects.  For this reason, we have to be extra careful when
assigning the new values.  We might be tempted, for example to assign the
:attr:`first` member like this::

   if (first) {
       Py_XDECREF(self->first);
       Py_INCREF(first);
       self->first = first;
   }

But this would be risky.  Our type doesn't restrict the type of the
:attr:`first` member, so it could be any kind of object.  It could have a
destructor that causes code to be executed that tries to access the
:attr:`first` member.  To be paranoid and protect ourselves against this
possibility, we almost always reassign members before decrementing their
reference counts.  When don't we have to do this?

* when we absolutely know that the reference count is greater than 1

* when we know that deallocation of the object [#]_ will not cause any calls
  back into our type's code

* when decrementing a reference count in a :c:member:`~PyTypeObject.tp_dealloc` handler when
  garbage-collections is not supported [#]_

We want to expose our instance variables as attributes. There are a
number of ways to do that. The simplest way is to define member definitions::

   static PyMemberDef Noddy_members[] = {
       {"first", T_OBJECT_EX, offsetof(Noddy, first), 0,
        "first name"},
       {"last", T_OBJECT_EX, offsetof(Noddy, last), 0,
        "last name"},
       {"number", T_INT, offsetof(Noddy, number), 0,
        "noddy number"},
       {NULL}  /* Sentinel */
   };

and put the definitions in the :c:member:`~PyTypeObject.tp_members` slot::

   Noddy_members,             /* tp_members */

Each member definition has a member name, type, offset, access flags and
documentation string. See the :ref:`Generic-Attribute-Management` section below for
details.

A disadvantage of this approach is that it doesn't provide a way to restrict the
types of objects that can be assigned to the Python attributes.  We expect the
first and last names to be strings, but any Python objects can be assigned.
Further, the attributes can be deleted, setting the C pointers to *NULL*.  Even
though we can make sure the members are initialized to non-*NULL* values, the
members can be set to *NULL* if the attributes are deleted.

We define a single method, :meth:`name`, that outputs the objects name as the
concatenation of the first and last names. ::

   static PyObject *
   Noddy_name(Noddy* self)
   {
       if (self->first == NULL) {
           PyErr_SetString(PyExc_AttributeError, "first");
           return NULL;
       }

       if (self->last == NULL) {
           PyErr_SetString(PyExc_AttributeError, "last");
           return NULL;
       }

       return PyUnicode_FromFormat("%S %S", self->first, self->last);
   }

The method is implemented as a C function that takes a :class:`Noddy` (or
:class:`Noddy` subclass) instance as the first argument.  Methods always take an
instance as the first argument. Methods often take positional and keyword
arguments as well, but in this case we don't take any and don't need to accept
a positional argument tuple or keyword argument dictionary. This method is
equivalent to the Python method::

   def name(self):
      return "%s %s" % (self.first, self.last)

Note that we have to check for the possibility that our :attr:`first` and
:attr:`last` members are *NULL*.  This is because they can be deleted, in which
case they are set to *NULL*.  It would be better to prevent deletion of these
attributes and to restrict the attribute values to be strings.  We'll see how to
do that in the next section.

Now that we've defined the method, we need to create an array of method
definitions::

   static PyMethodDef Noddy_methods[] = {
       {"name", (PyCFunction)Noddy_name, METH_NOARGS,
        "Return the name, combining the first and last name"
       },
       {NULL}  /* Sentinel */
   };

and assign them to the :c:member:`~PyTypeObject.tp_methods` slot::

   Noddy_methods,             /* tp_methods */

Note that we used the :const:`METH_NOARGS` flag to indicate that the method is
passed no arguments.

Finally, we'll make our type usable as a base class.  We've written our methods
carefully so far so that they don't make any assumptions about the type of the
object being created or used, so all we need to do is to add the
:const:`Py_TPFLAGS_BASETYPE` to our class flag definition::

   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/

We rename :c:func:`PyInit_noddy` to :c:func:`PyInit_noddy2` and update the module
name in the :c:type:`PyModuleDef` struct.

Finally, we update our :file:`setup.py` file to build the new module::

   from distutils.core import setup, Extension
   setup(name="noddy", version="1.0",
         ext_modules=[
            Extension("noddy", ["noddy.c"]),
            Extension("noddy2", ["noddy2.c"]),
            ])


Providing finer control over data attributes
--------------------------------------------

In this section, we'll provide finer control over how the :attr:`first` and
:attr:`last` attributes are set in the :class:`Noddy` example. In the previous
version of our module, the instance variables :attr:`first` and :attr:`last`
could be set to non-string values or even deleted. We want to make sure that
these attributes always contain strings.

.. literalinclude:: ../includes/noddy3.c


To provide greater control, over the :attr:`first` and :attr:`last` attributes,
we'll use custom getter and setter functions.  Here are the functions for
getting and setting the :attr:`first` attribute::

   Noddy_getfirst(Noddy *self, void *closure)
   {
       Py_INCREF(self->first);
       return self->first;
   }

   static int
   Noddy_setfirst(Noddy *self, PyObject *value, void *closure)
   {
     if (value == NULL) {
       PyErr_SetString(PyExc_TypeError, "Cannot delete the first attribute");
       return -1;
     }

     if (! PyUnicode_Check(value)) {
       PyErr_SetString(PyExc_TypeError,
                       "The first attribute value must be a str");
       return -1;
     }

     Py_DECREF(self->first);
     Py_INCREF(value);
     self->first = value;

     return 0;
   }

The getter function is passed a :class:`Noddy` object and a "closure", which is
void pointer. In this case, the closure is ignored. (The closure supports an
advanced usage in which definition data is passed to the getter and setter. This
could, for example, be used to allow a single set of getter and setter functions
that decide the attribute to get or set based on data in the closure.)

The setter function is passed the :class:`Noddy` object, the new value, and the
closure. The new value may be *NULL*, in which case the attribute is being
deleted.  In our setter, we raise an error if the attribute is deleted or if the
attribute value is not a string.

We create an array of :c:type:`PyGetSetDef` structures::

   static PyGetSetDef Noddy_getseters[] = {
       {"first",
        (getter)Noddy_getfirst, (setter)Noddy_setfirst,
        "first name",
        NULL},
       {"last",
        (getter)Noddy_getlast, (setter)Noddy_setlast,
        "last name",
        NULL},
       {NULL}  /* Sentinel */
   };

and register it in the :c:member:`~PyTypeObject.tp_getset` slot::

   Noddy_getseters,           /* tp_getset */

to register our attribute getters and setters.

The last item in a :c:type:`PyGetSetDef` structure is the closure mentioned
above. In this case, we aren't using the closure, so we just pass *NULL*.

We also remove the member definitions for these attributes::

   static PyMemberDef Noddy_members[] = {
       {"number", T_INT, offsetof(Noddy, number), 0,
        "noddy number"},
       {NULL}  /* Sentinel */
   };

We also need to update the :c:member:`~PyTypeObject.tp_init` handler to only allow strings [#]_ to
be passed::

   static int
   Noddy_init(Noddy *self, PyObject *args, PyObject *kwds)
   {
       PyObject *first=NULL, *last=NULL, *tmp;

       static char *kwlist[] = {"first", "last", "number", NULL};

       if (! PyArg_ParseTupleAndKeywords(args, kwds, "|SSi", kwlist,
                                         &first, &last,
                                         &self->number))
           return -1;

       if (first) {
           tmp = self->first;
           Py_INCREF(first);
           self->first = first;
           Py_DECREF(tmp);
       }

       if (last) {
           tmp = self->last;
           Py_INCREF(last);
           self->last = last;
           Py_DECREF(tmp);
       }

       return 0;
   }

With these changes, we can assure that the :attr:`first` and :attr:`last`
members are never *NULL* so we can remove checks for *NULL* values in almost all
cases. This means that most of the :c:func:`Py_XDECREF` calls can be converted to
:c:func:`Py_DECREF` calls. The only place we can't change these calls is in the
deallocator, where there is the possibility that the initialization of these
members failed in the constructor.

We also rename the module initialization function and module name in the
initialization function, as we did before, and we add an extra definition to the
:file:`setup.py` file.


Supporting cyclic garbage collection
------------------------------------

Python has a cyclic-garbage collector that can identify unneeded objects even
when their reference counts are not zero. This can happen when objects are
involved in cycles.  For example, consider::

   >>> l = []
   >>> l.append(l)
   >>> del l

In this example, we create a list that contains itself. When we delete it, it
still has a reference from itself. Its reference count doesn't drop to zero.
Fortunately, Python's cyclic-garbage collector will eventually figure out that
the list is garbage and free it.

In the second version of the :class:`Noddy` example, we allowed any kind of
object to be stored in the :attr:`first` or :attr:`last` attributes. [#]_ This
means that :class:`Noddy` objects can participate in cycles::

   >>> import noddy2
   >>> n = noddy2.Noddy()
   >>> l = [n]
   >>> n.first = l

This is pretty silly, but it gives us an excuse to add support for the
cyclic-garbage collector to the :class:`Noddy` example.  To support cyclic
garbage collection, types need to fill two slots and set a class flag that
enables these slots:

.. literalinclude:: ../includes/noddy4.c


The traversal method provides access to subobjects that could participate in
cycles::

   static int
   Noddy_traverse(Noddy *self, visitproc visit, void *arg)
   {
       int vret;

       if (self->first) {
           vret = visit(self->first, arg);
           if (vret != 0)
               return vret;
       }
       if (self->last) {
           vret = visit(self->last, arg);
           if (vret != 0)
               return vret;
       }

       return 0;
   }

For each subobject that can participate in cycles, we need to call the
:c:func:`visit` function, which is passed to the traversal method. The
:c:func:`visit` function takes as arguments the subobject and the extra argument
*arg* passed to the traversal method.  It returns an integer value that must be
returned if it is non-zero.

Python provides a :c:func:`Py_VISIT` macro that automates calling visit
functions.  With :c:func:`Py_VISIT`, :c:func:`Noddy_traverse` can be simplified::

   static int
   Noddy_traverse(Noddy *self, visitproc visit, void *arg)
   {
       Py_VISIT(self->first);
       Py_VISIT(self->last);
       return 0;
   }

.. note::

   Note that the :c:member:`~PyTypeObject.tp_traverse` implementation must name its arguments exactly
   *visit* and *arg* in order to use :c:func:`Py_VISIT`.  This is to encourage
   uniformity across these boring implementations.

We also need to provide a method for clearing any subobjects that can
participate in cycles.  We implement the method and reimplement the deallocator
to use it::

   static int
   Noddy_clear(Noddy *self)
   {
       PyObject *tmp;

       tmp = self->first;
       self->first = NULL;
       Py_XDECREF(tmp);

       tmp = self->last;
       self->last = NULL;
       Py_XDECREF(tmp);

       return 0;
   }

   static void
   Noddy_dealloc(Noddy* self)
   {
       Noddy_clear(self);
       Py_TYPE(self)->tp_free((PyObject*)self);
   }

Notice the use of a temporary variable in :c:func:`Noddy_clear`. We use the
temporary variable so that we can set each member to *NULL* before decrementing
its reference count.  We do this because, as was discussed earlier, if the
reference count drops to zero, we might cause code to run that calls back into
the object.  In addition, because we now support garbage collection, we also
have to worry about code being run that triggers garbage collection.  If garbage
collection is run, our :c:member:`~PyTypeObject.tp_traverse` handler could get called. We can't
take a chance of having :c:func:`Noddy_traverse` called when a member's reference
count has dropped to zero and its value hasn't been set to *NULL*.

Python provides a :c:func:`Py_CLEAR` that automates the careful decrementing of
reference counts.  With :c:func:`Py_CLEAR`, the :c:func:`Noddy_clear` function can
be simplified::

   static int
   Noddy_clear(Noddy *self)
   {
       Py_CLEAR(self->first);
       Py_CLEAR(self->last);
       return 0;
   }

Finally, we add the :const:`Py_TPFLAGS_HAVE_GC` flag to the class flags::

   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /* tp_flags */

That's pretty much it.  If we had written custom :c:member:`~PyTypeObject.tp_alloc` or
:c:member:`~PyTypeObject.tp_free` slots, we'd need to modify them for cyclic-garbage collection.
Most extensions will use the versions automatically provided.


Subclassing other types
-----------------------

It is possible to create new extension types that are derived from existing
types. It is easiest to inherit from the built in types, since an extension can
easily use the :class:`PyTypeObject` it needs. It can be difficult to share
these :class:`PyTypeObject` structures between extension modules.

In this example we will create a :class:`Shoddy` type that inherits from the
built-in :class:`list` type. The new type will be completely compatible with
regular lists, but will have an additional :meth:`increment` method that
increases an internal counter. ::

   >>> import shoddy
   >>> s = shoddy.Shoddy(range(3))
   >>> s.extend(s)
   >>> print(len(s))
   6
   >>> print(s.increment())
   1
   >>> print(s.increment())
   2

.. literalinclude:: ../includes/shoddy.c


As you can see, the source code closely resembles the :class:`Noddy` examples in
previous sections. We will break down the main differences between them. ::

   typedef struct {
       PyListObject list;
       int state;
   } Shoddy;

The primary difference for derived type objects is that the base type's object
structure must be the first value. The base type will already include the
:c:func:`PyObject_HEAD` at the beginning of its structure.

When a Python object is a :class:`Shoddy` instance, its *PyObject\** pointer can
be safely cast to both *PyListObject\** and *Shoddy\**. ::

   static int
   Shoddy_init(Shoddy *self, PyObject *args, PyObject *kwds)
   {
       if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
          return -1;
       self->state = 0;
       return 0;
   }

In the :attr:`__init__` method for our type, we can see how to call through to
the :attr:`__init__` method of the base type.

This pattern is important when writing a type with custom :attr:`new` and
:attr:`dealloc` methods. The :attr:`new` method should not actually create the
memory for the object with :c:member:`~PyTypeObject.tp_alloc`, that will be handled by the base
class when calling its :c:member:`~PyTypeObject.tp_new`.

When filling out the :c:func:`PyTypeObject` for the :class:`Shoddy` type, you see
a slot for :c:func:`tp_base`. Due to cross platform compiler issues, you can't
fill that field directly with the :c:func:`PyList_Type`; it can be done later in
the module's :c:func:`init` function. ::

   PyMODINIT_FUNC
   PyInit_shoddy(void)
   {
       PyObject *m;

       ShoddyType.tp_base = &PyList_Type;
       if (PyType_Ready(&ShoddyType) < 0)
           return NULL;

       m = PyModule_Create(&shoddymodule);
       if (m == NULL)
           return NULL;

       Py_INCREF(&ShoddyType);
       PyModule_AddObject(m, "Shoddy", (PyObject *) &ShoddyType);
       return m;
   }

Before calling :c:func:`PyType_Ready`, the type structure must have the
:c:member:`~PyTypeObject.tp_base` slot filled in. When we are deriving a new type, it is not
necessary to fill out the :c:member:`~PyTypeObject.tp_alloc` slot with :c:func:`PyType_GenericNew`
-- the allocate function from the base type will be inherited.

After that, calling :c:func:`PyType_Ready` and adding the type object to the
module is the same as with the basic :class:`Noddy` examples.


.. _dnt-type-methods:

Type Methods
============

This section aims to give a quick fly-by on the various type methods you can
implement and what they do.

Here is the definition of :c:type:`PyTypeObject`, with some fields only used in
debug builds omitted:

.. literalinclude:: ../includes/typestruct.h


Now that's a *lot* of methods.  Don't worry too much though - if you have a type
you want to define, the chances are very good that you will only implement a
handful of these.

As you probably expect by now, we're going to go over this and give more
information about the various handlers.  We won't go in the order they are
defined in the structure, because there is a lot of historical baggage that
impacts the ordering of the fields; be sure your type initialization keeps the
fields in the right order!  It's often easiest to find an example that includes
all the fields you need (even if they're initialized to ``0``) and then change
the values to suit your new type. ::

   const char *tp_name; /* For printing */

The name of the type - as mentioned in the last section, this will appear in
various places, almost entirely for diagnostic purposes. Try to choose something
that will be helpful in such a situation! ::

   Py_ssize_t tp_basicsize, tp_itemsize; /* For allocation */

These fields tell the runtime how much memory to allocate when new objects of
this type are created.  Python has some built-in support for variable length
structures (think: strings, lists) which is where the :c:member:`~PyTypeObject.tp_itemsize` field
comes in.  This will be dealt with later. ::

   const char *tp_doc;

Here you can put a string (or its address) that you want returned when the
Python script references ``obj.__doc__`` to retrieve the doc string.

Now we come to the basic type methods---the ones most extension types will
implement.


Finalization and De-allocation
------------------------------

.. index::
   single: object; deallocation
   single: deallocation, object
   single: object; finalization
   single: finalization, of objects

::

   destructor tp_dealloc;

This function is called when the reference count of the instance of your type is
reduced to zero and the Python interpreter wants to reclaim it.  If your type
has memory to free or other clean-up to perform, you can put it here.  The
object itself needs to be freed here as well.  Here is an example of this
function::

   static void
   newdatatype_dealloc(newdatatypeobject * obj)
   {
       free(obj->obj_UnderlyingDatatypePtr);
       Py_TYPE(obj)->tp_free(obj);
   }

.. index::
   single: PyErr_Fetch()
   single: PyErr_Restore()

One important requirement of the deallocator function is that it leaves any
pending exceptions alone.  This is important since deallocators are frequently
called as the interpreter unwinds the Python stack; when the stack is unwound
due to an exception (rather than normal returns), nothing is done to protect the
deallocators from seeing that an exception has already been set.  Any actions
which a deallocator performs which may cause additional Python code to be
executed may detect that an exception has been set.  This can lead to misleading
errors from the interpreter.  The proper way to protect against this is to save
a pending exception before performing the unsafe action, and restoring it when
done.  This can be done using the :c:func:`PyErr_Fetch` and
:c:func:`PyErr_Restore` functions::

   static void
   my_dealloc(PyObject *obj)
   {
       MyObject *self = (MyObject *) obj;
       PyObject *cbresult;

       if (self->my_callback != NULL) {
           PyObject *err_type, *err_value, *err_traceback;

           /* This saves the current exception state */
           PyErr_Fetch(&err_type, &err_value, &err_traceback);

           cbresult = PyObject_CallObject(self->my_callback, NULL);
           if (cbresult == NULL)
               PyErr_WriteUnraisable(self->my_callback);
           else
               Py_DECREF(cbresult);

           /* This restores the saved exception state */
           PyErr_Restore(err_type, err_value, err_traceback);

           Py_DECREF(self->my_callback);
       }
       Py_TYPE(obj)->tp_free((PyObject*)self);
   }

.. note::
   There are limitations to what you can safely do in a deallocator function.
   First, if your type supports garbage collection (using :c:member:`~PyTypeObject.tp_traverse`
   and/or :c:member:`~PyTypeObject.tp_clear`), some of the object's members can have been
   cleared or finalized by the time :c:member:`~PyTypeObject.tp_dealloc` is called.  Second, in
   :c:member:`~PyTypeObject.tp_dealloc`, your object is in an unstable state: its reference
   count is equal to zero.  Any call to a non-trivial object or API (as in the
   example above) might end up calling :c:member:`~PyTypeObject.tp_dealloc` again, causing a
   double free and a crash.

   Starting with Python 3.4, it is recommended not to put any complex
   finalization code in :c:member:`~PyTypeObject.tp_dealloc`, and instead use the new
   :c:member:`~PyTypeObject.tp_finalize` type method.

   .. seealso::
      :pep:`442` explains the new finalization scheme.

.. index::
   single: string; object representation
   builtin: repr

Object Presentation
-------------------

In Python, there are two ways to generate a textual representation of an object:
the :func:`repr` function, and the :func:`str` function.  (The :func:`print`
function just calls :func:`str`.)  These handlers are both optional.

::

   reprfunc tp_repr;
   reprfunc tp_str;

The :c:member:`~PyTypeObject.tp_repr` handler should return a string object containing a
representation of the instance for which it is called.  Here is a simple
example::

   static PyObject *
   newdatatype_repr(newdatatypeobject * obj)
   {
       return PyUnicode_FromFormat("Repr-ified_newdatatype{{size:\%d}}",
                                   obj->obj_UnderlyingDatatypePtr->size);
   }

If no :c:member:`~PyTypeObject.tp_repr` handler is specified, the interpreter will supply a
representation that uses the type's :c:member:`~PyTypeObject.tp_name` and a uniquely-identifying
value for the object.

The :c:member:`~PyTypeObject.tp_str` handler is to :func:`str` what the :c:member:`~PyTypeObject.tp_repr` handler
described above is to :func:`repr`; that is, it is called when Python code calls
:func:`str` on an instance of your object.  Its implementation is very similar
to the :c:member:`~PyTypeObject.tp_repr` function, but the resulting string is intended for human
consumption.  If :c:member:`~PyTypeObject.tp_str` is not specified, the :c:member:`~PyTypeObject.tp_repr` handler is
used instead.

Here is a simple example::

   static PyObject *
   newdatatype_str(newdatatypeobject * obj)
   {
       return PyUnicode_FromFormat("Stringified_newdatatype{{size:\%d}}",
                                   obj->obj_UnderlyingDatatypePtr->size);
   }



Attribute Management
--------------------

For every object which can support attributes, the corresponding type must
provide the functions that control how the attributes are resolved.  There needs
to be a function which can retrieve attributes (if any are defined), and another
to set attributes (if setting attributes is allowed).  Removing an attribute is
a special case, for which the new value passed to the handler is *NULL*.

Python supports two pairs of attribute handlers; a type that supports attributes
only needs to implement the functions for one pair.  The difference is that one
pair takes the name of the attribute as a :c:type:`char\*`, while the other
accepts a :c:type:`PyObject\*`.  Each type can use whichever pair makes more
sense for the implementation's convenience. ::

   getattrfunc  tp_getattr;        /* char * version */
   setattrfunc  tp_setattr;
   /* ... */
   getattrofunc tp_getattro;       /* PyObject * version */
   setattrofunc tp_setattro;

If accessing attributes of an object is always a simple operation (this will be
explained shortly), there are generic implementations which can be used to
provide the :c:type:`PyObject\*` version of the attribute management functions.
The actual need for type-specific attribute handlers almost completely
disappeared starting with Python 2.2, though there are many examples which have
not been updated to use some of the new generic mechanism that is available.


.. _generic-attribute-management:

Generic Attribute Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Most extension types only use *simple* attributes.  So, what makes the
attributes simple?  There are only a couple of conditions that must be met:

#. The name of the attributes must be known when :c:func:`PyType_Ready` is
   called.

#. No special processing is needed to record that an attribute was looked up or
   set, nor do actions need to be taken based on the value.

Note that this list does not place any restrictions on the values of the
attributes, when the values are computed, or how relevant data is stored.

When :c:func:`PyType_Ready` is called, it uses three tables referenced by the
type object to create :term:`descriptor`\s which are placed in the dictionary of the
type object.  Each descriptor controls access to one attribute of the instance
object.  Each of the tables is optional; if all three are *NULL*, instances of
the type will only have attributes that are inherited from their base type, and
should leave the :c:member:`~PyTypeObject.tp_getattro` and :c:member:`~PyTypeObject.tp_setattro` fields *NULL* as
well, allowing the base type to handle attributes.

The tables are declared as three fields of the type object::

   struct PyMethodDef *tp_methods;
   struct PyMemberDef *tp_members;
   struct PyGetSetDef *tp_getset;

If :c:member:`~PyTypeObject.tp_methods` is not *NULL*, it must refer to an array of
:c:type:`PyMethodDef` structures.  Each entry in the table is an instance of this
structure::

   typedef struct PyMethodDef {
       const char  *ml_name;       /* method name */
       PyCFunction  ml_meth;       /* implementation function */
       int          ml_flags;      /* flags */
       const char  *ml_doc;        /* docstring */
   } PyMethodDef;

One entry should be defined for each method provided by the type; no entries are
needed for methods inherited from a base type.  One additional entry is needed
at the end; it is a sentinel that marks the end of the array.  The
:attr:`ml_name` field of the sentinel must be *NULL*.

The second table is used to define attributes which map directly to data stored
in the instance.  A variety of primitive C types are supported, and access may
be read-only or read-write.  The structures in the table are defined as::

   typedef struct PyMemberDef {
       char *name;
       int   type;
       int   offset;
       int   flags;
       char *doc;
   } PyMemberDef;

For each entry in the table, a :term:`descriptor` will be constructed and added to the
type which will be able to extract a value from the instance structure.  The
:attr:`type` field should contain one of the type codes defined in the
:file:`structmember.h` header; the value will be used to determine how to
convert Python values to and from C values.  The :attr:`flags` field is used to
store flags which control how the attribute can be accessed.

The following flag constants are defined in :file:`structmember.h`; they may be
combined using bitwise-OR.

+---------------------------+----------------------------------------------+
| Constant                  | Meaning                                      |
+===========================+==============================================+
| :const:`READONLY`         | Never writable.                              |
+---------------------------+----------------------------------------------+
| :const:`READ_RESTRICTED`  | Not readable in restricted mode.             |
+---------------------------+----------------------------------------------+
| :const:`WRITE_RESTRICTED` | Not writable in restricted mode.             |
+---------------------------+----------------------------------------------+
| :const:`RESTRICTED`       | Not readable or writable in restricted mode. |
+---------------------------+----------------------------------------------+

.. index::
   single: READONLY
   single: READ_RESTRICTED
   single: WRITE_RESTRICTED
   single: RESTRICTED

An interesting advantage of using the :c:member:`~PyTypeObject.tp_members` table to build
descriptors that are used at runtime is that any attribute defined this way can
have an associated doc string simply by providing the text in the table.  An
application can use the introspection API to retrieve the descriptor from the
class object, and get the doc string using its :attr:`__doc__` attribute.

As with the :c:member:`~PyTypeObject.tp_methods` table, a sentinel entry with a :attr:`name` value
of *NULL* is required.

.. XXX Descriptors need to be explained in more detail somewhere, but not here.

   Descriptor objects have two handler functions which correspond to the
   \member{tp_getattro} and \member{tp_setattro} handlers.  The
   \method{__get__()} handler is a function which is passed the descriptor,
   instance, and type objects, and returns the value of the attribute, or it
   returns \NULL{} and sets an exception.  The \method{__set__()} handler is
   passed the descriptor, instance, type, and new value;


Type-specific Attribute Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For simplicity, only the :c:type:`char\*` version will be demonstrated here; the
type of the name parameter is the only difference between the :c:type:`char\*`
and :c:type:`PyObject\*` flavors of the interface. This example effectively does
the same thing as the generic example above, but does not use the generic
support added in Python 2.2.  It explains how the handler functions are
called, so that if you do need to extend their functionality, you'll understand
what needs to be done.

The :c:member:`~PyTypeObject.tp_getattr` handler is called when the object requires an attribute
look-up.  It is called in the same situations where the :meth:`__getattr__`
method of a class would be called.

Here is an example::

   static PyObject *
   newdatatype_getattr(newdatatypeobject *obj, char *name)
   {
       if (strcmp(name, "data") == 0)
       {
           return PyLong_FromLong(obj->data);
       }

       PyErr_Format(PyExc_AttributeError,
                    "'%.50s' object has no attribute '%.400s'",
                    tp->tp_name, name);
       return NULL;
   }

The :c:member:`~PyTypeObject.tp_setattr` handler is called when the :meth:`__setattr__` or
:meth:`__delattr__` method of a class instance would be called.  When an
attribute should be deleted, the third parameter will be *NULL*.  Here is an
example that simply raises an exception; if this were really all you wanted, the
:c:member:`~PyTypeObject.tp_setattr` handler should be set to *NULL*. ::

   static int
   newdatatype_setattr(newdatatypeobject *obj, char *name, PyObject *v)
   {
       (void)PyErr_Format(PyExc_RuntimeError, "Read-only attribute: \%s", name);
       return -1;
   }

Object Comparison
-----------------

::

   richcmpfunc tp_richcompare;

The :c:member:`~PyTypeObject.tp_richcompare` handler is called when comparisons are needed.  It is
analogous to the :ref:`rich comparison methods <richcmpfuncs>`, like
:meth:`__lt__`, and also called by :c:func:`PyObject_RichCompare` and
:c:func:`PyObject_RichCompareBool`.

This function is called with two Python objects and the operator as arguments,
where the operator is one of ``Py_EQ``, ``Py_NE``, ``Py_LE``, ``Py_GT``,
``Py_LT`` or ``Py_GT``.  It should compare the two objects with respect to the
specified operator and return ``Py_True`` or ``Py_False`` if the comparison is
successful, ``Py_NotImplemented`` to indicate that comparison is not
implemented and the other object's comparison method should be tried, or *NULL*
if an exception was set.

Here is a sample implementation, for a datatype that is considered equal if the
size of an internal pointer is equal::

   static PyObject *
   newdatatype_richcmp(PyObject *obj1, PyObject *obj2, int op)
   {
       PyObject *result;
       int c, size1, size2;

       /* code to make sure that both arguments are of type
          newdatatype omitted */

       size1 = obj1->obj_UnderlyingDatatypePtr->size;
       size2 = obj2->obj_UnderlyingDatatypePtr->size;

       switch (op) {
       case Py_LT: c = size1 <  size2; break;
       case Py_LE: c = size1 <= size2; break;
       case Py_EQ: c = size1 == size2; break;
       case Py_NE: c = size1 != size2; break;
       case Py_GT: c = size1 >  size2; break;
       case Py_GE: c = size1 >= size2; break;
       }
       result = c ? Py_True : Py_False;
       Py_INCREF(result);
       return result;
    }


Abstract Protocol Support
-------------------------

Python supports a variety of *abstract* 'protocols;' the specific interfaces
provided to use these interfaces are documented in :ref:`abstract`.


A number of these abstract interfaces were defined early in the development of
the Python implementation.  In particular, the number, mapping, and sequence
protocols have been part of Python since the beginning.  Other protocols have
been added over time.  For protocols which depend on several handler routines
from the type implementation, the older protocols have been defined as optional
blocks of handlers referenced by the type object.  For newer protocols there are
additional slots in the main type object, with a flag bit being set to indicate
that the slots are present and should be checked by the interpreter.  (The flag
bit does not indicate that the slot values are non-*NULL*. The flag may be set
to indicate the presence of a slot, but a slot may still be unfilled.) ::

   PyNumberMethods   *tp_as_number;
   PySequenceMethods *tp_as_sequence;
   PyMappingMethods  *tp_as_mapping;

If you wish your object to be able to act like a number, a sequence, or a
mapping object, then you place the address of a structure that implements the C
type :c:type:`PyNumberMethods`, :c:type:`PySequenceMethods`, or
:c:type:`PyMappingMethods`, respectively. It is up to you to fill in this
structure with appropriate values. You can find examples of the use of each of
these in the :file:`Objects` directory of the Python source distribution. ::

   hashfunc tp_hash;

This function, if you choose to provide it, should return a hash number for an
instance of your data type. Here is a moderately pointless example::

   static long
   newdatatype_hash(newdatatypeobject *obj)
   {
       long result;
       result = obj->obj_UnderlyingDatatypePtr->size;
       result = result * 3;
       return result;
   }

::

   ternaryfunc tp_call;

This function is called when an instance of your data type is "called", for
example, if ``obj1`` is an instance of your data type and the Python script
contains ``obj1('hello')``, the :c:member:`~PyTypeObject.tp_call` handler is invoked.

This function takes three arguments:

#. *arg1* is the instance of the data type which is the subject of the call. If
   the call is ``obj1('hello')``, then *arg1* is ``obj1``.

#. *arg2* is a tuple containing the arguments to the call.  You can use
   :c:func:`PyArg_ParseTuple` to extract the arguments.

#. *arg3* is a dictionary of keyword arguments that were passed. If this is
   non-*NULL* and you support keyword arguments, use
   :c:func:`PyArg_ParseTupleAndKeywords` to extract the arguments.  If you do not
   want to support keyword arguments and this is non-*NULL*, raise a
   :exc:`TypeError` with a message saying that keyword arguments are not supported.

Here is a desultory example of the implementation of the call function. ::

   /* Implement the call function.
    *    obj1 is the instance receiving the call.
    *    obj2 is a tuple containing the arguments to the call, in this
    *         case 3 strings.
    */
   static PyObject *
   newdatatype_call(newdatatypeobject *obj, PyObject *args, PyObject *other)
   {
       PyObject *result;
       char *arg1;
       char *arg2;
       char *arg3;

       if (!PyArg_ParseTuple(args, "sss:call", &arg1, &arg2, &arg3)) {
           return NULL;
       }
       result = PyUnicode_FromFormat(
           "Returning -- value: [\%d] arg1: [\%s] arg2: [\%s] arg3: [\%s]\n",
           obj->obj_UnderlyingDatatypePtr->size,
           arg1, arg2, arg3);
       return result;
   }

::

   /* Iterators */
   getiterfunc tp_iter;
   iternextfunc tp_iternext;

These functions provide support for the iterator protocol.  Any object which
wishes to support iteration over its contents (which may be generated during
iteration) must implement the ``tp_iter`` handler.  Objects which are returned
by a ``tp_iter`` handler must implement both the ``tp_iter`` and ``tp_iternext``
handlers. Both handlers take exactly one parameter, the instance for which they
are being called, and return a new reference.  In the case of an error, they
should set an exception and return *NULL*.

For an object which represents an iterable collection, the ``tp_iter`` handler
must return an iterator object.  The iterator object is responsible for
maintaining the state of the iteration.  For collections which can support
multiple iterators which do not interfere with each other (as lists and tuples
do), a new iterator should be created and returned.  Objects which can only be
iterated over once (usually due to side effects of iteration) should implement
this handler by returning a new reference to themselves, and should also
implement the ``tp_iternext`` handler.  File objects are an example of such an
iterator.

Iterator objects should implement both handlers.  The ``tp_iter`` handler should
return a new reference to the iterator (this is the same as the ``tp_iter``
handler for objects which can only be iterated over destructively).  The
``tp_iternext`` handler should return a new reference to the next object in the
iteration if there is one.  If the iteration has reached the end, it may return
*NULL* without setting an exception or it may set :exc:`StopIteration`; avoiding
the exception can yield slightly better performance.  If an actual error occurs,
it should set an exception and return *NULL*.


.. _weakref-support:

Weak Reference Support
----------------------

One of the goals of Python's weak-reference implementation is to allow any type
to participate in the weak reference mechanism without incurring the overhead on
those objects which do not benefit by weak referencing (such as numbers).

For an object to be weakly referencable, the extension must include a
:c:type:`PyObject\*` field in the instance structure for the use of the weak
reference mechanism; it must be initialized to *NULL* by the object's
constructor.  It must also set the :c:member:`~PyTypeObject.tp_weaklistoffset` field of the
corresponding type object to the offset of the field. For example, the instance
type is defined with the following structure::

   typedef struct {
       PyObject_HEAD
       PyClassObject *in_class;       /* The class object */
       PyObject      *in_dict;        /* A dictionary */
       PyObject      *in_weakreflist; /* List of weak references */
   } PyInstanceObject;

The statically-declared type object for instances is defined this way::

   PyTypeObject PyInstance_Type = {
       PyVarObject_HEAD_INIT(&PyType_Type, 0)
       0,
       "module.instance",

       /* Lots of stuff omitted for brevity... */

       Py_TPFLAGS_DEFAULT,                         /* tp_flags */
       0,                                          /* tp_doc */
       0,                                          /* tp_traverse */
       0,                                          /* tp_clear */
       0,                                          /* tp_richcompare */
       offsetof(PyInstanceObject, in_weakreflist), /* tp_weaklistoffset */
   };

The type constructor is responsible for initializing the weak reference list to
*NULL*::

   static PyObject *
   instance_new() {
       /* Other initialization stuff omitted for brevity */

       self->in_weakreflist = NULL;

       return (PyObject *) self;
   }

The only further addition is that the destructor needs to call the weak
reference manager to clear any weak references.  This is only required if the
weak reference list is non-*NULL*::

   static void
   instance_dealloc(PyInstanceObject *inst)
   {
       /* Allocate temporaries if needed, but do not begin
          destruction just yet.
        */

       if (inst->in_weakreflist != NULL)
           PyObject_ClearWeakRefs((PyObject *) inst);

       /* Proceed with object destruction normally. */
   }


More Suggestions
----------------

Remember that you can omit most of these functions, in which case you provide
``0`` as a value.  There are type definitions for each of the functions you must
provide.  They are in :file:`object.h` in the Python include directory that
comes with the source distribution of Python.

In order to learn how to implement any specific method for your new data type,
do the following: Download and unpack the Python source distribution.  Go to
the :file:`Objects` directory, then search the C source files for ``tp_`` plus
the function you want (for example, ``tp_richcompare``).  You will find examples
of the function you want to implement.

When you need to verify that an object is an instance of the type you are
implementing, use the :c:func:`PyObject_TypeCheck` function. A sample of its use
might be something like the following::

   if (! PyObject_TypeCheck(some_object, &MyType)) {
       PyErr_SetString(PyExc_TypeError, "arg #1 not a mything");
       return NULL;
   }

.. rubric:: Footnotes

.. [#] This is true when we know that the object is a basic type, like a string or a
   float.

.. [#] We relied on this in the :c:member:`~PyTypeObject.tp_dealloc` handler in this example, because our
   type doesn't support garbage collection. Even if a type supports garbage
   collection, there are calls that can be made to "untrack" the object from
   garbage collection, however, these calls are advanced and not covered here.

.. [#] We now know that the first and last members are strings, so perhaps we could be
   less careful about decrementing their reference counts, however, we accept
   instances of string subclasses. Even though deallocating normal strings won't
   call back into our objects, we can't guarantee that deallocating an instance of
   a string subclass won't call back into our objects.

.. [#] Even in the third version, we aren't guaranteed to avoid cycles.  Instances of
   string subclasses are allowed and string subclasses could allow cycles even if
   normal strings don't.
