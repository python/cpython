import copy
import sys
import warnings

# Fake a number that implements numeric methods through __coerce__
class CoerceNumber:
    def __init__(self, arg):
        self.arg = arg

    def __repr__(self):
        return '<CoerceNumber %s>' % repr(self.arg)

    def __coerce__(self, other):
        if isinstance(other, CoerceNumber):
            return self.arg, other.arg
        else:
            return (self.arg, other)


# Fake a number that implements numeric ops through methods.
class MethodNumber:

    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<MethodNumber %s>' % repr(self.arg)

    def __add__(self,other):
        return self.arg + other

    def __radd__(self,other):
        return other + self.arg

    def __sub__(self,other):
        return self.arg - other

    def __rsub__(self,other):
        return other - self.arg

    def __mul__(self,other):
        return self.arg * other

    def __rmul__(self,other):
        return other * self.arg

    def __div__(self,other):
        return self.arg / other

    def __rdiv__(self,other):
        return other / self.arg

    def __pow__(self,other):
        return self.arg ** other

    def __rpow__(self,other):
        return other ** self.arg

    def __mod__(self,other):
        return self.arg % other

    def __rmod__(self,other):
        return other % self.arg

    def __cmp__(self, other):
        return cmp(self.arg, other)


candidates = [ 2, 4.0, 2L, 2+0j, [1], (2,), None,
               MethodNumber(1), CoerceNumber(2)]

infix_binops = [ '+', '-', '*', '/', '**', '%' ]
prefix_binops = [ 'divmod' ]

def do_infix_binops():
    for a in candidates:
        for b in candidates:
            for op in infix_binops:
                print '%s %s %s' % (a, op, b),
                try:
                    x = eval('a %s b' % op)
                except:
                    error = sys.exc_info()[:2]
                    print '... %s' % error[0]
                else:
                    print '=', x
                try:
                    z = copy.copy(a)
                except copy.Error:
                    z = a # assume it has no inplace ops
                print '%s %s= %s' % (a, op, b),
                try:
                    exec('z %s= b' % op)
                except:
                    error = sys.exc_info()[:2]
                    print '... %s' % error[0]
                else:
                    print '=>', z

def do_prefix_binops():
    for a in candidates:
        for b in candidates:
            for op in prefix_binops:
                print '%s(%s, %s)' % (op, a, b),
                try:
                    x = eval('%s(a, b)' % op)
                except:
                    error = sys.exc_info()[:2]
                    print '... %s' % error[0]
                else:
                    print '=', x

warnings.filterwarnings("ignore",
                        r'complex divmod\(\), // and % are deprecated',
                        DeprecationWarning,
                        r'test_coercion$')
do_infix_binops()
do_prefix_binops()
