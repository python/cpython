:mod:`!filecmp` --- File and Directory Comparisons
==================================================

.. module:: filecmp
   :synopsis: Compare files efficiently.

.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/filecmp.py`

--------------

The :mod:`!filecmp` module defines functions to compare files and directories,
with various optional time/correctness trade-offs. For comparing files,
see also the :mod:`difflib` module.

The :mod:`!filecmp` module defines the following functions:


.. function:: cmp(f1, f2, shallow=True)

   Compare the files named *f1* and *f2*, returning ``True`` if they seem equal,
   ``False`` otherwise.

   If *shallow* is true and the :func:`os.stat` signatures (file type, size, and
   modification time) of both files are identical, the files are taken to be
   equal.

   Otherwise, the files are treated as different if their sizes or contents differ.

   Note that no external programs are called from this function, giving it
   portability and efficiency.

   This function uses a cache for past comparisons and the results,
   with cache entries invalidated if the :func:`os.stat` information for the
   file changes.  The entire cache may be cleared using :func:`clear_cache`.


.. function:: cmpfiles(dir1, dir2, common, shallow=True)

   Compare the files in the two directories *dir1* and *dir2* whose names are
   given by *common*.

   Returns three lists of file names: *match*, *mismatch*,
   *errors*.  *match* contains the list of files that match, *mismatch* contains
   the names of those that don't, and *errors* lists the names of files which
   could not be compared.  Files are listed in *errors* if they don't exist in
   one of the directories, the user lacks permission to read them or if the
   comparison could not be done for some other reason.

   The *shallow* parameter has the same meaning and default value as for
   :func:`filecmp.cmp`.

   For example, ``cmpfiles('a', 'b', ['c', 'd/e'])`` will compare ``a/c`` with
   ``b/c`` and ``a/d/e`` with ``b/d/e``.  ``'c'`` and ``'d/e'`` will each be in
   one of the three returned lists.


.. function:: clear_cache()

   Clear the filecmp cache. This may be useful if a file is compared so quickly
   after it is modified that it is within the mtime resolution of
   the underlying filesystem.

   .. versionadded:: 3.4


.. _dircmp-objects:

The :class:`dircmp` class
-------------------------

.. class:: dircmp(a, b, ignore=None, hide=None, *, shallow=True)

   Construct a new directory comparison object, to compare the directories *a*
   and *b*.  *ignore* is a list of names to ignore, and defaults to
   :const:`filecmp.DEFAULT_IGNORES`.  *hide* is a list of names to hide, and
   defaults to ``[os.curdir, os.pardir]``.

   The :class:`dircmp` class compares files by doing *shallow* comparisons
   as described for :func:`filecmp.cmp` by default using the *shallow*
   parameter.

   .. versionchanged:: 3.13

      Added the *shallow* parameter.

   The :class:`dircmp` class provides the following methods:

   .. method:: report()

      Print (to :data:`sys.stdout`) a comparison between *a* and *b*.

   .. method:: report_partial_closure()

      Print a comparison between *a* and *b* and common immediate
      subdirectories.

   .. method:: report_full_closure()

      Print a comparison between *a* and *b* and common subdirectories
      (recursively).

   The :class:`dircmp` class offers a number of interesting attributes that may be
   used to get various bits of information about the directory trees being
   compared.

   Note that via :meth:`~object.__getattr__` hooks, all attributes are computed lazily,
   so there is no speed penalty if only those attributes which are lightweight
   to compute are used.


   .. attribute:: left

      The directory *a*.


   .. attribute:: right

      The directory *b*.


   .. attribute:: left_list

      Files and subdirectories in *a*, filtered by *hide* and *ignore*.


   .. attribute:: right_list

      Files and subdirectories in *b*, filtered by *hide* and *ignore*.


   .. attribute:: common

      Files and subdirectories in both *a* and *b*.


   .. attribute:: left_only

      Files and subdirectories only in *a*.


   .. attribute:: right_only

      Files and subdirectories only in *b*.


   .. attribute:: common_dirs

      Subdirectories in both *a* and *b*.


   .. attribute:: common_files

      Files in both *a* and *b*.


   .. attribute:: common_funny

      Names in both *a* and *b*, such that the type differs between the
      directories, or names for which :func:`os.stat` reports an error.


   .. attribute:: same_files

      Files which are identical in both *a* and *b*, using the class's
      file comparison operator.


   .. attribute:: diff_files

      Files which are in both *a* and *b*, whose contents differ according
      to the class's file comparison operator.


   .. attribute:: funny_files

      Files which are in both *a* and *b*, but could not be compared.


   .. attribute:: subdirs

      A dictionary mapping names in :attr:`common_dirs` to :class:`dircmp`
      instances (or MyDirCmp instances if this instance is of type MyDirCmp, a
      subclass of :class:`dircmp`).

      .. versionchanged:: 3.10
         Previously entries were always :class:`dircmp` instances. Now entries
         are the same type as *self*, if *self* is a subclass of
         :class:`dircmp`.

.. data:: DEFAULT_IGNORES

   .. versionadded:: 3.4

   List of directories ignored by :class:`dircmp` by default.


Here is a simplified example of using the ``subdirs`` attribute to search
recursively through two directories to show common different files::

    >>> from filecmp import dircmp
    >>> def print_diff_files(dcmp):
    ...     for name in dcmp.diff_files:
    ...         print("diff_file %s found in %s and %s" % (name, dcmp.left,
    ...               dcmp.right))
    ...     for sub_dcmp in dcmp.subdirs.values():
    ...         print_diff_files(sub_dcmp)
    ...
    >>> dcmp = dircmp('dir1', 'dir2') # doctest: +SKIP
    >>> print_diff_files(dcmp) # doctest: +SKIP

