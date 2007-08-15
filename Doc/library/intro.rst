
.. _library-intro:

************
Introduction
************

The "Python library" contains several different kinds of components.

It contains data types that would normally be considered part of the "core" of a
language, such as numbers and lists.  For these types, the Python language core
defines the form of literals and places some constraints on their semantics, but
does not fully define the semantics.  (On the other hand, the language core does
define syntactic properties like the spelling and priorities of operators.)

The library also contains built-in functions and exceptions --- objects that can
be used by all Python code without the need of an :keyword:`import` statement.
Some of these are defined by the core language, but many are not essential for
the core semantics and are only described here.

The bulk of the library, however, consists of a collection of modules. There are
many ways to dissect this collection.  Some modules are written in C and built
in to the Python interpreter; others are written in Python and imported in
source form.  Some modules provide interfaces that are highly specific to
Python, like printing a stack trace; some provide interfaces that are specific
to particular operating systems, such as access to specific hardware; others
provide interfaces that are specific to a particular application domain, like
the World Wide Web. Some modules are available in all versions and ports of
Python; others are only available when the underlying system supports or
requires them; yet others are available only when a particular configuration
option was chosen at the time when Python was compiled and installed.

This manual is organized "from the inside out:" it first describes the built-in
data types, then the built-in functions and exceptions, and finally the modules,
grouped in chapters of related modules.  The ordering of the chapters as well as
the ordering of the modules within each chapter is roughly from most relevant to
least important.

This means that if you start reading this manual from the start, and skip to the
next chapter when you get bored, you will get a reasonable overview of the
available modules and application areas that are supported by the Python
library.  Of course, you don't *have* to read it like a novel --- you can also
browse the table of contents (in front of the manual), or look for a specific
function, module or term in the index (in the back).  And finally, if you enjoy
learning about random subjects, you choose a random page number (see module
:mod:`random`) and read a section or two.  Regardless of the order in which you
read the sections of this manual, it helps to start with chapter :ref:`builtin`,
as the remainder of the manual assumes familiarity with this material.

Let the show begin!

