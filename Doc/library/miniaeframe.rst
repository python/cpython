
:mod:`MiniAEFrame` --- Open Scripting Architecture server support
=================================================================

.. module:: MiniAEFrame
   :platform: Mac
   :synopsis: Support to act as an Open Scripting Architecture (OSA) server ("Apple Events").


.. index::
   single: Open Scripting Architecture
   single: AppleEvents
   module: FrameWork

The module :mod:`MiniAEFrame` provides a framework for an application that can
function as an Open Scripting Architecture  (OSA) server, i.e. receive and
process AppleEvents. It can be used in conjunction with :mod:`FrameWork` or
standalone. As an example, it is used in :program:`PythonCGISlave`.

The :mod:`MiniAEFrame` module defines the following classes:


.. class:: AEServer()

   A class that handles AppleEvent dispatch. Your application should subclass this
   class together with either :class:`MiniApplication` or
   :class:`FrameWork.Application`. Your :meth:`__init__` method should call the
   :meth:`__init__` method for both classes.


.. class:: MiniApplication()

   A class that is more or less compatible with :class:`FrameWork.Application` but
   with less functionality. Its event loop supports the apple menu, command-dot and
   AppleEvents; other events are passed on to the Python interpreter and/or Sioux.
   Useful if your application wants to use :class:`AEServer` but does not provide
   its own windows, etc.


.. _aeserver-objects:

AEServer Objects
----------------


.. method:: AEServer.installaehandler(classe, type, callback)

   Installs an AppleEvent handler. *classe* and *type* are the four-character OSA
   Class and Type designators, ``'****'`` wildcards are allowed. When a matching
   AppleEvent is received the parameters are decoded and your callback is invoked.


.. method:: AEServer.callback(_object, **kwargs)

   Your callback is called with the OSA Direct Object as first positional
   parameter. The other parameters are passed as keyword arguments, with the
   4-character designator as name. Three extra keyword parameters are passed:
   ``_class`` and ``_type`` are the Class and Type designators and ``_attributes``
   is a dictionary with the AppleEvent attributes.

   The return value of your method is packed with :func:`aetools.packevent` and
   sent as reply.

Note that there are some serious problems with the current design. AppleEvents
which have non-identifier 4-character designators for arguments are not
implementable, and it is not possible to return an error to the originator. This
will be addressed in a future release.

