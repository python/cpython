"""Something just to look at via pydoc."""

import types

def global_func(x, y):
    """Module global function"""

def global_func2(x, y):
    """Module global function 2"""

class A:
    "A class."

    def A_method(self):
        "Method defined in A."
    def AB_method(self):
        "Method defined in A and B."
    def AC_method(self):
        "Method defined in A and C."
    def AD_method(self):
        "Method defined in A and D."
    def ABC_method(self):
        "Method defined in A, B and C."
    def ABD_method(self):
        "Method defined in A, B and D."
    def ACD_method(self):
        "Method defined in A, C and D."
    def ABCD_method(self):
        "Method defined in A, B, C and D."

    def A_classmethod(cls, x):
        "A class method defined in A."
    A_classmethod = classmethod(A_classmethod)

    def A_staticmethod(x, y):
        "A static method defined in A."
    A_staticmethod = staticmethod(A_staticmethod)

    def _getx(self):
        "A property getter function."
    def _setx(self, value):
        "A property setter function."
    def _delx(self):
        "A property deleter function."
    A_property = property(fdel=_delx, fget=_getx, fset=_setx,
                          doc="A sample property defined in A.")

    A_int_alias = int

class B(A):
    "A class, derived from A."

    def AB_method(self):
        "Method defined in A and B."
    def ABC_method(self):
        "Method defined in A, B and C."
    def ABD_method(self):
        "Method defined in A, B and D."
    def ABCD_method(self):
        "Method defined in A, B, C and D."
    def B_method(self):
        "Method defined in B."
    def BC_method(self):
        "Method defined in B and C."
    def BD_method(self):
        "Method defined in B and D."
    def BCD_method(self):
        "Method defined in B, C and D."

    @classmethod
    def B_classmethod(cls, x):
        "A class method defined in B."

    global_func = global_func  # same name
    global_func_alias = global_func
    global_func2_alias = global_func2
    B_classmethod_alias = B_classmethod
    A_classmethod_ref = A.A_classmethod
    A_staticmethod = A.A_staticmethod  # same name
    A_staticmethod_alias = A.A_staticmethod
    A_method_ref = A().A_method
    A_method_alias = A.A_method
    B_method_alias = B_method
    __repr__ = object.__repr__  # same name
    object_repr = object.__repr__
    get = {}.get  # same name
    dict_get = {}.get

B.B_classmethod_ref = B.B_classmethod


class C(A):
    "A class, derived from A."

    def AC_method(self):
        "Method defined in A and C."
    def ABC_method(self):
        "Method defined in A, B and C."
    def ACD_method(self):
        "Method defined in A, C and D."
    def ABCD_method(self):
        "Method defined in A, B, C and D."
    def BC_method(self):
        "Method defined in B and C."
    def BCD_method(self):
        "Method defined in B, C and D."
    def C_method(self):
        "Method defined in C."
    def CD_method(self):
        "Method defined in C and D."

class D(B, C):
    """A class, derived from B and C.
    """

    def AD_method(self):
        "Method defined in A and D."
    def ABD_method(self):
        "Method defined in A, B and D."
    def ACD_method(self):
        "Method defined in A, C and D."
    def ABCD_method(self):
        "Method defined in A, B, C and D."
    def BD_method(self):
        "Method defined in B and D."
    def BCD_method(self):
        "Method defined in B, C and D."
    def CD_method(self):
        "Method defined in C and D."
    def D_method(self):
        "Method defined in D."

class FunkyProperties(object):
    """From SF bug 472347, by Roeland Rengelink.

    Property getters etc may not be vanilla functions or methods,
    and this used to make GUI pydoc blow up.
    """

    def __init__(self):
        self.desc = {'x':0}

    class get_desc:
        def __init__(self, attr):
            self.attr = attr
        def __call__(self, inst):
            print('Get called', self, inst)
            return inst.desc[self.attr]
    class set_desc:
        def __init__(self, attr):
            self.attr = attr
        def __call__(self, inst, val):
            print('Set called', self, inst, val)
            inst.desc[self.attr] = val
    class del_desc:
        def __init__(self, attr):
            self.attr = attr
        def __call__(self, inst):
            print('Del called', self, inst)
            del inst.desc[self.attr]

    x = property(get_desc('x'), set_desc('x'), del_desc('x'), 'prop x')


submodule = types.ModuleType(__name__ + '.submodule',
    """A submodule, which should appear in its parent's summary""")

global_func_alias = global_func
A_classmethod = A.A_classmethod  # same name
A_classmethod2 = A.A_classmethod
A_classmethod3 = B.A_classmethod
A_staticmethod = A.A_staticmethod  # same name
A_staticmethod_alias = A.A_staticmethod
A_staticmethod_ref = A().A_staticmethod
A_staticmethod_ref2 = B().A_staticmethod
A_method = A().A_method  # same name
A_method2 = A().A_method
A_method3 = B().A_method
B_method = B.B_method  # same name
B_method2 = B.B_method
count = list.count  # same name
list_count = list.count
get = {}.get  # same name
dict_get = {}.get
