from pybench import Test

# Check for new-style class support:
try:
    class c(object):
        pass
except NameError:
    raise ImportError

###

class CreateNewInstances(Test):

    version = 2.0
    operations = 3 + 7 + 4
    rounds = 60000

    def test(self):

        class c(object):
            pass

        class d(object):
            def __init__(self,a,b,c):
                self.a = a
                self.b = b
                self.c = c

        class e(object):
            def __init__(self,a,b,c=4):
                self.a = a
                self.b = b
                self.c = c
                self.d = a
                self.e = b
                self.f = c

        for i in range(self.rounds):
            o = c()
            o1 = c()
            o2 = c()
            p = d(i,i,3)
            p1 = d(i,i,3)
            p2 = d(i,3,3)
            p3 = d(3,i,3)
            p4 = d(i,i,i)
            p5 = d(3,i,3)
            p6 = d(i,i,i)
            q = e(i,i,3)
            q1 = e(i,i,3)
            q2 = e(i,i,3)
            q3 = e(i,i)

    def calibrate(self):

        class c(object):
            pass

        class d(object):
            def __init__(self,a,b,c):
                self.a = a
                self.b = b
                self.c = c

        class e(object):
            def __init__(self,a,b,c=4):
                self.a = a
                self.b = b
                self.c = c
                self.d = a
                self.e = b
                self.f = c

        for i in range(self.rounds):
            pass
