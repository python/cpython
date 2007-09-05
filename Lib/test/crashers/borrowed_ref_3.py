"""
PyDict_GetItem() returns a borrowed reference.
There are probably a number of places that are open to attacks
such as the following one, in bltinmodule.c:min_max().
"""

class KeyFunc(object):
    def __call__(self, n):
        del d['key']
        return 1


d = {'key': KeyFunc()}
min(range(10), **d)
