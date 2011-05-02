.. highlightlang:: rest

Additional Markup Constructs
============================

Sphinx adds a lot of new directives and interpreted text roles to standard reST
markup.  This section contains the reference material for these facilities.
Documentation for "standard" reST constructs is not included here, though
they are used in the Python documentation.

.. note::

   This is just an overview of Sphinx' extended markup capabilities; full
   coverage can be found in `its own documentation
   <http://sphinx.pocoo.org/contents.html>`_.


Meta-information markup
-----------------------

.. describe:: sectionauthor

   Identifies the author of the current section.  The argument should include
   the author's name such that it can be used for presentation (though it isn't)
   and email address.  The domain name portion of the address should be lower
   case.  Example::

      .. sectionauthor:: Guido van Rossum <guido@python.org>

   Currently, this markup isn't reflected in the output in any way, but it helps
   keep track of contributions.


Module-specific markup
----------------------

The markup described in this section is used to provide information about a
module being documented.  Each module should be documented in its own file.
Normally this markup appears after the title heading of that file; a typical
file might start like this::

   :mod:`parrot` -- Dead parrot access
   ===================================

   .. module:: parrot
      :platform: Unix, Windows
      :synopsis: Analyze and reanimate dead parrots.
   .. moduleauthor:: Eric Cleese <eric@python.invalid>
   .. moduleauthor:: John Idle <john@python.invalid>

As you can see, the module-specific markup consists of two directives, the
``module`` directive and the ``moduleauthor`` directive.

.. describe:: module

   This directive marks the beginning of the description of a module, package,
   or submodule. The name should be fully qualified (i.e. including the
   package name for submodules).

   The ``platform`` option, if present, is a comma-separated list of the
   platforms on which the module is available (if it is available on all
   platforms, the option should be omitted).  The keys are short identifiers;
   examples that are in use include "IRIX", "Mac", "Windows", and "Unix".  It is
   important to use a key which has already been used when applicable.

   The ``synopsis`` option should consist of one sentence describing the
   module's purpose -- it is currently only used in the Global Module Index.

   The ``deprecated`` option can be given (with no value) to mark a module as
   deprecated; it will be designated as such in various locations then.

.. describe:: moduleauthor

   The ``moduleauthor`` directive, which can appear multiple times, names the
   authors of the module code, just like ``sectionauthor`` names the author(s)
   of a piece of documentation.  It too does not result in any output currently.

.. note::

   It is important to make the section title of a module-describing file
   meaningful since that value will be inserted in the table-of-contents trees
   in overview files.


Information units
-----------------

There are a number of directives used to describe specific features provided by
modules.  Each directive requires one or more signatures to provide basic
information about what is being described, and the content should be the
description.  The basic version makes entries in the general index; if no index
entry is desired, you can give the directive option flag ``:noindex:``.  The
following example shows all of the features of this directive type::

    .. function:: spam(eggs)
                  ham(eggs)
       :noindex:

       Spam or ham the foo.

The signatures of object methods or data attributes should always include the
type name (``.. method:: FileInput.input(...)``), even if it is obvious from the
context which type they belong to; this is to enable consistent
cross-references.  If you describe methods belonging to an abstract protocol,
such as "context managers", include a (pseudo-)type name too to make the
index entries more informative.

The directives are:

.. describe:: c:function

   Describes a C function. The signature should be given as in C, e.g.::

      .. c:function:: PyObject* PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)

   This is also used to describe function-like preprocessor macros.  The names
   of the arguments should be given so they may be used in the description.

   Note that you don't have to backslash-escape asterisks in the signature,
   as it is not parsed by the reST inliner.

.. describe:: c:member

   Describes a C struct member. Example signature::

      .. c:member:: PyObject* PyTypeObject.tp_bases

   The text of the description should include the range of values allowed, how
   the value should be interpreted, and whether the value can be changed.
   References to structure members in text should use the ``member`` role.

.. describe:: c:macro

   Describes a "simple" C macro.  Simple macros are macros which are used
   for code expansion, but which do not take arguments so cannot be described as
   functions.  This is not to be used for simple constant definitions.  Examples
   of its use in the Python documentation include :c:macro:`PyObject_HEAD` and
   :c:macro:`Py_BEGIN_ALLOW_THREADS`.

