About Tix.py
-----------

Tix.py is based on an idea of Jean-Marc Lugrin (lugrin@ms.com) who wrote
pytix (another Python-Tix marriage). Tix widgets are an attractive and
useful extension to Tk. See http://tix.sourceforge.net
for more details about Tix and how to get it.

Features:
	1) It is almost complete.
	2) Tix widgets are represented by classes in Python. Sub-widgets
	   are members of the mega-widget class. For example, if a
	   particular TixWidget (e.g. ScrolledText) has an embedded widget
	   (Text in this case), it is possible to call the methods of the
	   child directly.
	3) The members of the class are created automatically. In the case
	   of widgets like ButtonBox, the members are added dynamically.


