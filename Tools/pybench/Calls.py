from pybench import Test

class PythonFunctionCalls(Test):

    version = 0.3
    operations = 5*(1+4+4+2)
    rounds = 60000

    def test(self):

        global f,f1,g,h

        # define functions
        def f():
            pass

        def f1(x):
            pass

        def g(a,b,c):
            return a,b,c

        def h(a,b,c,d=1,e=2,f=3):
            return d,e,f

        # do calls
        for i in xrange(self.rounds):

            f()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            h(i,i,3,i,i)
            h(i,i,i,2,i,3)

            f()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            h(i,i,3,i,i)
            h(i,i,i,2,i,3)

            f()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            h(i,i,3,i,i)
            h(i,i,i,2,i,3)

            f()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            h(i,i,3,i,i)
            h(i,i,i,2,i,3)

            f()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            g(i,i,i)
            h(i,i,3,i,i)
            h(i,i,i,2,i,3)

    def calibrate(self):

        global f,f1,g,h

        # define functions
        def f():
            pass

        def f1(x):
            pass

        def g(a,b,c):
            return a,b,c

        def h(a,b,c,d=1,e=2,f=3):
            return d,e,f

        # do calls
        for i in xrange(self.rounds):
            pass

###

class BuiltinFunctionCalls(Test):

    version = 0.4
    operations = 5*(2+5+5+5)
    rounds = 30000

    def test(self):

        # localize functions
        f0 = globals
        f1 = hash
        f2 = cmp
        f3 = range

        # do calls
        for i in xrange(self.rounds):

            f0()
            f0()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)

            f0()
            f0()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)

            f0()
            f0()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)

            f0()
            f0()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)

            f0()
            f0()
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f1(i)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f2(1,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)
            f3(1,3,2)

    def calibrate(self):

        # localize functions
        f0 = dir
        f1 = hash
        f2 = range
        f3 = range

        # do calls
        for i in xrange(self.rounds):
            pass

###

class PythonMethodCalls(Test):

    version = 0.3
    operations = 5*(6 + 5 + 4)
    rounds = 20000

    def test(self):

        class c:

            x = 2
            s = 'string'

            def f(self):

                return self.x

            def j(self,a,b):

                self.y = a
                self.t = b
                return self.y

            def k(self,a,b,c=3):

                self.y = a
                self.s = b
                self.t = c

        o = c()

        for i in xrange(self.rounds):

            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.j(i,i)
            o.j(i,i)
            o.j(i,2)
            o.j(i,2)
            o.j(2,2)
            o.k(i,i)
            o.k(i,2)
            o.k(i,2,3)
            o.k(i,i,c=4)

            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.j(i,i)
            o.j(i,i)
            o.j(i,2)
            o.j(i,2)
            o.j(2,2)
            o.k(i,i)
            o.k(i,2)
            o.k(i,2,3)
            o.k(i,i,c=4)

            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.j(i,i)
            o.j(i,i)
            o.j(i,2)
            o.j(i,2)
            o.j(2,2)
            o.k(i,i)
            o.k(i,2)
            o.k(i,2,3)
            o.k(i,i,c=4)

            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.j(i,i)
            o.j(i,i)
            o.j(i,2)
            o.j(i,2)
            o.j(2,2)
            o.k(i,i)
            o.k(i,2)
            o.k(i,2,3)
            o.k(i,i,c=4)

            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.f()
            o.j(i,i)
            o.j(i,i)
            o.j(i,2)
            o.j(i,2)
            o.j(2,2)
            o.k(i,i)
            o.k(i,2)
            o.k(i,2,3)
            o.k(i,i,c=4)

    def calibrate(self):

        class c:

            x = 2
            s = 'string'

            def f(self):

                return self.x

            def j(self,a,b):

                self.y = a
                self.t = b

            def k(self,a,b,c=3):

                self.y = a
                self.s = b
                self.t = c

        o = c

        for i in xrange(self.rounds):
            pass

###

class Recursion(Test):

    version = 0.3
    operations = 5
    rounds = 50000

    def test(self):

        global f

        def f(x):

            if x > 1:
                return f(x-1)
            return 1

        for i in xrange(self.rounds):
            f(10)
            f(10)
            f(10)
            f(10)
            f(10)

    def calibrate(self):

        global f

        def f(x):

            if x > 0:
                return f(x-1)
            return 1

        for i in xrange(self.rounds):
            pass

