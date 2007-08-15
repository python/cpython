
:mod:`gensuitemodule` --- Generate OSA stub packages
====================================================

.. module:: gensuitemodule
   :platform: Mac
   :synopsis: Create a stub package from an OSA dictionary
.. sectionauthor:: Jack Jansen <Jack.Jansen@cwi.nl>


.. % \moduleauthor{Jack Jansen?}{email}

The :mod:`gensuitemodule` module creates a Python package implementing stub code
for the AppleScript suites that are implemented by a specific application,
according to its AppleScript dictionary.

It is usually invoked by the user through the :program:`PythonIDE`, but it can
also be run as a script from the command line (pass :option:`--help` for help on
the options) or imported from Python code. For an example of its use see
:file:`Mac/scripts/genallsuites.py` in a source distribution, which generates
the stub packages that are included in the standard library.

It defines the following public functions:


.. function:: is_scriptable(application)

   Returns true if ``application``, which should be passed as a pathname, appears
   to be scriptable. Take the return value with a grain of salt: :program:`Internet
   Explorer` appears not to be scriptable but definitely is.


.. function:: processfile(application[, output, basepkgname,  edit_modnames, creatorsignature, dump, verbose])

   Create a stub package for ``application``, which should be passed as a full
   pathname. For a :file:`.app` bundle this is the pathname to the bundle, not to
   the executable inside the bundle; for an unbundled CFM application you pass the
   filename of the application binary.

   This function asks the application for its OSA terminology resources, decodes
   these resources and uses the resultant data to create the Python code for the
   package implementing the client stubs.

   ``output`` is the pathname where the resulting package is stored, if not
   specified a standard "save file as" dialog is presented to the user.
   ``basepkgname`` is the base package on which this package will build, and
   defaults to :mod:`StdSuites`. Only when generating :mod:`StdSuites` itself do
   you need to specify this. ``edit_modnames`` is a dictionary that can be used to
   change modulenames that are too ugly after name mangling. ``creator_signature``
   can be used to override the 4-char creator code, which is normally obtained from
   the :file:`PkgInfo` file in the package or from the CFM file creator signature.
   When ``dump`` is given it should refer to a file object, and ``processfile``
   will stop after decoding the resources and dump the Python representation of the
   terminology resources to this file. ``verbose`` should also be a file object,
   and specifying it will cause ``processfile`` to tell you what it is doing.


.. function:: processfile_fromresource(application[, output,  basepkgname, edit_modnames, creatorsignature, dump, verbose])

   This function does the same as ``processfile``, except that it uses a different
   method to get the terminology resources. It opens ``application`` as a resource
   file and reads all ``"aete"`` and ``"aeut"`` resources from this file.

