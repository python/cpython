
:mod:`dircache` --- Cached directory listings
=============================================

.. module:: dircache
   :synopsis: Return directory listing, with cache mechanism.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`dircache` module defines a function for reading directory listing
using a cache, and cache invalidation using the *mtime* of the directory.
Additionally, it defines a function to annotate directories by appending a
slash.

The :mod:`dircache` module defines the following functions:


.. function:: reset()

   Resets the directory cache.


.. function:: listdir(path)

   Return a directory listing of *path*, as gotten from :func:`os.listdir`. Note
   that unless *path* changes, further call to :func:`listdir` will not re-read the
   directory structure.

   Note that the list returned should be regarded as read-only. (Perhaps a future
   version should change it to return a tuple?)


.. function:: opendir(path)

   Same as :func:`listdir`. Defined for backwards compatibility.


.. function:: annotate(head, list)

   Assume *list* is a list of paths relative to *head*, and append, in place, a
   ``'/'`` to each path which points to a directory.

::

   >>> import dircache
   >>> a = dircache.listdir('/')
   >>> a = a[:] # Copy the return value so we can change 'a'
   >>> a
   ['bin', 'boot', 'cdrom', 'dev', 'etc', 'floppy', 'home', 'initrd', 'lib', 'lost+
   found', 'mnt', 'proc', 'root', 'sbin', 'tmp', 'usr', 'var', 'vmlinuz']
   >>> dircache.annotate('/', a)
   >>> a
   ['bin/', 'boot/', 'cdrom/', 'dev/', 'etc/', 'floppy/', 'home/', 'initrd/', 'lib/
   ', 'lost+found/', 'mnt/', 'proc/', 'root/', 'sbin/', 'tmp/', 'usr/', 'var/', 'vm
   linuz']