.. describe:: c:type

   Describes a C type. The signature should just be the type name.

.. describe:: c:var

   Describes a global C variable.  The signature should include the type, such
   as::

      .. cvar:: PyObject* PyClass_Type

.. describe:: data

   Describes global data in a module, including both variables and values used
   as "defined constants."  Class and object attributes are not documented
   using this directive.

.. describe:: exception

   Describes an exception class.  The signature can, but need not include
   parentheses with constructor arguments.

.. describe:: function

   Describes a module-level function.  The signature should include the
   parameters, enclosing optional parameters in brackets.  Default values can be
   given if it enhances clarity.  For example::

      .. function:: repeat([repeat=3[, number=1000000]])

   Object methods are not documented using this directive. Bound object methods
   placed in the module namespace as part of the public interface of the module
   are documented using this, as they are equivalent to normal functions for
   most purposes.

   The description should include information about the parameters required and
   how they are used (especially whether mutable objects passed as parameters
   are modified), side effects, and possible exceptions.  A small example may be
   provided.

.. describe:: decorator

   Describes a decorator function.  The signature should *not* represent the
   signature of the actual function, but the usage as a decorator.  For example,
   given the functions

   .. code-block:: python

      def removename(func):
          func.__name__ = ''
          return func

      def setnewname(name):
          def decorator(func):
              func.__name__ = name
              return func
          return decorator

   the descriptions should look like this::

      .. decorator:: removename

         Remove name of the decorated function.

      .. decorator:: setnewname(name)

         Set name of the decorated function to *name*.

   There is no ``deco`` role to link to a decorator that is marked up with
   this directive; rather, use the ``:func:`` role.

.. describe:: class

   Describes a class.  The signature can include parentheses with parameters
   which will be shown as the constructor arguments.

.. describe:: attribute

   Describes an object data attribute.  The description should include
   information about the type of the data to be expected and whether it may be
   changed directly.  This directive should be nested in a class directive,
   like in this example::

      .. class:: Spam

            Description of the class.

            .. data:: ham

               Description of the attribute.

   If is also possible to document an attribute outside of a class directive,
   for example if the documentation for different attributes and methods is
   split in multiple sections.  The class name should then be included
   explicitly::

      .. data:: Spam.eggs

.. describe:: method

   Describes an object method.  The parameters should not include the ``self``
   parameter.  The description should include similar information to that
   described for ``function``.  This directive should be nested in a class
   directive, like in the example above.

.. describe:: decoratormethod

   Same as ``decorator``, but for decorators that are methods.

   Refer to a decorator method using the ``:meth:`` role.

.. describe:: opcode

   Describes a Python :term:`bytecode` instruction.

.. describe:: cmdoption

   Describes a Python command line option or switch.  Option argument names
   should be enclosed in angle brackets.  Example::

      .. cmdoption:: -m <module>

         Run a module as a script.

.. describe:: envvar

   Describes an environment variable that Python uses or defines.


There is also a generic version of these directives:

.. describe:: describe

   This directive produces the same formatting as the specific ones explained
   above but does not create index entries or cross-referencing targets.  It is
   used, for example, to describe the directives in this document. Example::

      .. describe:: opcode

         Describes a Python bytecode instruction.


Showing code examples
---------------------

Examples of Python source code or interactive sessions are represented using
standard reST literal blocks.  They are started by a ``::`` at the end of the
preceding paragraph and delimited by indentation.

Representing an interactive session requires including the prompts and output
along with the Python code.  No special markup is required for interactive
sessions.  After the last line of input or output presented, there should not be
an "unused" primary prompt; this is an example of what *not* to do::

   >>> 1 + 1
   2
   >>>

Syntax highlighting is handled in a smart way:

* There is a "highlighting language" for each source file.  Per default,
  this is ``'python'`` as the majority of files will have to highlight Python
  snippets.

* Within Python highlighting mode, interactive sessions are recognized
  automatically and highlighted appropriately.

