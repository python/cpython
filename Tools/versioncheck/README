This is versioncheck 1.0, a first stab at automatic checking of versions of
Python extension packages installed on your system.

The basic idea is that each package contains a _checkversion.py
somewhere, probably at the root level of the package. In addition, each
package maintainer makes a file available on the net, through ftp or
http, which contains the version number of the most recent distribution
and some readable text explaining the differences with previous
versions, where to download the package, etc.

The checkversions.py script walks through the installed Python tree (or
through a tree of choice), and runs each _checkversion.py script. These
scripts retrieve the current-version file over the net, compares version
numbers and tells the user about new versions of packages available.

A boilerplate for the _checkversion.py file can be found here. Replace
package name, version and the URL of the version-check file and put it in
your distribution. In stead of a single URL you can also specify a list
of URLs. Each of these will be checked in order until one is available,
this is handy for distributions that live in multiple places. Put the
primary distribution site (the most up-to-date site) before others.
The script is read and executed with exec(), not imported, and the current
directory is the checkversion directory, so be careful with globals,
importing, etc.

The version-check file consists of an rfc822-style header followed by
plaintext. The only header field checked currently is
'Current-Version:', which should contain te current version and is
matched against the string contained in the _checkversion.py script.
The rest of the file is human-readable text and presented to the user if
there is a version mismatch. It should contain at the very least a URL
of either the current distribution or a webpage describing it.

Pycheckversion.py is the module that does the actual checking of versions.
It should be fine where it is, it is imported by checkversion before anything
else is done, but if imports fail you may want to move it to somewhere
along sys.path.

	Jack Jansen, CWI, 23-Dec-97.
	<jack@cwi.nl>
	
