.. _superseded:

******************
Superseded modules
******************

The modules described in this chapter have been superseded by other modules
for most use cases, and are retained primarily to preserve backwards compatibility.

Modules may appear in this chapter because they only cover a limited subset of
a problem space, and a more generally applicable solution is available elsewhere
in the standard library (for example, :mod:`getopt` covers the very specific
task of "mimic the C :c:func:`!getopt` API in Python", rather than the broader
command line option parsing and argument parsing capabilities offered by
:mod:`optparse` and :mod:`argparse`).

Alternatively, modules may appear in this chapter because they are deprecated
outright, and awaiting removal in a future release, or they are
:term:`soft deprecated` and their use is actively discouraged in new projects.
With the removal of various obsolete modules through :pep:`594`, there are
currently no modules in this latter category.

.. toctree::
   :maxdepth: 1

   getopt.rst
   profile.rst
