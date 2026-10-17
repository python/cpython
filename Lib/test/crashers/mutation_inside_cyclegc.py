
# The cycle GC collector can be executed when any GC-tracked object is
# allocated, e.g. during a call to PyList_New(), PyDict_New(), ...
# Moreover, it can invoke arbitrary Python code via a weakref callback.
# This means that there are many places in the source where an arbitrary
# mutation could unexpectedly occur.

# The example below shows list_slice() not expecting the call to
# PyList_New to mutate the input list.  (Of course there are many
# more examples like this one.)


import weakref

class A(object):
    pass

def callback(x):
    del lst[:]


keepalive = []

for i in range(100):
    lst = [str(i)]
    a = A()
    a.cycle = a
    keepalive.append(weakref.ref(a, callback))
    del a
    while lst:
        keepalive.append(lst[:])
