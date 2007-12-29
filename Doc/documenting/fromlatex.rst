.. highlightlang:: rest

Differences to the LaTeX markup
===============================

Though the markup language is different, most of the concepts and markup types
of the old LaTeX docs have been kept -- environments as reST directives, inline
commands as reST roles and so forth.

However, there are some differences in the way these work, partly due to the
differences in the markup languages, partly due to improvements in Sphinx.  This
section lists these differences, in order to give those familiar with the old
format a quick overview of what they might run into.

Inline markup
-------------

These changes have been made to inline markup:

* **Cross-reference roles**

  Most of the following semantic roles existed previously as inline commands,
  but didn't do anything except formatting the content as code.  Now, they
  cross-reference to known targets (some names have also been shortened):

  | *mod* (previously *refmodule* or *module*)
  | *func* (previously *function*)
  | *data* (new)
  | *const*
  | *class*
  | *meth* (previously *method*)
  | *attr* (previously *member*)
  | *exc* (previously *exception*)
  | *cdata*
  | *cfunc* (previously *cfunction*)
  | *cmacro* (previously *csimplemacro*)
  | *ctype*

  Also different is the handling of *func* and *meth*: while previously
  parentheses were added to the callable name (like ``\func{str()}``), they are
  now appended by the build system -- appending them in the source will result
  in double parentheses.  This also means that ``:func:`str(object)``` will not
  work as expected -- use ````str(object)```` instead!

* **Inline commands implemented as directives**

  These were inline commands in LaTeX, but are now directives in reST:

  | *deprecated*
  | *versionadded*
  | *versionchanged*

  These are used like so::

     .. deprecated:: 2.5
        Reason of deprecation.

  Also, no period is appended to the text for *versionadded* and
  *versionchanged*.

  | *note*
  | *warning*

  These are used like so::

     .. note::

        Content of note.

* **Otherwise changed commands**

  The *samp* command previously formatted code and added quotation marks around
  it.  The *samp* role, however, features a new highlighting system just like
  *file* does:

     ``:samp:`open({filename}, {mode})``` results in :samp:`open({filename}, {mode})`

* **Dropped commands**

  These were commands in LaTeX, but are not available as roles:

  | *bfcode*
  | *character* (use :samp:`\`\`'c'\`\``)
  | *citetitle* (use ```Title <URL>`_``)
  | *code* (use ````code````)
  | *email* (just write the address in body text)
  | *filenq*
  | *filevar* (use the ``{...}`` highlighting feature of *file*)
  | *programopt*, *longprogramopt* (use *option*)
  | *ulink* (use ```Title <URL>`_``)
  | *url* (just write the URL in body text)
  | *var* (use ``*var*``)
  | *infinity*, *plusminus* (use the Unicode character)
  | *shortversion*, *version* (use the ``|version|`` and ``|release|`` substitutions)
  | *emph*, *strong* (use the reST markup)

* **Backslash escaping**

  In reST, a backslash must be escaped in normal text, and in the content of
  roles.  However, in code literals and literal blocks, it must not be escaped.
  Example: ``:file:`C:\\Temp\\my.tmp``` vs. ````open("C:\Temp\my.tmp")````.


Information units
-----------------

Information units (*...desc* environments) have been made reST directives.
These changes to information units should be noted:

* **New names**

  "desc" has been removed from every name.  Additionally, these directives have
  new names:

  | *cfunction* (previously *cfuncdesc*)
  | *cmacro* (previously *csimplemacrodesc*)
  | *exception* (previously *excdesc*)
  | *function* (previously *funcdesc*)
  | *attribute* (previously *memberdesc*)

  The *classdesc\** and *excclassdesc* environments have been dropped, the
  *class* and *exception* directives support classes documented with and without
  constructor arguments.

* **Multiple objects**

  The equivalent of the *...line* commands is::

     .. function:: do_foo(bar)
                   do_bar(baz)

        Description of the functions.

  IOW, just give one signatures per line, at the same indentation level.

* **Arguments**

  There is no *optional* command.  Just give function signatures like they
  should appear in the output::

     .. function:: open(filename[, mode[, buffering]])

        Description.

  Note: markup in the signature is not supported.

* **Indexing**

  The *...descni* environments have been dropped.  To mark an information unit
  as unsuitable for index entry generation, use the *noindex* option like so::

     .. function:: foo_*
        :noindex:

        Description.

* **New information units**

  There are new generic information units: One is called "describe" and can be
  used to document things that are not covered by the other units::

     .. describe:: a == b

        The equals operator.

  The others are::

     .. cmdoption:: -O

        Describes a command-line option.

     .. envvar:: PYTHONINSPECT

        Describes an environment variable.


Structure
---------

The LaTeX docs were split in several toplevel manuals.  Now, all files are part
of the same documentation tree, as indicated by the *toctree* directives in the
sources (though individual output formats may choose to split them up into parts
again).  Every *toctree* directive embeds other files as subdocuments of the
current file (this structure is not necessarily mirrored in the filesystem
layout).  The toplevel file is :file:`contents.rst`.

However, most of the old directory structure has been kept, with the
directories renamed as follows:

* :file:`api` -> :file:`c-api`
* :file:`dist` -> :file:`distutils`, with the single TeX file split up
* :file:`doc` -> :file:`documenting`
* :file:`ext` -> :file:`extending`
* :file:`inst` -> :file:`installing`
* :file:`lib` -> :file:`library`
* :file:`mac` -> merged into :file:`library`, with :file:`mac/using.tex`
  moved to :file:`using/mac.rst`
* :file:`ref` -> :file:`reference`
* :file:`tut` -> :file:`tutorial`, with the single TeX file split up


.. XXX more (index-generating, production lists, ...)
