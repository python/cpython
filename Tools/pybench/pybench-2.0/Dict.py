from pybench import Test

class DictCreation(Test):

    version = 2.0
    operations = 5*(5 + 5)
    rounds = 80000

    def test(self):

        for i in xrange(self.rounds):

            d1 = {}
            d2 = {}
            d3 = {}
            d4 = {}
            d5 = {}

            d1 = {1:2,3:4,5:6}
            d2 = {2:3,4:5,6:7}
            d3 = {3:4,5:6,7:8}
            d4 = {4:5,6:7,8:9}
            d5 = {6:7,8:9,10:11}

            d1 = {}
            d2 = {}
            d3 = {}
            d4 = {}
            d5 = {}

            d1 = {1:2,3:4,5:6}
            d2 = {2:3,4:5,6:7}
            d3 = {3:4,5:6,7:8}
            d4 = {4:5,6:7,8:9}
            d5 = {6:7,8:9,10:11}

            d1 = {}
            d2 = {}
            d3 = {}
            d4 = {}
            d5 = {}

            d1 = {1:2,3:4,5:6}
            d2 = {2:3,4:5,6:7}
            d3 = {3:4,5:6,7:8}
            d4 = {4:5,6:7,8:9}
            d5 = {6:7,8:9,10:11}

            d1 = {}
            d2 = {}
            d3 = {}
            d4 = {}
            d5 = {}

            d1 = {1:2,3:4,5:6}
            d2 = {2:3,4:5,6:7}
            d3 = {3:4,5:6,7:8}
            d4 = {4:5,6:7,8:9}
            d5 = {6:7,8:9,10:11}

            d1 = {}
            d2 = {}
            d3 = {}
            d4 = {}
            d5 = {}

            d1 = {1:2,3:4,5:6}
            d2 = {2:3,4:5,6:7}
            d3 = {3:4,5:6,7:8}
            d4 = {4:5,6:7,8:9}
            d5 = {6:7,8:9,10:11}

    def calibrate(self):

        for i in xrange(self.rounds):
            pass

class DictWithStringKeys(Test):

    version = 2.0
    operations = 5*(6 + 6)
    rounds = 200000

    def test(self):

        d = {}

        for i in xrange(self.rounds):

            d['abc'] = 1
            d['def'] = 2
            d['ghi'] = 3
            d['jkl'] = 4
            d['mno'] = 5
            d['pqr'] = 6

            d['abc']
            d['def']
            d['ghi']
            d['jkl']
            d['mno']
            d['pqr']

            d['abc'] = 1
            d['def'] = 2
            d['ghi'] = 3
            d['jkl'] = 4
            d['mno'] = 5
            d['pqr'] = 6

            d['abc']
            d['def']
            d['ghi']
            d['jkl']
            d['mno']
            d['pqr']

            d['abc'] = 1
            d['def'] = 2
            d['ghi'] = 3
            d['jkl'] = 4
            d['mno'] = 5
            d['pqr'] = 6

            d['abc']
            d['def']
            d['ghi']
            d['jkl']
            d['mno']
            d['pqr']

            d['abc'] = 1
            d['def'] = 2
            d['ghi'] = 3
            d['jkl'] = 4
            d['mno'] = 5
            d['pqr'] = 6

            d['abc']
            d['def']
            d['ghi']
            d['jkl']
            d['mno']
            d['pqr']

            d['abc'] = 1
            d['def'] = 2
            d['ghi'] = 3
            d['jkl'] = 4
            d['mno'] = 5
            d['pqr'] = 6

            d['abc']
            d['def']
            d['ghi']
            d['jkl']
            d['mno']
            d['pqr']

    def calibrate(self):

        d = {}

        for i in xrange(self.rounds):
            pass

class DictWithFloatKeys(Test):

    version = 2.0
    operations = 5*(6 + 6)
    rounds = 150000

    def test(self):

        d = {}

        for i in xrange(self.rounds):

            d[1.234] = 1
            d[2.345] = 2
            d[3.456] = 3
            d[4.567] = 4
            d[5.678] = 5
            d[6.789] = 6

            d[1.234]
            d[2.345]
            d[3.456]
            d[4.567]
            d[5.678]
            d[6.789]

            d[1.234] = 1
            d[2.345] = 2
            d[3.456] = 3
            d[4.567] = 4
            d[5.678] = 5
            d[6.789] = 6

            d[1.234]
            d[2.345]
            d[3.456]
            d[4.567]
            d[5.678]
            d[6.789]

            d[1.234] = 1
            d[2.345] = 2
            d[3.456] = 3
            d[4.567] = 4
            d[5.678] = 5
            d[6.789] = 6

            d[1.234]
            d[2.345]
            d[3.456]
            d[4.567]
            d[5.678]
            d[6.789]

            d[1.234] = 1
            d[2.345] = 2
            d[3.456] = 3
            d[4.567] = 4
            d[5.678] = 5
            d[6.789] = 6

            d[1.234]
            d[2.345]
            d[3.456]
            d[4.567]
            d[5.678]
            d[6.789]

            d[1.234] = 1
            d[2.345] = 2
            d[3.456] = 3
            d[4.567] = 4
            d[5.678] = 5
            d[6.789] = 6

            d[1.234]
            d[2.345]
            d[3.456]
            d[4.567]
            d[5.678]
            d[6.789]

    def calibrate(self):

        d = {}

        for i in xrange(self.rounds):
            pass