* The highlighting language can be changed using the ``highlightlang``
  directive, used as follows::

     .. highlightlang:: c

  This language is used until the next ``highlightlang`` directive is
  encountered.

* The values normally used for the highlighting language are:

  * ``python`` (the default)
  * ``c``
  * ``rest``
  * ``none`` (no highlighting)

* If highlighting with the current language fails, the block is not highlighted
  in any way.

Longer displays of verbatim text may be included by storing the example text in
an external file containing only plain text.  The file may be included using the
``literalinclude`` directive. [1]_ For example, to include the Python source file
:file:`example.py`, use::

   .. literalinclude:: example.py

The file name is relative to the current file's path.  Documentation-specific
include files should be placed in the ``Doc/includes`` subdirectory.


Inline markup
-------------

As said before, Sphinx uses interpreted text roles to insert semantic markup in
documents.

Names of local variables, such as function/method arguments, are an exception,
they should be marked simply with ``*var*``.

For all other roles, you have to write ``:rolename:`content```.

There are some additional facilities that make cross-referencing roles more
versatile:

* You may supply an explicit title and reference target, like in reST direct
  hyperlinks: ``:role:`title <target>``` will refer to *target*, but the link
  text will be *title*.

* If you prefix the content with ``!``, no reference/hyperlink will be created.

* For the Python object roles, if you prefix the content with ``~``, the link
  text will only be the last component of the target.  For example,
  ``:meth:`~Queue.Queue.get``` will refer to ``Queue.Queue.get`` but only
  display ``get`` as the link text.

  In HTML output, the link's ``title`` attribute (that is e.g. shown as a
  tool-tip on mouse-hover) will always be the full target name.

The following roles refer to objects in modules and are possibly hyperlinked if
a matching identifier is found:

.. describe:: mod

   The name of a module; a dotted name may be used.  This should also be used for
   package names.

.. describe:: func

   The name of a Python function; dotted names may be used.  The role text
   should not include trailing parentheses to enhance readability.  The
   parentheses are stripped when searching for identifiers.

.. describe:: data

   The name of a module-level variable or constant.

.. describe:: const

   The name of a "defined" constant.  This may be a C-language ``#define``
   or a Python variable that is not intended to be changed.

.. describe:: class

   A class name; a dotted name may be used.

.. describe:: meth

   The name of a method of an object.  The role text should include the type
   name and the method name.  A dotted name may be used.

.. describe:: attr

   The name of a data attribute of an object.

.. describe:: exc

   The name of an exception. A dotted name may be used.

The name enclosed in this markup can include a module name and/or a class name.
For example, ``:func:`filter``` could refer to a function named ``filter`` in
the current module, or the built-in function of that name.  In contrast,
``:func:`foo.filter``` clearly refers to the ``filter`` function in the ``foo``
module.

Normally, names in these roles are searched first without any further
qualification, then with the current module name prepended, then with the
current module and class name (if any) prepended.  If you prefix the name with a
dot, this order is reversed.  For example, in the documentation of the
:mod:`codecs` module, ``:func:`open``` always refers to the built-in function,
while ``:func:`.open``` refers to :func:`codecs.open`.

A similar heuristic is used to determine whether the name is an attribute of
the currently documented class.

The following roles create cross-references to C-language constructs if they
are defined in the API documentation:

.. describe:: c:data

   The name of a C-language variable.

.. describe:: c:func

   The name of a C-language function. Should include trailing parentheses.

.. describe:: c:macro

   The name of a "simple" C macro, as defined above.

.. describe:: c:type

   The name of a C-language type.

.. describe:: c:member

   The name of a C type member, as defined above.


The following role does possibly create a cross-reference, but does not refer
to objects:

.. describe:: token

   The name of a grammar token (used in the reference manual to create links
   between production displays).


The following role creates a cross-reference to the term in the glossary:

.. describe:: term

   Reference to a term in the glossary.  The glossary is created using the
   ``glossary`` directive containing a definition list with terms and
   definitions.  It does not have to be in the same file as the ``term``
   markup, in fact, by default the Python docs have one global glossary
   in the ``glossary.rst`` file.

   If you use a term that's not explained in a glossary, you'll get a warning
   during build.

