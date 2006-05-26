from pybench import Test

class EmptyTest(Test):
    """This is just here as a potential measure of repeatability."""

    version = 0.3
    operations = 1
    rounds = 60000

    def test(self):

        l = []
        for i in xrange(self.rounds):
            pass


    def calibrate(self):

        l = []

        for i in xrange(self.rounds):
            pass
