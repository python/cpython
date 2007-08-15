
:mod:`filecmp` --- File and Directory Comparisons
=================================================

.. module:: filecmp
   :synopsis: Compare files efficiently.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`filecmp` module defines functions to compare files and directories,
with various optional time/correctness trade-offs.

The :mod:`filecmp` module defines the following functions:


.. function:: cmp(f1, f2[, shallow])

   Compare the files named *f1* and *f2*, returning ``True`` if they seem equal,
   ``False`` otherwise.

   Unless *shallow* is given and is false, files with identical :func:`os.stat`
   signatures are taken to be equal.

   Files that were compared using this function will not be compared again unless
   their :func:`os.stat` signature changes.

   Note that no external programs are called from this function, giving it
   portability and efficiency.


.. function:: cmpfiles(dir1, dir2, common[, shallow])

   Returns three lists of file names: *match*, *mismatch*, *errors*.  *match*
   contains the list of files match in both directories, *mismatch* includes the
   names of those that don't, and *errros* lists the names of files which could not
   be compared.  Files may be listed in *errors* because the user may lack
   permission to read them or many other reasons, but always that the comparison
   could not be done for some reason.

   The *common* parameter is a list of file names found in both directories. The
   *shallow* parameter has the same meaning and default value as for
   :func:`filecmp.cmp`.

Example::

   >>> import filecmp
   >>> filecmp.cmp('undoc.rst', 'undoc.rst')
   True
   >>> filecmp.cmp('undoc.rst', 'index.rst')
   False


.. _dircmp-objects:

The :class:`dircmp` class
-------------------------

:class:`dircmp` instances are built using this constructor:


.. class:: dircmp(a, b[, ignore[, hide]])

   Construct a new directory comparison object, to compare the directories *a* and
   *b*. *ignore* is a list of names to ignore, and defaults to ``['RCS', 'CVS',
   'tags']``. *hide* is a list of names to hide, and defaults to ``[os.curdir,
   os.pardir]``.

The :class:`dircmp` class provides the following methods:


.. method:: dircmp.report()

   Print (to ``sys.stdout``) a comparison between *a* and *b*.


.. method:: dircmp.report_partial_closure()

   Print a comparison between *a* and *b* and common immediate subdirectories.


.. method:: dircmp.report_full_closure()

   Print a comparison between *a* and *b* and common  subdirectories (recursively).

The :class:`dircmp` offers a number of interesting attributes that may be used
to get various bits of information about the directory trees being compared.

Note that via :meth:`__getattr__` hooks, all attributes are computed lazily, so
there is no speed penalty if only those attributes which are lightweight to
compute are used.


.. attribute:: dircmp.left_list

   Files and subdirectories in *a*, filtered by *hide* and *ignore*.


.. attribute:: dircmp.right_list

   Files and subdirectories in *b*, filtered by *hide* and *ignore*.


.. attribute:: dircmp.common

   Files and subdirectories in both *a* and *b*.


.. attribute:: dircmp.left_only

   Files and subdirectories only in *a*.


.. attribute:: dircmp.right_only

   Files and subdirectories only in *b*.


.. attribute:: dircmp.common_dirs

   Subdirectories in both *a* and *b*.


.. attribute:: dircmp.common_files

   Files in both *a* and *b*


.. attribute:: dircmp.common_funny

   Names in both *a* and *b*, such that the type differs between the directories,
   or names for which :func:`os.stat` reports an error.


.. attribute:: dircmp.same_files

   Files which are identical in both *a* and *b*.


.. attribute:: dircmp.diff_files

   Files which are in both *a* and *b*, whose contents differ.


.. attribute:: dircmp.funny_files

   Files which are in both *a* and *b*, but could not be compared.


.. attribute:: dircmp.subdirs

   A dictionary mapping names in :attr:`common_dirs` to :class:`dircmp` objects.