---------

The following roles don't do anything special except formatting the text
in a different style:

.. describe:: command

   The name of an OS-level command, such as ``rm``.

.. describe:: dfn

   Mark the defining instance of a term in the text.  (No index entries are
   generated.)

.. describe:: envvar

   An environment variable.  Index entries are generated.

.. describe:: file

   The name of a file or directory.  Within the contents, you can use curly
   braces to indicate a "variable" part, for example::

      ... is installed in :file:`/usr/lib/python2.{x}/site-packages` ...

   In the built documentation, the ``x`` will be displayed differently to
   indicate that it is to be replaced by the Python minor version.

.. describe:: guilabel

   Labels presented as part of an interactive user interface should be marked
   using ``guilabel``.  This includes labels from text-based interfaces such as
   those created using :mod:`curses` or other text-based libraries.  Any label
   used in the interface should be marked with this role, including button
   labels, window titles, field names, menu and menu selection names, and even
   values in selection lists.

.. describe:: kbd

   Mark a sequence of keystrokes.  What form the key sequence takes may depend
   on platform- or application-specific conventions.  When there are no relevant
   conventions, the names of modifier keys should be spelled out, to improve
   accessibility for new users and non-native speakers.  For example, an
   *xemacs* key sequence may be marked like ``:kbd:`C-x C-f```, but without
   reference to a specific application or platform, the same sequence should be
   marked as ``:kbd:`Control-x Control-f```.

.. describe:: keyword

   The name of a keyword in Python.

.. describe:: mailheader

   The name of an RFC 822-style mail header.  This markup does not imply that
   the header is being used in an email message, but can be used to refer to any
   header of the same "style."  This is also used for headers defined by the
   various MIME specifications.  The header name should be entered in the same
   way it would normally be found in practice, with the camel-casing conventions
   being preferred where there is more than one common usage. For example:
   ``:mailheader:`Content-Type```.

.. describe:: makevar

   The name of a :command:`make` variable.

.. describe:: manpage

   A reference to a Unix manual page including the section,
   e.g. ``:manpage:`ls(1)```.

.. describe:: menuselection

   Menu selections should be marked using the ``menuselection`` role.  This is
   used to mark a complete sequence of menu selections, including selecting
   submenus and choosing a specific operation, or any subsequence of such a
   sequence.  The names of individual selections should be separated by
   ``-->``.

   For example, to mark the selection "Start > Programs", use this markup::

      :menuselection:`Start --> Programs`

   When including a selection that includes some trailing indicator, such as the
   ellipsis some operating systems use to indicate that the command opens a
   dialog, the indicator should be omitted from the selection name.

.. describe:: mimetype

   The name of a MIME type, or a component of a MIME type (the major or minor
   portion, taken alone).

.. describe:: newsgroup

   The name of a Usenet newsgroup.

.. describe:: option

   A command-line option of Python.  The leading hyphen(s) must be included.
   If a matching ``cmdoption`` directive exists, it is linked to.  For options
   of other programs or scripts, use simple ````code```` markup.

.. describe:: program

   The name of an executable program.  This may differ from the file name for
   the executable for some platforms.  In particular, the ``.exe`` (or other)
   extension should be omitted for Windows programs.

.. describe:: regexp

   A regular expression. Quotes should not be included.

.. describe:: samp

   A piece of literal text, such as code.  Within the contents, you can use
   curly braces to indicate a "variable" part, as in ``:file:``.

   If you don't need the "variable part" indication, use the standard
   ````code```` instead.


The following roles generate external links:

.. describe:: pep

   A reference to a Python Enhancement Proposal.  This generates appropriate
   index entries. The text "PEP *number*\ " is generated; in the HTML output,
   this text is a hyperlink to an online copy of the specified PEP.

.. describe:: rfc

   A reference to an Internet Request for Comments.  This generates appropriate
   index entries. The text "RFC *number*\ " is generated; in the HTML output,
   this text is a hyperlink to an online copy of the specified RFC.


Note that there are no special roles for including hyperlinks as you can use
the standard reST markup for that purpose.


