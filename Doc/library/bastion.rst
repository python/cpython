
:mod:`Bastion` --- Restricting access to objects
================================================

.. module:: Bastion
   :synopsis: Providing restricted access to objects.
.. moduleauthor:: Barry Warsaw <bwarsaw@python.org>


.. versionchanged:: 2.3
   Disabled module.

.. warning::

   The documentation has been left in place to help in reading old code that uses
   the module.

According to the dictionary, a bastion is "a fortified area or position", or
"something that is considered a stronghold."  It's a suitable name for this
module, which provides a way to forbid access to certain attributes of an
object.  It must always be used with the :mod:`rexec` module, in order to allow
restricted-mode programs access to certain safe attributes of an object, while
denying access to other, unsafe attributes.

.. I'm concerned that the word 'bastion' won't be understood by people
.. for whom English is a second language, making the module name
.. somewhat mysterious.  Thus, the brief definition... --amk

.. I've punted on the issue of documenting keyword arguments for now.


.. function:: Bastion(object[, filter[, name[, class]]])

   Protect the object *object*, returning a bastion for the object.  Any attempt to
   access one of the object's attributes will have to be approved by the *filter*
   function; if the access is denied an :exc:`AttributeError` exception will be
   raised.

   If present, *filter* must be a function that accepts a string containing an
   attribute name, and returns true if access to that attribute will be permitted;
   if *filter* returns false, the access is denied.  The default filter denies
   access to any function beginning with an underscore (``'_'``).  The bastion's
   string representation will be ``<Bastion for name>`` if a value for *name* is
   provided; otherwise, ``repr(object)`` will be used.

   *class*, if present, should be a subclass of :class:`BastionClass`;  see the
   code in :file:`bastion.py` for the details.  Overriding the default
   :class:`BastionClass` will rarely be required.


.. class:: BastionClass(getfunc, name)

   Class which actually implements bastion objects.  This is the default class used
   by :func:`Bastion`.  The *getfunc* parameter is a function which returns the
   value of an attribute which should be exposed to the restricted execution
   environment when called with the name of the attribute as the only parameter.
   *name* is used to construct the :func:`repr` of the :class:`BastionClass`
   instance.

