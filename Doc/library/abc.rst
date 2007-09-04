
:mod:`abc` --- Abstract Base Classes
====================================

.. module:: abc
   :synopsis: Abstract base classes according to PEP 3119.
.. moduleauthor:: Guido van Rossum
.. sectionauthor:: Georg Brandl
.. much of the content adapted from docstrings

This module provides the infrastructure for defining abstract base classes
(ABCs) in Python, as outlined in :pep:`3119`.

Concrete base ABCs to derive from can be found in the :mod:`collections` module.


The module provides the following class:

.. class:: ABCMeta

   Metaclass for defining Abstract Base Classes (ABCs).

   Use this metaclass to create an ABC.  An ABC can be subclassed directly, and
   then acts as a mix-in class.  You can also register unrelated concrete
   classes (even built-in classes) and unrelated ABCs as "virtual subclasses" --
   these and their descendants will be considered subclasses of the registering
   ABC by the built-in :func:`issubclass` function, but the registering ABC
   won't show up in their MRO (Method Resolution Order) nor will method
   implementations defined by the registering ABC be callable (not even via
   :func:`super`).

   Classes created with a metaclass of :class:`ABCMeta` have the following method:

   .. method:: register(subclass)

      Register *subclass* as a "virtual subclass" of this ABC.  From now on,
      ``issubclass(subclass, ABC)`` is true.


   You can also override this method in an abstract base class:

   .. method:: __subclasshook__(subclass)

      (Must be defined as a class method.)

      Check whether *subclass* is considered a subclass of this ABC.  This means
      that you can customize the behavior of ``issubclass`` further without the
      need to call :meth:`register` on every class you want to consider a
      subclass of the ABC.

      This method should return ``True``, ``False`` or ``NotImplemented``.  If
      it returns ``True``, the *subclass* is considered a subclass of this ABC.
      If it returns ``False``, the *subclass* is not considered a subclass of
      this ABC, even if it would normally be one.  If it returns
      ``NotImplemented``, the subclass check is continued with the usual
      mechanism.


   To demonstrate these concepts, look at this example ABC definition::

      class MyIterator:
          pass

      class Iterator(metaclass=ABCMeta):

          @abstractmethod
          def __next__(self):
              raise StopIteration

          def __iter__(self):
              return self

          @classmethod
          def __subclasshook__(cls, C):
              if cls is Iterator:
                  if any("__next__" in B.__dict__ for B in C.__mro__):
                      return True
              return NotImplemented

      Iterator.register(MyIterator)

   The ABC ``Iterator`` defines the two standard iterator methods:
   :meth:`__iter__` and :meth:`__next__`.  The :meth:`__iter__` method is given
   a default implementation, while the :meth:`__next__` method is abstract.

   .. XXX why is an implementation given then?

   The :meth:`__subclasshook__` class method defined here says that any class
   that has a :meth:`__next__` method in its :attr:`__dict__` (or in that of one
   of its subclasses, accessed via the :attr:`__mro__`) is considered an
   ``Iterator`` too.

   Finally, the last line makes ``MyIterator`` a virtual subclass of
   ``Iterator``, even though it does not define a :meth:`__next__` method.
   (Of course, this doesn't make much sense in this context.)

   .. XXX perhaps find better example


It also provides the following decorators:

.. function:: abstractmethod(function)

   A decorator indicating abstract methods.

   Requires that the metaclass is :class:`ABCMeta` or derived from it.  A class
   that has a metaclass derived from :class:`ABCMeta` cannot be instantiated
   unless all of its abstract methods are overridden.  The abstract methods can
   be called using any of the the normal 'super' call mechanisms.

   Usage::

      class C(metaclass=ABCMeta):
          @abstractmethod
          def my_abstract_method(self, ...):
              ...


.. function:: abstractproperty(property)

   A decorator indicating abstract properties.

   Requires that the metaclass is :class:`ABCMeta` or derived from it.  A class
   that has a metaclass derived from :class:`ABCMeta` cannot be instantiated
   unless all of its abstract properties are overridden.  The abstract
   properties can be called using any of the the normal 'super' call mechanisms.

   Usage::

      class C(metaclass=ABCMeta):
          @abstractproperty
          def my_abstract_property(self):
              ...

   This defines a read-only property; you can also define a read-write abstract
   property using the 'long' form of property declaration::

      class C(metaclass=ABCMeta):
          def getx(self): ...
          def setx(self, value): ...
          x = abstractproperty(getx, setx)

