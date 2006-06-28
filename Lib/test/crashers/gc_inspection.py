"""
gc.get_referrers() can be used to see objects before they are fully built.
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
