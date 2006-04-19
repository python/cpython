from pybench import Test

class SpecialClassAttribute(Test):

    version = 0.3
    operations = 5*(12 + 12)
    rounds = 100000

    def test(self):

        class c:
            pass

        for i in xrange(self.rounds):

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            c.__a = 2
            c.__b = 3
            c.__c = 4

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

            x = c.__a
            x = c.__b
            x = c.__c

    def calibrate(self):

        class c:
            pass

        for i in xrange(self.rounds):
            pass

class NormalClassAttribute(Test):

    version = 0.3
    operations = 5*(12 + 12)
    rounds = 100000

    def test(self):

        class c:
            pass

        for i in xrange(self.rounds):

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4


            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4


            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4


            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4


            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4

            c.a = 2
            c.b = 3
            c.c = 4


            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

            x = c.a
            x = c.b
            x = c.c

    def calibrate(self):

        class c:
            pass

        for i in xrange(self.rounds):
            pass

class SpecialInstanceAttribute(Test):

    version = 0.3
    operations = 5*(12 + 12)
    rounds = 100000

    def test(self):

        class c:
            pass
        o = c()

        for i in xrange(self.rounds):

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4


            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4


            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4


            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4


            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4

            o.__a__ = 2
            o.__b__ = 3
            o.__c__ = 4


            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

            x = o.__a__
            x = o.__b__
            x = o.__c__

    def calibrate(self):

        class c:
            pass
        o = c()

        for i in xrange(self.rounds):
            pass

class NormalInstanceAttribute(Test):

    version = 0.3
    operations = 5*(12 + 12)
    rounds = 100000

    def test(self):

        class c:
            pass
        o = c()

        for i in xrange(self.rounds):

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4


            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4


            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4


            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4


            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4

            o.a = 2
            o.b = 3
            o.c = 4


            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

            x = o.a
            x = o.b
            x = o.c

    def calibrate(self):

        class c:
            pass
        o = c()

        for i in xrange(self.rounds):
            pass

class BuiltinMethodLookup(Test):

    version = 0.3
    operations = 5*(3*5 + 3*5)
    rounds = 70000

    def test(self):

        l = []
        d = {}

        for i in xrange(self.rounds):

            l.append
            l.append
            l.append
            l.append
            l.append

            l.insert
            l.insert
            l.insert
            l.insert
            l.insert

            l.sort
            l.sort
            l.sort
            l.sort
            l.sort

            d.has_key
            d.has_key
            d.has_key
            d.has_key
            d.has_key

            d.items
            d.items
            d.items
            d.items
            d.items

            d.get
            d.get
            d.get
            d.get
            d.get

            l.append
            l.append
            l.append
            l.append
            l.append

            l.insert
            l.insert
            l.insert
            l.insert
            l.insert

            l.sort
            l.sort
            l.sort
            l.sort
            l.sort

            d.has_key
            d.has_key
            d.has_key
            d.has_key
            d.has_key

            d.items
            d.items
            d.items
            d.items
            d.items

            d.get
            d.get
            d.get
            d.get
            d.get

            l.append
            l.append
            l.append
            l.append
            l.append

            l.insert
            l.insert
            l.insert
            l.insert
            l.insert

            l.sort
            l.sort
            l.sort
            l.sort
            l.sort

            d.has_key
            d.has_key
            d.has_key
            d.has_key
            d.has_key

            d.items
            d.items
            d.items
            d.items
            d.items

            d.get
            d.get
            d.get
            d.get
            d.get

            l.append
            l.append
            l.append
            l.append
            l.append

            l.insert
            l.insert
            l.insert
            l.insert
            l.insert

            l.sort
            l.sort
            l.sort
            l.sort
            l.sort

            d.has_key
            d.has_key
            d.has_key
            d.has_key
            d.has_key

            d.items
            d.items
            d.items
            d.items
            d.items

            d.get
            d.get
            d.get
            d.get
            d.get

            l.append
            l.append
            l.append
            l.append
            l.append

            l.insert
            l.insert
            l.insert
            l.insert
            l.insert

            l.sort
            l.sort
            l.sort
            l.sort
            l.sort

            d.has_key
            d.has_key
            d.has_key
            d.has_key
            d.has_key

            d.items
            d.items
            d.items
            d.items
            d.items

            d.get
            d.get
            d.get
            d.get
            d.get

    def calibrate(self):

        l = []
        d = {}

        for i in xrange(self.rounds):
            pass

