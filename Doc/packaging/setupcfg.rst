.. highlightlang:: cfg

.. _setupcfg-spec:

*******************************************
Specification of the :file:`setup.cfg` file
*******************************************

:version: 0.9

This document describes the :file:`setup.cfg`, an ini-style configuration file
used by Packaging to replace the :file:`setup.py` file used by Distutils.
This specification is language-agnostic, and will therefore repeat some
information that's already documented for Python in the
:class:`configparser.RawConfigParser` documentation.

.. contents::
   :depth: 3
   :local:


Syntax
======

The ini-style format used in the configuration file is a simple collection of
sections that group sets of key-value fields separated by ``=`` or ``:`` and
optional whitespace.  Lines starting with ``#`` or ``;`` are comments and will
be ignored.  Empty lines are also ignored.  Example::

   [section1]
   # comment
   name = value
   name2 = "other value"

   [section2]
   foo = bar


Parsing values
---------------

Here are a set of rules to parse values:

- If a value is quoted with ``"`` chars, it's a string.  If a quote character is
  present in the quoted value, it can be escaped as ``\"`` or left as-is.

- If the value is ``true``, ``t``, ``yes``, ``y`` (case-insensitive) or ``1``,
  it's converted to the language equivalent of a ``True`` value; if it's
  ``false``, ``f``, ``no``, ``n`` (case-insensitive) or ``0``, it's converted to
  the equivalent of ``False``.

- A value can contain multiple lines.  When read, lines are converted into a
  sequence of values.  Each line after the first must start with a least one
  space or tab character; this leading indentation will be stripped.

- All other values are considered strings.

Examples::

   [section]
   foo = one
         two
         three

   bar = false
   baz = 1.3
   boo = "ok"
   beee = "wqdqw pojpj w\"ddq"


Extending files
---------------

A configuration file can be extended (i.e. included) by other files.  For this,
a ``DEFAULT`` section must contain an ``extends`` key which value points to one
or more files which will be merged into the current files by adding new sections
and fields.  If a file loaded by ``extends`` contains sections or keys that
already exist in the original file, they will not override the previous values.

Contents of :file:`one.cfg`::

    [section1]
    name = value

    [section2]
    foo = foo from one.cfg

Contents of :file:`two.cfg`::

    [DEFAULT]
    extends = one.cfg

    [section2]
    foo = foo from two.cfg
    baz = baz from two.cfg

The result of parsing :file:`two.cfg` is equivalent to this file::

    [section1]
    name = value

    [section2]
    foo = foo from one.cfg
    baz = baz from two.cfg

Example use of multi-line notation to include more than one file::

    [DEFAULT]
    extends = one.cfg
              two.cfg

When several files are provided, they are processed sequentially, following the
precedence rules explained above.  This means that the list of files should go
from most specialized to most common.

**Tools will need to provide a way to produce a merged version of the
file**.  This will be useful to let users publish a single file.


Description of sections and fields
==================================

Each section contains a description of its options.

- Options that are marked *multi* can have multiple values, one value per
  line.
- Options that are marked *optional* can be omitted.
- Options that are marked *environ* can use environment markers, as described
  in :PEP:`345`.


The sections are:

global
   Global options not related to one command.

metadata
   Name, version and other information defined by :PEP:`345`.

files
   Modules, scripts, data, documentation and other files to include in the
   distribution.

command sections
   Options given for specific commands, identical to those that can be given
   on the command line.


Global options
==============

Contains global options for Packaging. This section is shared with Distutils.


commands
   Defined Packaging command. A command is defined by its fully
   qualified name. *optional*, *multi*

   Examples::

      [global]
      commands =
          package.setup.CustomSdistCommand
          package.setup.BdistDeb

compilers
   Defined Packaging compiler. A compiler is defined by its fully
   qualified name. *optional*, *multi*

   Example::

      [global]
      compilers =
          hotcompiler.SmartCCompiler

setup_hook
   defines a callable that will be called right after the
   :file:`setup.cfg` file is read. The callable receives the configuration
   in form of a mapping and can make some changes to it. *optional*

   Example::

      [global]
      setup_hook = package.setup.customize_dist


Metadata
========

The metadata section contains the metadata for the project as described in
:PEP:`345`.  Field names are case-insensitive.

Fields:

name
   Name of the project.

version
   Version of the project. Must comply with :PEP:`386`.

