.. highlightlang:: rest

Style guide
===========

The Python documentation should follow the `Apple Publications Style Guide`_
wherever possible. This particular style guide was selected mostly because it
seems reasonable and is easy to get online.

Topics which are either not covered in Apple's style guide or treated
differently in Python documentation will be discussed in this
document.

Use of whitespace
-----------------

All reST files use an indentation of 3 spaces; no tabs are allowed.  The
maximum line length is 80 characters for normal text, but tables, deeply
indented code samples and long links may extend beyond that.  Code example
bodies should use normal Python 4-space indentation.

Make generous use of blank lines where applicable; they help grouping things
together.

A sentence-ending period may be followed by one or two spaces; while reST
ignores the second space, it is customarily put in by some users, for example
to aid Emacs' auto-fill mode.

Footnotes
---------

Footnotes are generally discouraged, though they may be used when they are the
best way to present specific information. When a footnote reference is added at
the end of the sentence, it should follow the sentence-ending punctuation. The
reST markup should appear something like this::

    This sentence has a footnote reference. [#]_ This is the next sentence.

Footnotes should be gathered at the end of a file, or if the file is very long,
at the end of a section. The docutils will automatically create backlinks to
the footnote reference.

Footnotes may appear in the middle of sentences where appropriate.

Capitalization
--------------

.. sidebar:: Sentence case

   Sentence case is a set of capitalization rules used in English
   sentences: the first word is always capitalized and other words are
   only capitalized if there is a specific rule requiring it.

Apple style guide recommends the use of title case in section titles.
However, rules for which words should be capitalized in title case
vary greaty between publications.

In Python documentation, use of sentence case in section titles is
preferable, but consistency within a unit is more important than
following this rule.  If you add a section to the chapter where most
sections are in title case you can either convert all titles to
sentence case or use the dominant style in the new section title.

Sentences that start with a word for which specific rules require
starting it with a lower case letter should be avoided in titles and
elsewhere.

.. note::

   Sections that describe a library module often have titles in the
   form of "modulename --- Short description of the module."  In this
   case, the description should be capitalized as a stand-alone
   sentence.

Many special names are used in the Python documentation, including the names of
operating systems, programming languages, standards bodies, and the like. Most
of these entities are not assigned any special markup, but the preferred
spellings are given here to aid authors in maintaining the consistency of
presentation in the Python documentation.

Other terms and words deserve special mention as well; these conventions should
be used to ensure consistency throughout the documentation:

CPU
   For "central processing unit." Many style guides say this should be
   spelled out on the first use (and if you must use it, do so!). For
   the Python documentation, this abbreviation should be avoided since
   there's no reasonable way to predict which occurrence will be the
   first seen by the reader. It is better to use the word "processor"
   instead.

POSIX
   The name assigned to a particular group of standards. This is always
   uppercase.

Python
   The name of our favorite programming language is always capitalized.

reST
   For "reStructuredText," an easy to read, plaintext markup syntax
   used to produce Python documentation.  When spelled out, it is
   always one word and both forms start with a lower case 'r'.

Unicode
   The name of a character coding system. This is always written
   capitalized.

Unix
   The name of the operating system developed at AT&T Bell Labs in the early
   1970s.


.. _Apple Publications Style Guide: http://developer.apple.com/mac/library/documentation/UserExperience/Conceptual/APStyleGuide/APSG_2009.pdf

