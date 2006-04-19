from pybench import Test

# First imports:
import os
import package.submodule

class SecondImport(Test):

    version = 0.1
    operations = 5 * 5
    rounds = 20000

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

    version = 0.1
    operations = 5 * 5
    rounds = 20000

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

    version = 0.1
    operations = 5 * 5
    rounds = 20000

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
            
