"""Descriptions of all the slots in Python's type objects."""

class Slot(object):
    def __init__(self, name, cast=None, special=None, default="0"):
        self.name = name
        self.cast = cast
        self.special = special
        self.default = default

Slots = (Slot("ob_size"),
         Slot("tp_name"),
         Slot("tp_basicsize"),
         Slot("tp_itemsize"),
         Slot("tp_dealloc", "destructor"),
         Slot("tp_print", "printfunc"),
         Slot("tp_getattr", "getattrfunc"),
         Slot("tp_setattr", "setattrfunc"),
         Slot("tp_compare", "cmpfunc", "__cmp__"),
         Slot("tp_repr", "reprfunc", "__repr__"),
         Slot("tp_as_number"),
         Slot("tp_as_sequence"),
         Slot("tp_as_mapping"),
         Slot("tp_hash", "hashfunc", "__hash__"),
         Slot("tp_call", "ternaryfunc", "__call__"),
         Slot("tp_str", "reprfunc", "__str__"),
         Slot("tp_getattro", "getattrofunc", "__getattr__", # XXX
              "PyObject_GenericGetAttr"),
         Slot("tp_setattro", "setattrofunc", "__setattr__"),
         Slot("tp_as_buffer"),
         Slot("tp_flags", default="Py_TPFLAGS_DEFAULT"),
         Slot("tp_doc"),
         Slot("tp_traverse", "traverseprox"),
         Slot("tp_clear", "inquiry"),
         Slot("tp_richcompare", "richcmpfunc"),
         Slot("tp_weaklistoffset"),
         Slot("tp_iter", "getiterfunc", "__iter__"),
         Slot("tp_iternext", "iternextfunc", "__next__"), # XXX
         Slot("tp_methods"),
         Slot("tp_members"),
         Slot("tp_getset"),
         Slot("tp_base"),
         Slot("tp_dict"),
         Slot("tp_descr_get", "descrgetfunc"),
         Slot("tp_descr_set", "descrsetfunc"),
         Slot("tp_dictoffset"),
         Slot("tp_init", "initproc", "__init__"),
         Slot("tp_alloc", "allocfunc"),
         Slot("tp_new", "newfunc"),
         Slot("tp_free", "freefunc"),
         Slot("tp_is_gc", "inquiry"),
         Slot("tp_bases"),
         Slot("tp_mro"),
         Slot("tp_cache"),
         Slot("tp_subclasses"),
         Slot("tp_weaklist"),
         )

# give some slots symbolic names
TP_NAME = Slots[1]
TP_BASICSIZE = Slots[2]
TP_DEALLOC = Slots[4]
TP_DOC = Slots[20]
TP_METHODS = Slots[27]
TP_MEMBERS = Slots[28]
