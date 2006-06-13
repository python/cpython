from pybench import Test

# First imports:
import os
import package.submodule

class SecondImport(Test):

    version = 2.0
    operations = 5 * 5
    rounds = 40000

    def test(self):

        for i in xrange(self.rounds):
            import os
            import os
            import os
            import os
            import os

            import os
            import os
            import os
            import os
            import os

            import os
            import os
            import os
            import os
            import os

            import os
            import os
            import os
            import os
            import os

            import os
            import os
            import os
            import os
            import os

    def calibrate(self):

        for i in xrange(self.rounds):
            pass


class SecondPackageImport(Test):

    version = 2.0
    operations = 5 * 5
    rounds = 40000

    def test(self):

        for i in xrange(self.rounds):
            import package
            import package
            import package
            import package
            import package

            import package
            import package
            import package
            import package
            import package

            import package
            import package
            import package
            import package
            import package

            import package
            import package
            import package
            import package
            import package

            import package
            import package
            import package
            import package
            import package

    def calibrate(self):

        for i in xrange(self.rounds):
            pass

class SecondSubmoduleImport(Test):

    version = 2.0
    operations = 5 * 5
    rounds = 40000

    def test(self):

        for i in xrange(self.rounds):
            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule

            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule

            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule

            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule

            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule
            import package.submodule

    def calibrate(self):

        for i in xrange(self.rounds):
            pass