.. _doc-ref-role:

Cross-linking markup
--------------------

To support cross-referencing to arbitrary sections in the documentation, the
standard reST labels are "abused" a bit: Every label must precede a section
title; and every label name must be unique throughout the entire documentation
source.

You can then reference to these sections using the ``:ref:`label-name``` role.

Example::

   .. _my-reference-label:

   Section to cross-reference
   --------------------------

   This is the text of the section.

   It refers to the section itself, see :ref:`my-reference-label`.

The ``:ref:`` invocation is replaced with the section title.

Alternatively, you can reference any label (not just section titles)
if you provide the link text ``:ref:`link text <reference-label>```.

Paragraph-level markup
----------------------

These directives create short paragraphs and can be used inside information
units as well as normal text:

.. describe:: note

   An especially important bit of information about an API that a user should be
   aware of when using whatever bit of API the note pertains to.  The content of
   the directive should be written in complete sentences and include all
   appropriate punctuation.

   Example::

      .. note::

         This function is not suitable for sending spam e-mails.

.. describe:: warning

   An important bit of information about an API that a user should be aware of
   when using whatever bit of API the warning pertains to.  The content of the
   directive should be written in complete sentences and include all appropriate
   punctuation.  In the interest of not scaring users away from pages filled
   with warnings, this directive should only be chosen over ``note`` for
   information regarding the possibility of crashes, data loss, or security
   implications.

.. describe:: versionadded

   This directive documents the version of Python which added the described
   feature to the library or C API. When this applies to an entire module, it
   should be placed at the top of the module section before any prose.

   The first argument must be given and is the version in question; you can add
   a second argument consisting of a *brief* explanation of the change.

   Example::

      .. versionadded:: 3.1
         The *spam* parameter.

   Note that there must be no blank line between the directive head and the
   explanation; this is to make these blocks visually continuous in the markup.

.. describe:: versionchanged

   Similar to ``versionadded``, but describes when and what changed in the named
   feature in some way (new parameters, changed side effects, etc.).

--------------

.. describe:: impl-detail

   This directive is used to mark CPython-specific information.  Use either with
   a block content or a single sentence as an argument, i.e. either ::

      .. impl-detail::

         This describes some implementation detail.

         More explanation.

   or ::

      .. impl-detail:: This shortly mentions an implementation detail.

   "\ **CPython implementation detail:**\ " is automatically prepended to the
   content.

.. describe:: seealso

   Many sections include a list of references to module documentation or
   external documents.  These lists are created using the ``seealso`` directive.

   The ``seealso`` directive is typically placed in a section just before any
   sub-sections.  For the HTML output, it is shown boxed off from the main flow
   of the text.

   The content of the ``seealso`` directive should be a reST definition list.
   Example::

      .. seealso::

         Module :mod:`zipfile`
            Documentation of the :mod:`zipfile` standard module.

         `GNU tar manual, Basic Tar Format <http://link>`_
            Documentation for tar archive files, including GNU tar extensions.

.. describe:: rubric

   This directive creates a paragraph heading that is not used to create a
   table of contents node.  It is currently used for the "Footnotes" caption.

.. describe:: centered

   This directive creates a centered boldfaced paragraph.  Use it as follows::

      .. centered::

         Paragraph contents.


Table-of-contents markup
------------------------

Since reST does not have facilities to interconnect several documents, or split
documents into multiple output files, Sphinx uses a custom directive to add
relations between the single files the documentation is made of, as well as
tables of contents.  The ``toctree`` directive is the central element.

.. describe:: toctree

   This directive inserts a "TOC tree" at the current location, using the
   individual TOCs (including "sub-TOC trees") of the files given in the
   directive body.  A numeric ``maxdepth`` option may be given to indicate the
   depth of the tree; by default, all levels are included.

   Consider this example (taken from the library reference index)::

      .. toctree::
         :maxdepth: 2

         intro
         strings
         datatypes
         numeric
         (many more files listed here)

   This accomplishes two things:

   * Tables of contents from all those files are inserted, with a maximum depth
     of two, that means one nested heading.  ``toctree`` directives in those
     files are also taken into account.
   * Sphinx knows that the relative order of the files ``intro``,
     ``strings`` and so forth, and it knows that they are children of the
     shown file, the library index.  From this information it generates "next
     chapter", "previous chapter" and "parent chapter" links.

   In the end, all files included in the build process must occur in one
   ``toctree`` directive; Sphinx will emit a warning if it finds a file that is
   not included, because that means that this file will not be reachable through
   standard navigation.

   The special file ``contents.rst`` at the root of the source directory is the
   "root" of the TOC tree hierarchy; from it the "Contents" page is generated.


