"""Something just to look at via pydoc."""

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


class A_new_dynamic(object):
    "A new-style dynamic class."

    __dynamic__ = 1

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

class B_new_dynamic(A_new_dynamic):
    "A new-style dynamic class, derived from A_new_dynamic."

    __dynamic__ = 1

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

class C_new_dynamic(A_new_dynamic):
    "A new-style dynamic class, derived from A_new_dynamic."

    __dynamic__ = 1

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

class D_new_dynamic(B_new_dynamic, C_new_dynamic):
    """A new-style dynamic class, derived from B_new_dynamic and
       C_new_dynamic.
    """

    __dynamic__ = 1

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


class A_new_static(object):
    "A new-style static class."

    __dynamic__ = 0

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


class B_new_static(A_new_static):
    "A new-style static class, derived from A_new_static."

    __dynamic__ = 0

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

class C_new_static(A_new_static):
    "A new-style static class, derived from A_new_static."

    __dynamic__ = 0

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

class D_new_static(B_new_static, C_new_static):
    "A new-style static class, derived from B_new_static and C_new_static."

    __dynamic__ = 0

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
