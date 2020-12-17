About PyTix
-----------

PyTix is based on an idea of Jean-Marc Lugrin (lugrin@ms.com) who wrote
pytix (another Python-Tix marriage). Tix widgets are an attractive and
useful extension to Tk. See http://www.cis.upenn.edu/~ioi/tix/tix.html
for more details about Tix and how to get it.

PyTix differs from Pytix in the following ways -
	1) It is complete at least as far as the Tix documentation
	   would allow !!
	2) Tix widgets are represented by classes in Python. Sub-widgets
	   are members of the mega-widget class. For example, if a
	   particular TixWidget (e.g. ScrolledText) has an embedded widget
	   (Text in this case), it is possible to call the methods of the
	   child directly.
	3) The members of the class are created automatically. In the case
	   of widgets like ButtonBox, the members are added dynamically.


Changes in Version 1.12 (see ChangeLog for details)
-----------------------

1) Minor bug fixes
2) Tested with Python 1.4, Tk 4.2, Tcl 7.6 and Tix 4.0.5
3) Compound Images now work !!

Modifications to Tkinter
------------------------

To support the full Tix functionality, the Tkinter module has been
modified. The modifications are quite minor (see Tkinter.diff for a diff
listing). Basically, in order to support the 'tixForm' geometry manager,
a new class 'Form' has been added. The class 'Widget' now also inherits
from this class.


Tix.py
------

PyTix has Tix.py as the wrapper for Tix widgets. It defines a couple of new
classes (TixWidget and TixSubWidget) which do the dirty work of initializing
a new widget instance.

The problem in Tix is that a single widget is actually a composite of many
smaller widgets. For example, a LabelEntry widget contains a Label and
and Entry. When we instantiate the LabelEntry widget, Python code is not
aware of the subwidgets. This is a problem because we cannot refer to
the widgets e.g. to change the color. We cannot instantiate a new widget
either because this would send a call to Tk to create a widget that is
already created. To separate the instantiation of the Python class and
the Tk widget, we need to have the new classes.

The subwidgets instances in Python is properly subclassed from the
appropriate base class. For example the 'entry' child of the LabelEntry
widget is subclassed from the Entry class. Thus all the methods of the
Entry class are known by the subwidget. This makes it possible to write
code like

	w = Tix.LabelEntry(master)
	w.text['bg'] = 'gray'
	w.text.insert(Tix.END, 'Hello, world')

The appropriate subwidgets are created automatically (they are kept in a
list in the parent). Member access is provided by writing the __getattr__
method.

There is a separate class for each Tix widget.
There are a few Tix convenience routines defined at the end as well.


How to install
--------------

See the file INSTALL for installation directions


How to use
----------

You must import the module Tix in your program. You no longer have to
import Tkinter since Tix.py does it for you.

See the sample programs in demos and demos/samples for an idea of how
to use Tix.

The basic documentation remains the Tix man pages. Note that currently
this documentation is not complete but it is expected to be so by Jan.
1996.

Credits
-------

The original pytix was written by Jean-Marc Lugrin (lugrin@ms.com).

PyTix (this package) is written by Sudhir Shenoy (sshenoy@gol.com)

Version 1.11 was only possible with help from Steffen Kremser(kremser@danet.de)
who helped a lot on cleaning up the HList widget and also translated the HList
demos from the Tix distribution
