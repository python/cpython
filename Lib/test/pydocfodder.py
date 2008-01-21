"""Something just to look at via pydoc."""

import types

class A_classic:
    "A classic class."
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


class B_classic(A_classic):
    "A classic class, derived from A_classic."
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

class C_classic(A_classic):
    "A classic class, derived from A_classic."
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

class D_classic(B_classic, C_classic):
    "A classic class, derived from B_classic and C_classic."
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


class A_new(object):
    "A new-style class."

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

    def A_staticmethod():
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

class B_new(A_new):
    "A new-style class, derived from A_new."

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

class C_new(A_new):
    "A new-style class, derived from A_new."

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

class D_new(B_new, C_new):
    """A new-style class, derived from B_new and C_new.
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
            print 'Get called', self, inst
            return inst.desc[self.attr]
    class set_desc:
        def __init__(self, attr):
            self.attr = attr
        def __call__(self, inst, val):
            print 'Set called', self, inst, val
            inst.desc[self.attr] = val
    class del_desc:
        def __init__(self, attr):
            self.attr = attr
        def __call__(self, inst):
            print 'Del called', self, inst
            del inst.desc[self.attr]

    x = property(get_desc('x'), set_desc('x'), del_desc('x'), 'prop x')


submodule = types.ModuleType(__name__ + '.submodule',
    """A submodule, which should appear in its parent's summary""")
