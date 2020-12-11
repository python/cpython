# README:  Tcl

This is the **Tcl 8.6.10** source distribution.

You can get any source release of Tcl from [our distribution
site](https://sourceforge.net/projects/tcl/files/Tcl/).

[![Build Status](https://travis-ci.org/tcltk/tcl.svg?branch=core-8-6-branch)](https://travis-ci.org/tcltk/tcl)

## Contents
 1. [Introduction](#intro)
 2. [Documentation](#doc)
 3. [Compiling and installing Tcl](#build)
 4. [Development tools](#devtools)
 5. [Tcl newsgroup](#complangtcl)
 6. [The Tcler's Wiki](#wiki)
 7. [Mailing lists](#email)
 8. [Support and Training](#support)
 9. [Tracking Development](#watch)
 10. [Thank You](#thanks)

## <a id="intro">1.</a> Introduction
Tcl provides a powerful platform for creating integration applications that
tie together diverse applications, protocols, devices, and frameworks.
When paired with the Tk toolkit, Tcl provides the fastest and most powerful
way to create GUI applications that run on PCs, Unix, and Mac OS X.
Tcl can also be used for a variety of web-related tasks and for creating
powerful command languages for applications.

Tcl is maintained, enhanced, and distributed freely by the Tcl community.
Source code development and tracking of bug reports and feature requests
takes place at [core.tcl-lang.org](https://core.tcl-lang.org/).
Tcl/Tk release and mailing list services are [hosted by
SourceForge](https://sourceforge.net/projects/tcl/)
with the Tcl Developer Xchange hosted at
[www.tcl-lang.org](https://www.tcl-lang.org).

Tcl is a freely available open source package.  You can do virtually
anything you like with it, such as modifying it, redistributing it,
and selling it either in whole or in part.  See the file
`license.terms` for complete information.

## <a id="doc">2.</a> Documentation
Extensive documentation is available at our website.
The home page for this release, including new features, is
[here](https://www.tcl.tk/software/tcltk/8.6.html).
Detailed release notes can be found at the
[file distributions page](https://sourceforge.net/projects/tcl/files/Tcl/)
by clicking on the relevant version.

Information about Tcl itself can be found at the [Developer
Xchange](https://www.tcl-lang.org/about/).
There have been many Tcl books on the market.  Many are mentioned in
[the Wiki](https://wiki.tcl-lang.org/_/ref?N=25206).

The complete set of reference manual entries for Tcl 8.6 is [online,
here](https://www.tcl-lang.org/man/tcl8.6/).

### <a id="doc.unix">2a.</a> Unix Documentation
The `doc` subdirectory in this release contains a complete set of
reference manual entries for Tcl.  Files with extension "`.1`" are for
programs (for example, `tclsh.1`); files with extension "`.3`" are for C
library procedures; and files with extension "`.n`" describe Tcl
commands.  The file "`doc/Tcl.n`" gives a quick summary of the Tcl
language syntax.  To print any of the man pages on Unix, cd to the
"doc" directory and invoke your favorite variant of troff using the
normal -man macros, for example

		groff -man -Tpdf Tcl.n >output.pdf

to print Tcl.n to PDF.  If Tcl has been installed correctly and your "man" program
supports it, you should be able to access the Tcl manual entries using the
normal "man" mechanisms, such as

		man Tcl

### <a id="doc.win">2b.</a> Windows Documentation
The "doc" subdirectory in this release contains a complete set of Windows
help files for Tcl.  Once you install this Tcl release, a shortcut to the
Windows help Tcl documentation will appear in the "Start" menu:

		Start | Programs | Tcl | Tcl Help

## <a id="build">3.</a> Compiling and installing Tcl
There are brief notes in the `unix/README`, `win/README`, and `macosx/README`
about compiling on these different platforms.  There is additional information
about building Tcl from sources
[online](https://www.tcl-lang.org/doc/howto/compile.html).

## <a id="devtools">4.</a> Development tools
ActiveState produces a high quality set of commercial quality development
tools that is available to accelerate your Tcl application development.
Tcl Dev Kit builds on the earlier TclPro toolset and provides a debugger,
static code checker, single-file wrapping utility, bytecode compiler and
more.  More information can be found at

	http://www.ActiveState.com/Tcl

## <a id="complangtcl">5.</a> Tcl newsgroup
There is a USENET news group, "`comp.lang.tcl`", intended for the exchange of
information about Tcl, Tk, and related applications.  The newsgroup is a
great place to ask general information questions.  For bug reports, please
see the "Support and bug fixes" section below.

## <a id="wiki">6.</a> Tcl'ers Wiki
There is a [wiki-based open community site](https://wiki.tcl-lang.org/)
covering all aspects of Tcl/Tk.

It is dedicated to the Tcl programming language and its extensions.  A
wealth of useful information can be found there.  It contains code
snippets, references to papers, books, and FAQs, as well as pointers to
development tools, extensions, and applications.  You can also recommend
additional URLs by editing the wiki yourself.

## <a id="email">7.</a> Mailing lists
Several mailing lists are hosted at SourceForge to discuss development or use
issues (like Macintosh and Windows topics).  For more information and to
subscribe, visit [here](https://sourceforge.net/projects/tcl/) and go to the
Mailing Lists page.

## <a id="support">8.</a> Support and Training
We are very interested in receiving bug reports, patches, and suggestions for
improvements.  We prefer that you send this information to us as tickets
entered into [our issue tracker](https://core.tcl-lang.org/tcl/reportlist).

We will log and follow-up on each bug, although we cannot promise a
specific turn-around time.  Enhancements may take longer and may not happen
at all unless there is widespread support for them (we're trying to
slow the rate at which Tcl/Tk turns into a kitchen sink).  It's very
difficult to make incompatible changes to Tcl/Tk at this point, due to
the size of the installed base.

The Tcl community is too large for us to provide much individual support for
users.  If you need help we suggest that you post questions to `comp.lang.tcl`
or ask a question on [Stack
Overflow](https://stackoverflow.com/questions/tagged/tcl).  We read the
newsgroup and will attempt to answer esoteric questions for which no one else
is likely to know the answer.  In addition, see the wiki for [links to other
organizations](https://wiki.tcl-lang.org/training) that offer Tcl/Tk training.

## <a id="watch">9.</a> Tracking Development
Tcl is developed in public.  You can keep an eye on how Tcl is changing at
[core.tcl-lang.org](https://core.tcl-lang.org/).

## <a id="thanks">10.</a> Thank You
We'd like to express our thanks to the Tcl community for all the
helpful suggestions, bug reports, and patches we have received.
Tcl/Tk has improved vastly and will continue to do so with your help.
