:mod:`mhlib` --- Access to MH mailboxes
=======================================

.. module:: mhlib
   :synopsis: Manipulate MH mailboxes from Python.
   :deprecated:

.. deprecated:: 2.6
    The :mod:`mhlib` module has been removed in Python 3. Use the
    :mod:`mailbox` instead.

.. sectionauthor:: Skip Montanaro <skip@pobox.com>

The :mod:`mhlib` module provides a Python interface to MH folders and their
contents.

The module contains three basic classes, :class:`MH`, which represents a
particular collection of folders, :class:`Folder`, which represents a single
folder, and :class:`Message`, which represents a single message.


.. class:: MH([path[, profile]])

   :class:`MH` represents a collection of MH folders.


.. class:: Folder(mh, name)

   The :class:`Folder` class represents a single folder and its messages.


.. class:: Message(folder, number[, name])

   :class:`Message` objects represent individual messages in a folder.  The Message
   class is derived from :class:`mimetools.Message`.


.. _mh-objects:

MH Objects
----------

:class:`MH` instances have the following methods:


.. method:: MH.error(format[, ...])

   Print an error message -- can be overridden.


.. method:: MH.getprofile(key)

   Return a profile entry (``None`` if not set).


.. method:: MH.getpath()

   Return the mailbox pathname.


.. method:: MH.getcontext()

   Return the current folder name.


.. method:: MH.setcontext(name)

   Set the current folder name.


.. method:: MH.listfolders()

   Return a list of top-level folders.


.. method:: MH.listallfolders()

   Return a list of all folders.


.. method:: MH.listsubfolders(name)

   Return a list of direct subfolders of the given folder.


.. method:: MH.listallsubfolders(name)

   Return a list of all subfolders of the given folder.


.. method:: MH.makefolder(name)

   Create a new folder.


.. method:: MH.deletefolder(name)

   Delete a folder -- must have no subfolders.


.. method:: MH.openfolder(name)

   Return a new open folder object.


.. _mh-folder-objects:

Folder Objects
--------------

:class:`Folder` instances represent open folders and have the following methods:


.. method:: Folder.error(format[, ...])

   Print an error message -- can be overridden.


.. method:: Folder.getfullname()

   Return the folder's full pathname.


.. method:: Folder.getsequencesfilename()

   Return the full pathname of the folder's sequences file.


.. method:: Folder.getmessagefilename(n)

   Return the full pathname of message *n* of the folder.


.. method:: Folder.listmessages()

   Return a list of messages in the folder (as numbers).


.. method:: Folder.getcurrent()

   Return the current message number.


.. method:: Folder.setcurrent(n)

   Set the current message number to *n*.


.. method:: Folder.parsesequence(seq)

   Parse msgs syntax into list of messages.


.. method:: Folder.getlast()

   Get last message, or ``0`` if no messages are in the folder.


.. method:: Folder.setlast(n)

   Set last message (internal use only).


.. method:: Folder.getsequences()

   Return dictionary of sequences in folder.  The sequence names are used  as keys,
   and the values are the lists of message numbers in the sequences.


.. method:: Folder.putsequences(dict)

   Return dictionary of sequences in folder name: list.


.. method:: Folder.removemessages(list)

   Remove messages in list from folder.


.. method:: Folder.refilemessages(list, tofolder)

   Move messages in list to other folder.


.. method:: Folder.movemessage(n, tofolder, ton)

   Move one message to a given destination in another folder.


.. method:: Folder.copymessage(n, tofolder, ton)

   Copy one message to a given destination in another folder.


.. _mh-message-objects:

Message Objects
---------------

The :class:`Message` class adds one method to those of
:class:`mimetools.Message`:


.. method:: Message.openmessage(n)

   Return a new open message object (costs a file descriptor).

