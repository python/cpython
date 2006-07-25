"""
gc.get_referrers() can be used to see objects before they are fully built.

Note that this is only an example.  There are many ways to crash Python
by using gc.get_referrers(), as well as many extension modules (even
when they are using perfectly documented patterns to build objects).

Identifying and removing all places that expose to the GC a
partially-built object is a long-term project.  A patch was proposed on
SF specifically for this example but I consider fixing just this single
example a bit pointless (#1517042).

A fix would include a whole-scale code review, possibly with an API
change to decouple object creation and GC registration, and according
fixes to the documentation for extension module writers.  It's unlikely
to happen, though.  So this is currently classified as
"gc.get_referrers() is dangerous, use only for debugging".
"""

import gc


def g():
    marker = object()
    yield marker
    # now the marker is in the tuple being constructed
    [tup] = [x for x in gc.get_referrers(marker) if type(x) is tuple]
    print tup
    print tup[1]


tuple(g())
