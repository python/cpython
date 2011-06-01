:mod:`packaging.metadata` --- Metadata handling
===============================================

.. module:: packaging.metadata
   :synopsis: Class holding the metadata of a release.


.. TODO use sphinx-autogen to generate basic doc from the docstrings

.. class:: Metadata

   This class can read and write metadata files complying with any of the
   defined versions: 1.0 (:PEP:`241`), 1.1 (:PEP:`314`) and 1.2 (:PEP:`345`).  It
   implements methods to parse Metadata files and write them, and a mapping
   interface to its contents.

   The :PEP:`345` implementation supports the micro-language for the environment
   markers, and displays warnings when versions that are supposed to be
   :PEP:`386`-compliant are violating the specification.


Reading metadata
----------------

The :class:`Metadata` class can be instantiated
with the path of the metadata file, and provides a dict-like interface to the
values::

   >>> from packaging.metadata import Metadata
   >>> metadata = Metadata('PKG-INFO')
   >>> metadata.keys()[:5]
   ('Metadata-Version', 'Name', 'Version', 'Platform', 'Supported-Platform')
   >>> metadata['Name']
   'CLVault'
   >>> metadata['Version']
   '0.5'
   >>> metadata['Requires-Dist']
   ["pywin32; sys.platform == 'win32'", "Sphinx"]


The fields that support environment markers can be automatically ignored if
the object is instantiated using the ``platform_dependent`` option.
:class:`Metadata` will interpret in this case
the markers and will automatically remove the fields that are not compliant
with the running environment. Here's an example under Mac OS X. The win32
dependency we saw earlier is ignored::

   >>> from packaging.metadata import Metadata
   >>> metadata = Metadata('PKG-INFO', platform_dependent=True)
   >>> metadata['Requires-Dist']
   ['Sphinx']


If you want to provide your own execution context, let's say to test the
metadata under a particular environment that is not the current environment,
you can provide your own values in the ``execution_context`` option, which
is the dict that may contain one or more keys of the context the micro-language
expects.

Here's an example, simulating a win32 environment::

   >>> from packaging.metadata import Metadata
   >>> context = {'sys.platform': 'win32'}
   >>> metadata = Metadata('PKG-INFO', platform_dependent=True,
   ...                     execution_context=context)
   ...
   >>> metadata['Requires-Dist'] = ["pywin32; sys.platform == 'win32'",
   ...                              "Sphinx"]
   ...
   >>> metadata['Requires-Dist']
   ['pywin32', 'Sphinx']


Writing metadata
----------------

Writing metadata can be done using the ``write`` method::

   >>> metadata.write('/to/my/PKG-INFO')

The class will pick the best version for the metadata, depending on the values
provided. If all the values provided exist in all versions, the class will
use :attr:`PKG_INFO_PREFERRED_VERSION`.  It is set by default to 1.0, the most
widespread version.


Conflict checking and best version
----------------------------------

Some fields in :PEP:`345` have to comply with the version number specification
defined in :PEP:`386`.  When they don't comply, a warning is emitted::

   >>> from packaging.metadata import Metadata
   >>> metadata = Metadata()
   >>> metadata['Requires-Dist'] = ['Funky (Groovie)']
   "Funky (Groovie)" is not a valid predicate
   >>> metadata['Requires-Dist'] = ['Funky (1.2)']

See also :mod:`packaging.version`.


.. TODO talk about check()


:mod:`packaging.markers` --- Environment markers
================================================

.. module:: packaging.markers
   :synopsis: Micro-language for environment markers


This is an implementation of environment markers `as defined in PEP 345
<http://www.python.org/dev/peps/pep-0345/#environment-markers>`_.  It is used
for some metadata fields.

.. function:: interpret(marker, execution_context=None)

   Interpret a marker and return a boolean result depending on the environment.
   Example:

      >>> interpret("python_version > '1.0'")
      True