class DictWithIntegerKeys(Test):

    version = 2.0
    operations = 5*(6 + 6)
    rounds = 200000

    def test(self):

        d = {}

        for i in xrange(self.rounds):

            d[1] = 1
            d[2] = 2
            d[3] = 3
            d[4] = 4
            d[5] = 5
            d[6] = 6

            d[1]
            d[2]
            d[3]
            d[4]
            d[5]
            d[6]

            d[1] = 1
            d[2] = 2
            d[3] = 3
            d[4] = 4
            d[5] = 5
            d[6] = 6

            d[1]
            d[2]
            d[3]
            d[4]
            d[5]
            d[6]

            d[1] = 1
            d[2] = 2
            d[3] = 3
            d[4] = 4
            d[5] = 5
            d[6] = 6

            d[1]
            d[2]
            d[3]
            d[4]
            d[5]
            d[6]

            d[1] = 1
            d[2] = 2
            d[3] = 3
            d[4] = 4
            d[5] = 5
            d[6] = 6

            d[1]
            d[2]
            d[3]
            d[4]
            d[5]
            d[6]

            d[1] = 1
            d[2] = 2
            d[3] = 3
            d[4] = 4
            d[5] = 5
            d[6] = 6

            d[1]
            d[2]
            d[3]
            d[4]
            d[5]
            d[6]

    def calibrate(self):

        d = {}

        for i in xrange(self.rounds):
            pass

class SimpleDictManipulation(Test):

    version = 2.0
    operations = 5*(6 + 6 + 6 + 6)
    rounds = 100000

    def test(self):

        d = {}
        has_key = d.has_key

        for i in xrange(self.rounds):

            d[0] = 3
            d[1] = 4
            d[2] = 5
            d[3] = 3
            d[4] = 4
            d[5] = 5

            x = d[0]
            x = d[1]
            x = d[2]
            x = d[3]
            x = d[4]
            x = d[5]

            has_key(0)
            has_key(2)
            has_key(4)
            has_key(6)
            has_key(8)
            has_key(10)

            del d[0]
            del d[1]
            del d[2]
            del d[3]
            del d[4]
            del d[5]

            d[0] = 3
            d[1] = 4
            d[2] = 5
            d[3] = 3
            d[4] = 4
            d[5] = 5

            x = d[0]
            x = d[1]
            x = d[2]
            x = d[3]
            x = d[4]
            x = d[5]

            has_key(0)
            has_key(2)
            has_key(4)
            has_key(6)
            has_key(8)
            has_key(10)

            del d[0]
            del d[1]
            del d[2]
            del d[3]
            del d[4]
            del d[5]

            d[0] = 3
            d[1] = 4
            d[2] = 5
            d[3] = 3
            d[4] = 4
            d[5] = 5

            x = d[0]
            x = d[1]
            x = d[2]
            x = d[3]
            x = d[4]
            x = d[5]

            has_key(0)
            has_key(2)
            has_key(4)
            has_key(6)
            has_key(8)
            has_key(10)

            del d[0]
            del d[1]
            del d[2]
            del d[3]
            del d[4]
            del d[5]

            d[0] = 3
            d[1] = 4
            d[2] = 5
            d[3] = 3
            d[4] = 4
            d[5] = 5

            x = d[0]
            x = d[1]
            x = d[2]
            x = d[3]
            x = d[4]
            x = d[5]

            has_key(0)
            has_key(2)
            has_key(4)
            has_key(6)
            has_key(8)
            has_key(10)

            del d[0]
            del d[1]
            del d[2]
            del d[3]
            del d[4]
            del d[5]

            d[0] = 3
            d[1] = 4
            d[2] = 5
            d[3] = 3
            d[4] = 4
            d[5] = 5

            x = d[0]
            x = d[1]
            x = d[2]
            x = d[3]
            x = d[4]
            x = d[5]

            has_key(0)
            has_key(2)
            has_key(4)
            has_key(6)
            has_key(8)
            has_key(10)

            del d[0]
            del d[1]
            del d[2]
            del d[3]
            del d[4]
            del d[5]

    def calibrate(self):

        d = {}
        has_key = d.has_key

        for i in xrange(self.rounds):
            pass
