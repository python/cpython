*************************
  Python Advocacy HOWTO
*************************

:Author: A.M. Kuchling
:Release: 0.03


.. topic:: Abstract

   It's usually difficult to get your management to accept open source software,
   and Python is no exception to this rule.  This document discusses reasons to use
   Python, strategies for winning acceptance, facts and arguments you can use, and
   cases where you *shouldn't* try to use Python.


Reasons to Use Python
=====================

There are several reasons to incorporate a scripting language into your
development process, and this section will discuss them, and why Python has some
properties that make it a particularly good choice.


Programmability
---------------

Programs are often organized in a modular fashion.  Lower-level operations are
grouped together, and called by higher-level functions, which may in turn be
used as basic operations by still further upper levels.

For example, the lowest level might define a very low-level set of functions for
accessing a hash table.  The next level might use hash tables to store the
headers of a mail message, mapping a header name like ``Date`` to a value such
as ``Tue, 13 May 1997 20:00:54 -0400``.  A yet higher level may operate on
message objects, without knowing or caring that message headers are stored in a
hash table, and so forth.

Often, the lowest levels do very simple things; they implement a data structure
such as a binary tree or hash table, or they perform some simple computation,
such as converting a date string to a number.  The higher levels then contain
logic connecting these primitive operations.  Using the approach, the primitives
can be seen as basic building blocks which are then glued together to produce
the complete product.

Why is this design approach relevant to Python?  Because Python is well suited
to functioning as such a glue language.  A common approach is to write a Python
module that implements the lower level operations; for the sake of speed, the
implementation might be in C, Java, or even Fortran.  Once the primitives are
available to Python programs, the logic underlying higher level operations is
written in the form of Python code.  The high-level logic is then more
understandable, and easier to modify.

John Ousterhout wrote a paper that explains this idea at greater length,
entitled "Scripting: Higher Level Programming for the 21st Century".  I
recommend that you read this paper; see the references for the URL.  Ousterhout
is the inventor of the Tcl language, and therefore argues that Tcl should be
used for this purpose; he only briefly refers to other languages such as Python,
Perl, and Lisp/Scheme, but in reality, Ousterhout's argument applies to
scripting languages in general, since you could equally write extensions for any
of the languages mentioned above.


Prototyping
-----------

In *The Mythical Man-Month*, Fredrick Brooks suggests the following rule when
planning software projects: "Plan to throw one away; you will anyway."  Brooks
is saying that the first attempt at a software design often turns out to be
wrong; unless the problem is very simple or you're an extremely good designer,
you'll find that new requirements and features become apparent once development
has actually started.  If these new requirements can't be cleanly incorporated
into the program's structure, you're presented with two unpleasant choices:
hammer the new features into the program somehow, or scrap everything and write
a new version of the program, taking the new features into account from the
beginning.

Python provides you with a good environment for quickly developing an initial
prototype.  That lets you get the overall program structure and logic right, and
you can fine-tune small details in the fast development cycle that Python
provides.  Once you're satisfied with the GUI interface or program output, you
can translate the Python code into C++, Fortran, Java, or some other compiled
language.

Prototyping means you have to be careful not to use too many Python features
that are hard to implement in your other language.  Using ``eval()``, or regular
expressions, or the :mod:`pickle` module, means that you're going to need C or
Java libraries for formula evaluation, regular expressions, and serialization,
for example.  But it's not hard to avoid such tricky code, and in the end the
translation usually isn't very difficult.  The resulting code can be rapidly
debugged, because any serious logical errors will have been removed from the
prototype, leaving only more minor slip-ups in the translation to track down.

This strategy builds on the earlier discussion of programmability. Using Python
as glue to connect lower-level components has obvious relevance for constructing
prototype systems.  In this way Python can help you with development, even if
end users never come in contact with Python code at all.  If the performance of
the Python version is adequate and corporate politics allow it, you may not need
to do a translation into C or Java, but it can still be faster to develop a
prototype and then translate it, instead of attempting to produce the final
version immediately.

One example of this development strategy is Microsoft Merchant Server. Version
1.0 was written in pure Python, by a company that subsequently was purchased by
Microsoft.  Version 2.0 began to translate the code into C++, shipping with some
C++code and some Python code.  Version 3.0 didn't contain any Python at all; all
the code had been translated into C++.  Even though the product doesn't contain
a Python interpreter, the Python language has still served a useful purpose by
speeding up development.

This is a very common use for Python.  Past conference papers have also
described this approach for developing high-level numerical algorithms; see
David M. Beazley and Peter S. Lomdahl's paper "Feeding a Large-scale Physics
Application to Python" in the references for a good example.  If an algorithm's
basic operations are things like "Take the inverse of this 4000x4000 matrix",
and are implemented in some lower-level language, then Python has almost no
additional performance cost; the extra time required for Python to evaluate an
expression like ``m.invert()`` is dwarfed by the cost of the actual computation.
It's particularly good for applications where seemingly endless tweaking is
required to get things right. GUI interfaces and Web sites are prime examples.

