.. highlightlang:: rest

Style Guide
===========

The Python documentation should follow the `Apple Publications Style Guide`_
wherever possible. This particular style guide was selected mostly because it
seems reasonable and is easy to get online.

Topics which are not covered in the Apple's style guide will be discussed in
this document.

All reST files use an indentation of 3 spaces.  The maximum line length is 80
characters for normal text, but tables, deeply indented code samples and long
links may extend beyond that.

Make generous use of blank lines where applicable; they help grouping things
together.

A sentence-ending period may be followed by one or two spaces; while reST
ignores the second space, it is customarily put in by some users, for example
to aid Emacs' auto-fill mode.

Footnotes are generally discouraged, though they may be used when they are the
best way to present specific information. When a footnote reference is added at
the end of the sentence, it should follow the sentence-ending punctuation. The
reST markup should appear something like this::

    This sentence has a footnote reference. [#]_ This is the next sentence.

Footnotes should be gathered at the end of a file, or if the file is very long,
at the end of a section. The docutils will automatically create backlinks to
the footnote reference.

Footnotes may appear in the middle of sentences where appropriate.

Many special names are used in the Python documentation, including the names of
operating systems, programming languages, standards bodies, and the like. Most
of these entities are not assigned any special markup, but the preferred
spellings are given here to aid authors in maintaining the consistency of
presentation in the Python documentation.

Other terms and words deserve special mention as well; these conventions should
be used to ensure consistency throughout the documentation:

CPU
    For "central processing unit." Many style guides say this should be spelled
    out on the first use (and if you must use it, do so!). For the Python
    documentation, this abbreviation should be avoided since there's no
    reasonable way to predict which occurrence will be the first seen by the
    reader. It is better to use the word "processor" instead.

POSIX
    The name assigned to a particular group of standards. This is always
    uppercase.

Python
    The name of our favorite programming language is always capitalized.

Unicode
    The name of a character set and matching encoding. This is always written
    capitalized.

Unix
    The name of the operating system developed at AT&T Bell Labs in the early
    1970s.


.. _Apple Publications Style Guide: http://developer.apple.com/documentation/UserExperience/Conceptual/APStyleGuide/AppleStyleGuide2003.pdf