Index-generating markup
-----------------------

Sphinx automatically creates index entries from all information units (like
functions, classes or attributes) like discussed before.

However, there is also an explicit directive available, to make the index more
comprehensive and enable index entries in documents where information is not
mainly contained in information units, such as the language reference.

The directive is ``index`` and contains one or more index entries.  Each entry
consists of a type and a value, separated by a colon.

For example::

   .. index::
      single: execution; context
      module: __main__
      module: sys
      triple: module; search; path

This directive contains five entries, which will be converted to entries in the
generated index which link to the exact location of the index statement (or, in
case of offline media, the corresponding page number).

The possible entry types are:

single
   Creates a single index entry.  Can be made a subentry by separating the
   subentry text with a semicolon (this notation is also used below to describe
   what entries are created).
pair
   ``pair: loop; statement`` is a shortcut that creates two index entries,
   namely ``loop; statement`` and ``statement; loop``.
triple
   Likewise, ``triple: module; search; path`` is a shortcut that creates three
   index entries, which are ``module; search path``, ``search; path, module`` and
   ``path; module search``.
module, keyword, operator, object, exception, statement, builtin
   These all create two index entries.  For example, ``module: hashlib`` creates
   the entries ``module; hashlib`` and ``hashlib; module``.

For index directives containing only "single" entries, there is a shorthand
notation::

   .. index:: BNF, grammar, syntax, notation

This creates four index entries.


Grammar production displays
---------------------------

Special markup is available for displaying the productions of a formal grammar.
The markup is simple and does not attempt to model all aspects of BNF (or any
derived forms), but provides enough to allow context-free grammars to be
displayed in a way that causes uses of a symbol to be rendered as hyperlinks to
the definition of the symbol.  There is this directive:

.. describe:: productionlist

   This directive is used to enclose a group of productions.  Each production is
   given on a single line and consists of a name, separated by a colon from the
   following definition.  If the definition spans multiple lines, each
   continuation line must begin with a colon placed at the same column as in the
   first line.

   Blank lines are not allowed within ``productionlist`` directive arguments.

   The definition can contain token names which are marked as interpreted text
   (e.g. ``unaryneg ::= "-" `integer```) -- this generates cross-references
   to the productions of these tokens.

   Note that no further reST parsing is done in the production, so that you
   don't have to escape ``*`` or ``|`` characters.


.. XXX describe optional first parameter

The following is an example taken from the Python Reference Manual::

   .. productionlist::
      try_stmt: try1_stmt | try2_stmt
      try1_stmt: "try" ":" `suite`
               : ("except" [`expression` ["," `target`]] ":" `suite`)+
               : ["else" ":" `suite`]
               : ["finally" ":" `suite`]
      try2_stmt: "try" ":" `suite`
               : "finally" ":" `suite`


Substitutions
-------------

The documentation system provides three substitutions that are defined by default.
They are set in the build configuration file :file:`conf.py`.

.. describe:: |release|

   Replaced by the Python release the documentation refers to.  This is the full
   version string including alpha/beta/release candidate tags, e.g. ``2.5.2b3``.

.. describe:: |version|

   Replaced by the Python version the documentation refers to. This consists
   only of the major and minor version parts, e.g. ``2.5``, even for version
   2.5.1.

.. describe:: |today|

   Replaced by either today's date, or the date set in the build configuration
   file.  Normally has the format ``April 14, 2007``.


.. rubric:: Footnotes

.. [1] There is a standard ``.. include`` directive, but it raises errors if the
       file is not found.  This one only emits a warning.