platform
   Platform specification describing an operating system
   supported by the distribution which is not listed in the "Operating System"
   Trove classifiers (:PEP:`301`).  *optional*, *multi*

supported-platform
   Binary distributions containing a PKG-INFO file will
   use the Supported-Platform field in their metadata to specify the OS and
   CPU for which the binary distribution was compiled.  The semantics of
   the Supported-Platform field are free form. *optional*, *multi*

summary
   A one-line summary of what the distribution does.
   (Used to be called *description* in Distutils1.)

description
   A longer description. (Used to be called *long_description*
   in Distutils1.) A file can be provided in the *description-file* field.
   *optional*

keywords
   A list of additional keywords to be used to assist searching
   for the distribution in a larger catalog. Comma or space-separated.
   *optional*

home-page
   The URL for the distribution's home page.

download-url
   The URL from which this version of the distribution
   can be downloaded. *optional*

author
   Author's name. *optional*

author-email
   Author's e-mail. *optional*

maintainer
   Maintainer's name. *optional*

maintainer-email
   Maintainer's e-mail. *optional*

license
   A text indicating the term of uses, when a trove classifier does
   not match. *optional*.

classifiers
   Classification for the distribution, as described in PEP 301.
   *optional*, *multi*, *environ*

requires-dist
   name of another packaging project required as a dependency.
   The format is *name (version)* where version is an optional
   version declaration, as described in PEP 345. *optional*, *multi*, *environ*

provides-dist
   name of another packaging project contained within this
   distribution. Same format than *requires-dist*. *optional*, *multi*,
   *environ*

obsoletes-dist
   name of another packaging project this version obsoletes.
   Same format than *requires-dist*. *optional*, *multi*, *environ*

requires-python
   Specifies the Python version the distribution requires.
   The value is a version number, as described in PEP 345.
   *optional*, *multi*, *environ*

requires-externals
   a dependency in the system. This field is free-form,
   and just a hint for downstream maintainers. *optional*, *multi*,
   *environ*

project-url
   A label, followed by a browsable URL for the project.
   "label, url". The label is limited to 32 signs. *optional*, *multi*

One extra field not present in PEP 345 is supported:

description-file
   Path to a text file that will be used to fill the ``description`` field.
   ``description-file`` and ``description`` are mutually exclusive.  *optional*



Example::

   [metadata]
   name = pypi2rpm
   version = 0.1
   author = Tarek Ziadé
   author-email = tarek@ziade.org
   summary = Script that transforms an sdist archive into a RPM package
   description-file = README
   home-page = http://bitbucket.org/tarek/pypi2rpm/wiki/Home
   project-url:
       Repository, http://bitbucket.org/tarek/pypi2rpm/
       RSS feed, https://bitbucket.org/tarek/pypi2rpm/rss
   classifier =
       Development Status :: 3 - Alpha
       License :: OSI Approved :: Mozilla Public License 1.1 (MPL 1.1)

You should not give any explicit value for metadata-version: it will be guessed
from the fields present in the file.


Files
=====

This section describes the files included in the project.

packages_root
   the root directory containing all packages and modules
   (default: current directory).  *optional*

packages
   a list of packages the project includes *optional*, *multi*

modules
   a list of packages the project includes *optional*, *multi*

scripts
   a list of scripts the project includes *optional*, *multi*

extra_files
   a list of patterns to include extra files *optional*,
   *multi*

Example::

   [files]
   packages_root = src
   packages =
       pypi2rpm
       pypi2rpm.command

   scripts =
       pypi2rpm/pypi2rpm.py

   extra_files =
       setup.py
       README


.. Note::
   The :file:`setup.cfg` configuration file is included by default.  Contrary to
   Distutils, :file:`README` (or :file:`README.txt`) and :file:`setup.py` are
   not included by default.


Resources
---------

This section describes the files used by the project which must not be installed
in the same place that python modules or libraries, they are called
**resources**. They are for example documentation files, script files,
databases, etc...

For declaring resources, you must use this notation::

   source = destination

Data-files are declared in the **resources** field in the **file** section, for
example::

   [files]
   resources =
       source1 = destination1
       source2 = destination2

