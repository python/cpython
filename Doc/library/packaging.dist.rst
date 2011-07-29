:mod:`packaging.dist` --- The Distribution class
================================================

.. module:: packaging.dist
   :synopsis: Core Distribution class.


This module provides the :class:`Distribution` class, which represents the
module distribution being built/packaged/distributed/installed.

.. class:: Distribution(arguments)

   A :class:`Distribution` describes how to build, package, distribute and
   install a Python project.

   The arguments accepted by the constructor are laid out in the following
   table.  Some of them will end up in a metadata object, the rest will become
   data attributes of the :class:`Distribution` instance.

   .. TODO improve constructor to take a Metadata object + named params?
      (i.e. Distribution(metadata, cmdclass, py_modules, etc)
   .. TODO also remove obsolete(?) script_name, etc. parameters?  see what
      py2exe and other tools need

   +--------------------+--------------------------------+-------------------------------------------------------------+
   | argument name      | value                          | type                                                        |
   +====================+================================+=============================================================+
   | *name*             | The name of the project        | string                                                      |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *version*          | The version number of the      | See :mod:`packaging.version`                                |
   |                    | release                        |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *summary*          | A single line describing the   | a string                                                    |
   |                    | project                        |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *description*      | Longer description of the      | a string                                                    |
   |                    | project                        |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *author*           | The name of the project author | a string                                                    |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *author_email*     | The email address of the       | a string                                                    |
   |                    | project author                 |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *maintainer*       | The name of the current        | a string                                                    |
   |                    | maintainer, if different from  |                                                             |
   |                    | the author                     |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *maintainer_email* | The email address of the       |                                                             |
   |                    | current maintainer, if         |                                                             |
   |                    | different from the author      |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *home_page*        | A URL for the proejct          | a URL                                                       |
   |                    | (homepage)                     |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *download_url*     | A URL to download the project  | a URL                                                       |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *packages*         | A list of Python packages that | a list of strings                                           |
   |                    | packaging will manipulate      |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *py_modules*       | A list of Python modules that  | a list of strings                                           |
   |                    | packaging will manipulate      |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *scripts*          | A list of standalone scripts   | a list of strings                                           |
   |                    | to be built and installed      |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *ext_modules*      | A list of Python extensions to | A list of instances of                                      |
   |                    | be built                       | :class:`packaging.compiler.extension.Extension`             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *classifiers*      | A list of categories for the   | The list of available                                       |
   |                    | distribution                   | categorizations is available on `PyPI                       |
   |                    |                                | <http://pypi.python.org/pypi?:action=list_classifiers>`_.   |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *distclass*        | the :class:`Distribution`      | A subclass of                                               |
   |                    | class to use                   | :class:`packaging.dist.Distribution`                        |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *script_name*      | The name of the setup.py       | a string                                                    |
   |                    | script - defaults to           |                                                             |
   |                    | ``sys.argv[0]``                |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *script_args*      | Arguments to supply to the     | a list of strings                                           |
   |                    | setup script                   |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *options*          | default options for the setup  | a string                                                    |
   |                    | script                         |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *license*          | The license for the            | a string; should be used when there is no suitable License  |
   |                    | distribution                   | classifier, or to specify a classifier                      |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *keywords*         | Descriptive keywords           | a list of strings; used by catalogs                         |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *platforms*        | Platforms compatible with this | a list of strings; should be used when there is no          |
   |                    | distribution                   | suitable Platform classifier                                |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *cmdclass*         | A mapping of command names to  | a dictionary                                                |
   |                    | :class:`Command` subclasses    |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *data_files*       | A list of data files to        | a list                                                      |
   |                    | install                        |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
   | *package_dir*      | A mapping of Python packages   | a dictionary                                                |
   |                    | to directory names             |                                                             |
   +--------------------+--------------------------------+-------------------------------------------------------------+
