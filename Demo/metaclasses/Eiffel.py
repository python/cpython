"""Support Eiffel-style preconditions and postconditions.

For example,

class C:
    def m1(self, arg):
        require arg > 0
        return whatever
        ensure Result > arg

can be written (clumsily, I agree) as:

class C(Eiffel):
    def m1(self, arg):
        return whatever
    def m1_pre(self, arg):
        assert arg > 0
    def m1_post(self, Result, arg):
        assert Result > arg

Pre- and post-conditions for a method, being implemented as methods
themselves, are inherited independently from the method.  This gives
much of the same effect of Eiffel, where pre- and post-conditions are
inherited when a method is overridden by a derived class.  However,
when a derived class in Python needs to extend a pre- or
post-condition, it must manually merge the base class' pre- or
post-condition with that defined in the derived class', for example:

class D(C):
    def m1(self, arg):
        return arg**2
    def m1_post(self, Result, arg):
        C.m1_post(self, Result, arg)
        assert Result < 100

This gives derived classes more freedom but also more responsibility
than in Eiffel, where the compiler automatically takes care of this.

In Eiffel, pre-conditions combine using contravariance, meaning a
derived class can only make a pre-condition weaker; in Python, this is
up to the derived class.  For example, a derived class that takes away
the requirement that arg > 0 could write:

    def m1_pre(self, arg):
        pass

but one could equally write a derived class that makes a stronger
requirement:

    def m1_pre(self, arg):
        require arg > 50

It would be easy to modify the classes shown here so that pre- and
post-conditions can be disabled (separately, on a per-class basis).

A different design would have the pre- or post-condition testing
functions return true for success and false for failure.  This would
make it possible to implement automatic combination of inherited
and new pre-/post-conditions.  All this is left as an exercise to the
reader.

"""

from Meta import MetaClass, MetaHelper, MetaMethodWrapper

class EiffelMethodWrapper(MetaMethodWrapper):

    def __init__(self, func, inst):
        MetaMethodWrapper.__init__(self, func, inst)
        # Note that the following causes recursive wrappers around
        # the pre-/post-condition testing methods.  These are harmless
        # but inefficient; to avoid them, the lookup must be done
        # using the class.
        try:
            self.pre = getattr(inst, self.__name__ + "_pre")
        except AttributeError:
            self.pre = None
        try:
            self.post = getattr(inst, self.__name__ + "_post")
        except AttributeError:
            self.post = None

    def __call__(self, *args, **kw):
        if self.pre:
            apply(self.pre, args, kw)
        Result = apply(self.func, (self.inst,) + args, kw)
        if self.post:
            apply(self.post, (Result,) + args, kw)
        return Result

class EiffelHelper(MetaHelper):
    __methodwrapper__ = EiffelMethodWrapper

class EiffelMetaClass(MetaClass):
    __helper__ = EiffelHelper

Eiffel = EiffelMetaClass('Eiffel', (), {})


def _test():
    class C(Eiffel):
        def m1(self, arg):
            return arg+1
        def m1_pre(self, arg):
            assert arg > 0, "precondition for m1 failed"
        def m1_post(self, Result, arg):
            assert Result > arg
    x = C()
    x.m1(12)
##    x.m1(-1)

if __name__ == '__main__':
    _test()
