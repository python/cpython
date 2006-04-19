from pybench import Test

class SimpleListManipulation(Test):

    version = 0.3
    operations = 5* (6 + 6 + 6)
    rounds = 60000

    def test(self):

        l = []

        for i in xrange(self.rounds):

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            x = l[0]
            x = l[1]
            x = l[2]
            x = l[3]
            x = l[4]
            x = l[5]

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            x = l[0]
            x = l[1]
            x = l[2]
            x = l[3]
            x = l[4]
            x = l[5]

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            x = l[0]
            x = l[1]
            x = l[2]
            x = l[3]
            x = l[4]
            x = l[5]

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            x = l[0]
            x = l[1]
            x = l[2]
            x = l[3]
            x = l[4]
            x = l[5]

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            x = l[0]
            x = l[1]
            x = l[2]
            x = l[3]
            x = l[4]
            x = l[5]

            if len(l) > 10000:
                # cut down the size
                del l[:]

    def calibrate(self):

        l = []

        for i in xrange(self.rounds):
            pass

class ListSlicing(Test):

    version = 0.4
    operations = 25*(3+1+2+1)
    rounds = 400

    def test(self):

        n = range(100)
        r = range(25)

        for i in xrange(self.rounds):

            l = range(100)

            for j in r:

                m = l[50:]
                m = l[:25]
                m = l[50:55]
                l[:3] = n
                m = l[:-1]
                m = l[1:]
                l[-1:] = n

    def calibrate(self):

        n = range(100)
        r = range(25)

        for i in xrange(self.rounds):

            l = range(100)

            for j in r:
                pass

class SmallLists(Test):

    version = 0.3
    operations = 5*(1+ 6 + 6 + 3 + 1)
    rounds = 60000

    def test(self):

        for i in xrange(self.rounds):

            l = []

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            l[:3] = [1,2,3]
            m = l[:-1]
            m = l[1:]
            
            l[-1:] = [4,5,6]

            l = []

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            l[:3] = [1,2,3]
            m = l[:-1]
            m = l[1:]
            
            l[-1:] = [4,5,6]

            l = []

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            l[:3] = [1,2,3]
            m = l[:-1]
            m = l[1:]
            
            l[-1:] = [4,5,6]

            l = []

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            l[:3] = [1,2,3]
            m = l[:-1]
            m = l[1:]
            
            l[-1:] = [4,5,6]

            l = []

            l.append(2)
            l.append(3)
            l.append(4)
            l.append(2)
            l.append(3)
            l.append(4)

            l[0] = 3
            l[1] = 4
            l[2] = 5
            l[3] = 3
            l[4] = 4
            l[5] = 5
            
            l[:3] = [1,2,3]
            m = l[:-1]
            m = l[1:]
            
            l[-1:] = [4,5,6]

    def calibrate(self):

        for i in xrange(self.rounds):
            l = []