The Python code is also shorter and faster to write (once you're familiar with
Python), so it's easier to throw it away if you decide your approach was wrong;
if you'd spent two weeks working on it instead of just two hours, you might
waste time trying to patch up what you've got out of a natural reluctance to
admit that those two weeks were wasted.  Truthfully, those two weeks haven't
been wasted, since you've learnt something about the problem and the technology
you're using to solve it, but it's human nature to view this as a failure of
some sort.


Simplicity and Ease of Understanding
------------------------------------

Python is definitely *not* a toy language that's only usable for small tasks.
The language features are general and powerful enough to enable it to be used
for many different purposes.  It's useful at the small end, for 10- or 20-line
scripts, but it also scales up to larger systems that contain thousands of lines
of code.

However, this expressiveness doesn't come at the cost of an obscure or tricky
syntax.  While Python has some dark corners that can lead to obscure code, there
are relatively few such corners, and proper design can isolate their use to only
a few classes or modules.  It's certainly possible to write confusing code by
using too many features with too little concern for clarity, but most Python
code can look a lot like a slightly-formalized version of human-understandable
pseudocode.

In *The New Hacker's Dictionary*, Eric S. Raymond gives the following definition
for "compact":

.. epigraph::

   Compact *adj.*  Of a design, describes the valuable property that it can all be
   apprehended at once in one's head. This generally means the thing created from
   the design can be used with greater facility and fewer errors than an equivalent
   tool that is not compact. Compactness does not imply triviality or lack of
   power; for example, C is compact and FORTRAN is not, but C is more powerful than
   FORTRAN. Designs become non-compact through accreting features and cruft that
   don't merge cleanly into the overall design scheme (thus, some fans of Classic C
   maintain that ANSI C is no longer compact).

   (From http://www.catb.org/~esr/jargon/html/C/compact.html)

In this sense of the word, Python is quite compact, because the language has
just a few ideas, which are used in lots of places.  Take namespaces, for
example.  Import a module with ``import math``, and you create a new namespace
called ``math``.  Classes are also namespaces that share many of the properties
of modules, and have a few of their own; for example, you can create instances
of a class. Instances?  They're yet another namespace.  Namespaces are currently
implemented as Python dictionaries, so they have the same methods as the
standard dictionary data type: .keys() returns all the keys, and so forth.

This simplicity arises from Python's development history.  The language syntax
derives from different sources; ABC, a relatively obscure teaching language, is
one primary influence, and Modula-3 is another.  (For more information about ABC
and Modula-3, consult their respective Web sites at http://www.cwi.nl/~steven/abc/
and http://www.m3.org.)  Other features have come from C, Icon,
Algol-68, and even Perl.  Python hasn't really innovated very much, but instead
has tried to keep the language small and easy to learn, building on ideas that
have been tried in other languages and found useful.

Simplicity is a virtue that should not be underestimated.  It lets you learn the
language more quickly, and then rapidly write code -- code that often works the
first time you run it.


Java Integration
----------------

If you're working with Java, Jython (http://www.jython.org/) is definitely worth
your attention.  Jython is a re-implementation of Python in Java that compiles
Python code into Java bytecodes.  The resulting environment has very tight,
almost seamless, integration with Java.  It's trivial to access Java classes
from Python, and you can write Python classes that subclass Java classes.
Jython can be used for prototyping Java applications in much the same way
CPython is used, and it can also be used for test suites for Java code, or
embedded in a Java application to add scripting capabilities.


Arguments and Rebuttals
=======================

Let's say that you've decided upon Python as the best choice for your
application.  How can you convince your management, or your fellow developers,
to use Python?  This section lists some common arguments against using Python,
and provides some possible rebuttals.

**Python is freely available software that doesn't cost anything. How good can
it be?**

Very good, indeed.  These days Linux and Apache, two other pieces of open source
software, are becoming more respected as alternatives to commercial software,
but Python hasn't had all the publicity.

Python has been around for several years, with many users and developers.
Accordingly, the interpreter has been used by many people, and has gotten most
of the bugs shaken out of it.  While bugs are still discovered at intervals,
they're usually either quite obscure (they'd have to be, for no one to have run
into them before) or they involve interfaces to external libraries.  The
internals of the language itself are quite stable.

Having the source code should be viewed as making the software available for
peer review; people can examine the code, suggest (and implement) improvements,
and track down bugs.  To find out more about the idea of open source code, along
with arguments and case studies supporting it, go to http://www.opensource.org.

**Who's going to support it?**

Python has a sizable community of developers, and the number is still growing.
The Internet community surrounding the language is an active one, and is worth
being considered another one of Python's advantages. Most questions posted to
the comp.lang.python newsgroup are quickly answered by someone.

Should you need to dig into the source code, you'll find it's clear and
well-organized, so it's not very difficult to write extensions and track down
bugs yourself.  If you'd prefer to pay for support, there are companies and
individuals who offer commercial support for Python.

**Who uses Python for serious work?**

Lots of people; one interesting thing about Python is the surprising diversity
of applications that it's been used for.  People are using Python to:

* Run Web sites

* Write GUI interfaces

* Control number-crunching code on supercomputers

* Make a commercial application scriptable by embedding the Python interpreter
  inside it

* Process large XML data sets

* Build test suites for C or Java code

Whatever your application domain is, there's probably someone who's used Python
for something similar.  Yet, despite being useable for such high-end
applications, Python's still simple enough to use for little jobs.

See http://wiki.python.org/moin/OrganizationsUsingPython for a list of some of
the  organizations that use Python.

**What are the restrictions on Python's use?**

They're practically nonexistent.  Consult the :file:`Misc/COPYRIGHT` file in the
source distribution, or the section :ref:`history-and-license` for the full
language, but it boils down to three conditions:

* You have to leave the copyright notice on the software; if you don't include
  the source code in a product, you have to put the copyright notice in the
  supporting documentation.

* Don't claim that the institutions that have developed Python endorse your
  product in any way.

* If something goes wrong, you can't sue for damages.  Practically all software
  licenses contain this condition.

Notice that you don't have to provide source code for anything that contains
Python or is built with it.  Also, the Python interpreter and accompanying
documentation can be modified and redistributed in any way you like, and you
don't have to pay anyone any licensing fees at all.

**Why should we use an obscure language like Python instead of well-known
language X?**

I hope this HOWTO, and the documents listed in the final section, will help
convince you that Python isn't obscure, and has a healthily growing user base.
One word of advice: always present Python's positive advantages, instead of
concentrating on language X's failings.  People want to know why a solution is
good, rather than why all the other solutions are bad.  So instead of attacking
a competing solution on various grounds, simply show how Python's virtues can
help.


Useful Resources
================

http://www.pythonology.com/success
   The Python Success Stories are a collection of stories from successful users of
   Python, with the emphasis on business and corporate users.

.. http://www.fsbassociates.com/books/pythonchpt1.htm
   The first chapter of \emph{Internet Programming with Python} also
   examines some of the reasons for using Python.  The book is well worth
   buying, but the publishers have made the first chapter available on
   the Web.

http://home.pacbell.net/ouster/scripting.html
   John Ousterhout's white paper on scripting is a good argument for the utility of
   scripting languages, though naturally enough, he emphasizes Tcl, the language he
   developed.  Most of the arguments would apply to any scripting language.

http://www.python.org/workshops/1997-10/proceedings/beazley.html
   The authors, David M. Beazley and Peter S. Lomdahl,  describe their use of
   Python at Los Alamos National Laboratory. It's another good example of how
   Python can help get real work done. This quotation from the paper has been
   echoed by many people:

   .. epigraph::

      Originally developed as a large monolithic application for massively parallel
      processing systems, we have used Python to transform our application into a
      flexible, highly modular, and extremely powerful system for performing
      simulation, data analysis, and visualization. In addition, we describe how
      Python has solved a number of important problems related to the development,
      debugging, deployment, and maintenance of scientific software.

http://pythonjournal.cognizor.com/pyj1/Everitt-Feit_interview98-V1.html
   This interview with Andy Feit, discussing Infoseek's use of Python, can be used
   to show that choosing Python didn't introduce any difficulties into a company's
   development process, and provided some substantial benefits.

.. http://www.python.org/psa/Commercial.html
   Robin Friedrich wrote this document on how to support Python's use in
   commercial projects.

http://www.python.org/workshops/1997-10/proceedings/stein.ps
   For the 6th Python conference, Greg Stein presented a paper that traced Python's
   adoption and usage at a startup called eShop, and later at Microsoft.

http://www.opensource.org
   Management may be doubtful of the reliability and usefulness of software that
   wasn't written commercially.  This site presents arguments that show how open
   source software can have considerable advantages over closed-source software.

http://www.faqs.org/docs/Linux-mini/Advocacy.html
   The Linux Advocacy mini-HOWTO was the inspiration for this document, and is also
   well worth reading for general suggestions on winning acceptance for a new
   technology, such as Linux or Python.  In general, you won't make much progress
   by simply attacking existing systems and complaining about their inadequacies;
   this often ends up looking like unfocused whining.  It's much better to point
   out some of the many areas where Python is an improvement over other systems.

