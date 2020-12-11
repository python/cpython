# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id: README-2.1.txt,v 1.2 2001/12/09 05:01:28 idiscovery Exp $
#

About Tix.py
-----------

Tix.py is based on an idea of Jean-Marc Lugrin (lugrin@ms.com) who wrote
pytix (another Python-Tix marriage). Tix widgets are an attractive and
useful extension to Tk. See http://tix.sourceforge.net
for more details about Tix and how to get it.

As of version 2.1, Tix.py is a part of the standard Python library.
You should find Tix.py in the Lib/lib-tk directory, and demos in the Demo/tix
directory. The files here were current as of Python 2.2 and should work with
any version of Python, but may have updated in the standard Python distribution
(http://python.sourceforge.net and http://www.python.org).

Features:
	1) It is almost complete.
	2) Tix widgets are represented by classes in Python. Sub-widgets
	   are members of the mega-widget class. For example, if a
	   particular TixWidget (e.g. ScrolledText) has an embedded widget
	   (Text in this case), it is possible to call the methods of the
	   child directly.
	3) The members of the class are created automatically. In the case
	   of widgets like ButtonBox, the members are added dynamically.