The **source** part of the declaration are relative paths of resources files
(using unix path separator **/**). For example, if you've this source tree::

   foo/
      doc/
         doc.man
      scripts/
         foo.sh

Your setup.cfg will look like::

   [files]
   resources =
       doc/doc.man = destination_doc
       scripts/foo.sh = destination_scripts

The final paths where files will be placed are composed by : **source** +
**destination**. In the previous example, **doc/doc.man** will be placed in
**destination_doc/doc/doc.man** and **scripts/foo.sh** will be placed in
**destination_scripts/scripts/foo.sh**. (If you want more control on the final
path, take a look at :ref:`setupcfg-resources-base-prefix`).

The **destination** part of resources declaration are paths with categories.
Indeed, it's generally a bad idea to give absolute path as it will be cross
incompatible. So, you must use resources categories in your **destination**
declaration. Categories will be replaced by their real path at the installation
time. Using categories is all benefit, your declaration will be simpler, cross
platform and it will allow packager to place resources files where they want
without breaking your code.

Categories can be specified by using this syntax::

   {category}

Default categories are:

* config
* appdata
* appdata.arch
* appdata.persistent
* appdata.disposable
* help
* icon
* scripts
* doc
* info
* man

A special category also exists **{distribution.name}** that will be replaced by
the name of the distribution, but as most of the defaults categories use them,
so it's not necessary to add **{distribution.name}** into your destination.

If you use categories in your declarations, and you are encouraged to do, final
path will be::

   source + destination_expanded

.. _example_final_path:

For example, if you have this setup.cfg::

   [metadata]
   name = foo

   [files]
   resources =
       doc/doc.man = {doc}

And if **{doc}** is replaced by **{datadir}/doc/{distribution.name}**, final
path will be::

   {datadir}/doc/foo/doc/doc.man

Where {datafir} category will be platform-dependent.


More control on source part
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Glob syntax
"""""""""""

When you declare source file, you can use a glob-like syntax to match multiples file, for example::

   scripts/* = {script}

Will match all the files in the scripts directory and placed them in the script category.

Glob tokens are:

 * ``*``: match all files.
 * ``?``: match any character.
 * ``**``: match any level of tree recursion (even 0).
 * ``{}``: will match any part separated by comma (example: ``{sh,bat}``).

.. TODO Add examples

Order of declaration
""""""""""""""""""""

The order of declaration is important if one file match multiple rules. The last
rules matched by file is used, this is useful if you have this source tree::

   foo/
      doc/
         index.rst
         setup.rst
         documentation.txt
         doc.tex
         README

And you want all the files in the doc directory to be placed in {doc} category,
but README must be placed in {help} category, instead of listing all the files
one by one, you can declare them in this way::

   [files]
   resources =
       doc/* = {doc}
       doc/README = {help}

Exclude
"""""""

You can exclude some files of resources declaration by giving no destination, it
can be useful if you have a non-resources file in the same directory of
resources files::

   foo/
      doc/
         RELEASES
         doc.tex
         documentation.txt
         docu.rst

Your **files** section will be::

   [files]
   resources =
       doc/* = {doc}
       doc/RELEASES =

More control on destination part
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _setupcfg-resources-base-prefix:

Defining a base prefix
""""""""""""""""""""""

When you define your resources, you can have more control of how the final path
is computed.

By default, the final path is::

   destination + source

This can generate long paths, for example (example_final_path_)::

   {datadir}/doc/foo/doc/doc.man

When you declare your source, you can use whitespace to split the source in
**prefix** **suffix**.  So, for example, if you have this source::

   docs/ doc.man

The **prefix** is "docs/" and the **suffix** is "doc.html".

.. note::

   Separator can be placed after a path separator or replace it. So these two
   sources are equivalent::

      docs/ doc.man
      docs doc.man

.. note::

   Glob syntax is working the same way with standard source and split source.
   So these rules::

      docs/*
      docs/ *
      docs *

   Will match all the files in the docs directory.

When you use split source, the final path is computed this way::

   destination + prefix

So for example, if you have this setup.cfg::

   [metadata]
   name = foo

   [files]
   resources =
       doc/ doc.man = {doc}

And if **{doc}** is replaced by **{datadir}/doc/{distribution.name}**, final
path will be::

   {datadir}/doc/foo/doc.man


Overwriting paths for categories
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This part is intended for system administrators or downstream OS packagers.

The real paths of categories are registered in the *sysconfig.cfg* file
installed in your python installation. This file uses an ini format too.
The content of the file is organized into several sections:

* globals: Standard categories's paths.
* posix_prefix: Standard paths for categories and installation paths for posix
  system.
* other ones XXX

Standard categories paths are platform independent, they generally refers to
other categories, which are platform dependent. :mod:`sysconfig` will choose
these category from sections matching os.name. For example::

   doc = {datadir}/doc/{distribution.name}

It refers to datadir category, which can be different between platforms. In
posix system, it may be::

   datadir = /usr/share

So the final path will be::

   doc = /usr/share/doc/{distribution.name}

The platform-dependent categories are:

* confdir
* datadir
* libdir
* base


Defining extra categories
^^^^^^^^^^^^^^^^^^^^^^^^^

.. TODO


Examples
^^^^^^^^

These examples are incremental but work unitarily.

Resources in root dir
"""""""""""""""""""""

Source tree::

   babar-1.0/
      README
      babar.sh
      launch.sh
      babar.py

:file:`setup.cfg`::

   [files]
   resources =
       README = {doc}
       *.sh = {scripts}

So babar.sh and launch.sh will be placed in {scripts} directory.

Now let's move all the scripts into a scripts directory.

Resources in sub-directory
""""""""""""""""""""""""""

Source tree::

   babar-1.1/
      README
      scripts/
         babar.sh
         launch.sh
         LAUNCH
      babar.py

:file:`setup.cfg`::

   [files]
   resources =
       README = {doc}
       scripts/ LAUNCH = {doc}
       scripts/ *.sh = {scripts}

It's important to use the separator after scripts/ to install all the shell
scripts into {scripts} instead of {scripts}/scripts.

Now let's add some docs.

Resources in multiple sub-directories
"""""""""""""""""""""""""""""""""""""

Source tree::

   babar-1.2/
      README
      scripts/
         babar.sh
         launch.sh
         LAUNCH
      docs/
         api
         man
      babar.py

:file:`setup.cfg`::

   [files]
   resources =
        README = {doc}
        scripts/ LAUNCH = {doc}
        scripts/ *.sh = {scripts}
        doc/ * = {doc}
        doc/ man = {man}

You want to place all the file in the docs script into {doc} category, instead
of man, which must be placed into {man} category, we will use the order of
declaration of globs to choose the destination, the last glob that match the
file is used.

Now let's add some scripts for windows users.

Complete example
""""""""""""""""

Source tree::

   babar-1.3/
      README
      doc/
         api
         man
      scripts/
         babar.sh
         launch.sh
         babar.bat
         launch.bat
         LAUNCH

:file:`setup.cfg`::

    [files]
    resources =
        README = {doc}
        scripts/ LAUNCH = {doc}
        scripts/ *.{sh,bat} = {scripts}
        doc/ * = {doc}
        doc/ man = {man}

We use brace expansion syntax to place all the shell and batch scripts into
{scripts} category.


Command sections
================

To pass options to commands without having to type them on the command line
for each invocation, you can write them in the :file:`setup.cfg` file, in a
section named after the command.  Example::

   [sdist]
   # special function to add custom files
   manifest-builders = package.setup.list_extra_files

   [build]
   use-2to3 = True

   [build_ext]
   inplace = on

   [check]
   strict = on
   all = on

Option values given in the configuration file can be overriden on the command
line.  See :ref:`packaging-setup-config` for more information.


Extensibility
=============

Every section can have fields that are not part of this specification.  They are
called **extensions**.

An extension field starts with ``X-``.  Example::

   [metadata]
   name = Distribute
   X-Debian-Name = python-distribute


Changes in the specification
============================

The versioning scheme for this specification is **MAJOR.MINOR**.  Changes in the
specification will cause the version number to be updated.

Changes to the minor number reflect backwards-compatible changes:

- New fields and sections (optional or mandatory) can be added.
- Optional fields can be removed.

The major number will be incremented for backwards-incompatible changes:

- Mandatory fields or sections are removed.
- Fields change their meaning.

As a consequence, a tool written to consume 1.5 has these properties:

- Can read 1.1, 1.2 and all versions < 1.5, since the tool knows what
  optional fields weren't there.

  .. XXX clarify

- Can also read 1.6 and other 1.x versions: The tool will just ignore fields it
  doesn't know about, even if they are mandatory in the new version.  If
  optional fields were removed, the tool will just consider them absent.

- Cannot read 2.x and should refuse to interpret such files.

A tool written to produce 1.x should have these properties:

- Writes all mandatory fields.
- May write optional fields.


Acknowledgments
===============

This specification includes work and feedback from these people:

- Tarek Ziadé
- Julien Jehannet
- Boris Feld
- Éric Araujo

(If your name is missing, please :ref:`let us know <reporting-bugs>`.)
