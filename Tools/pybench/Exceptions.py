from pybench import Test

class TryRaiseExcept(Test):

    version = 2.0
    operations = 2 + 3 + 3
    rounds = 80000

    def test(self):

        error = ValueError

        for i in xrange(self.rounds):
            try:
                raise error
            except:
                pass
            try:
                raise error
            except:
                pass
            try:
                raise error,"something"
            except:
                pass
            try:
                raise error,"something"
            except:
                pass
            try:
                raise error,"something"
            except:
                pass
            try:
                raise error("something")
            except:
                pass
            try:
                raise error("something")
            except:
                pass
            try:
                raise error("something")
            except:
                pass

    def calibrate(self):

        error = ValueError

        for i in xrange(self.rounds):
            pass


class TryExcept(Test):

    version = 2.0
    operations = 15 * 10
    rounds = 150000

    def test(self):

        for i in xrange(self.rounds):
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass

            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass

            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass

            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass

            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass

            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass


            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass
            try:
                pass
            except:
                pass

    def calibrate(self):

        for i in xrange(self.rounds):
            pass

### Test to make Fredrik happy...

if __name__ == '__main__':
    import timeit
    timeit.TestClass = TryRaiseExcept
    timeit.main(['-s', 'test = TestClass(); test.rounds = 1000',
                 'test.test()'])
