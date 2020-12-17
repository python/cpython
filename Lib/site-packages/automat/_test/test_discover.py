import operator
import os
import shutil
import sys
import textwrap
import tempfile
from unittest import skipIf, TestCase

import six


def isTwistedInstalled():
    try:
        __import__('twisted')
    except ImportError:
        return False
    else:
        return True


class _WritesPythonModules(TestCase):
    """
    A helper that enables generating Python module test fixtures.
    """

    def setUp(self):
        super(_WritesPythonModules, self).setUp()

        from twisted.python.modules import getModule, PythonPath
        from twisted.python.filepath import FilePath

        self.getModule = getModule
        self.PythonPath = PythonPath
        self.FilePath = FilePath

        self.originalSysModules = set(sys.modules.keys())
        self.savedSysPath = sys.path[:]

        self.pathDir = tempfile.mkdtemp()
        self.makeImportable(self.pathDir)

    def tearDown(self):
        super(_WritesPythonModules, self).tearDown()

        sys.path[:] = self.savedSysPath
        modulesToDelete = six.viewkeys(sys.modules) - self.originalSysModules
        for module in modulesToDelete:
            del sys.modules[module]

        shutil.rmtree(self.pathDir)

    def makeImportable(self, path):
        sys.path.append(path)

    def writeSourceInto(self, source, path, moduleName):
        directory = self.FilePath(path)

        module = directory.child(moduleName)
        # FilePath always opens a file in binary mode - but that will
        # break on Python 3
        with open(module.path, 'w') as f:
            f.write(textwrap.dedent(source))

        return self.PythonPath([directory.path])

    def makeModule(self, source, path, moduleName):
        pythonModuleName, _ = os.path.splitext(moduleName)
        return self.writeSourceInto(source, path, moduleName)[pythonModuleName]

    def attributesAsDict(self, hasIterAttributes):
        return {attr.name: attr for attr in hasIterAttributes.iterAttributes()}

    def loadModuleAsDict(self, module):
        module.load()
        return self.attributesAsDict(module)

    def makeModuleAsDict(self, source, path, name):
        return self.loadModuleAsDict(self.makeModule(source, path, name))


@skipIf(not isTwistedInstalled(), "Twisted is not installed.")
class OriginalLocationTests(_WritesPythonModules):
    """
    Tests that L{isOriginalLocation} detects when a
    L{PythonAttribute}'s FQPN refers to an object inside the module
    where it was defined.

    For example: A L{twisted.python.modules.PythonAttribute} with a
    name of 'foo.bar' that refers to a 'bar' object defined in module
    'baz' does *not* refer to bar's original location, while a
    L{PythonAttribute} with a name of 'baz.bar' does.

    """
    def setUp(self):
        super(OriginalLocationTests, self).setUp()
        from .._discover import isOriginalLocation
        self.isOriginalLocation = isOriginalLocation

    def test_failsWithNoModule(self):
        """
        L{isOriginalLocation} returns False when the attribute refers to an
        object whose source module cannot be determined.
        """
        source = """\
        class Fake(object):
            pass
        hasEmptyModule = Fake()
        hasEmptyModule.__module__ = None
        """

        moduleDict = self.makeModuleAsDict(source,
                                           self.pathDir,
                                           'empty_module_attr.py')

        self.assertFalse(self.isOriginalLocation(
            moduleDict['empty_module_attr.hasEmptyModule']))

    def test_failsWithDifferentModule(self):
        """
        L{isOriginalLocation} returns False when the attribute refers to
        an object outside of the module where that object was defined.
        """
        originalSource = """\
        class ImportThisClass(object):
            pass
        importThisObject = ImportThisClass()
        importThisNestingObject = ImportThisClass()
        importThisNestingObject.nestedObject = ImportThisClass()
        """

        importingSource = """\
        from original import (ImportThisClass,
                              importThisObject,
                              importThisNestingObject)
        """

        self.makeModule(originalSource, self.pathDir, 'original.py')
        importingDict = self.makeModuleAsDict(importingSource,
                                              self.pathDir,
                                              'importing.py')
        self.assertFalse(
            self.isOriginalLocation(
                importingDict['importing.ImportThisClass']))
        self.assertFalse(
            self.isOriginalLocation(
                importingDict['importing.importThisObject']))

        nestingObject = importingDict['importing.importThisNestingObject']
        nestingObjectDict = self.attributesAsDict(nestingObject)
        nestedObject = nestingObjectDict[
            'importing.importThisNestingObject.nestedObject']

        self.assertFalse(self.isOriginalLocation(nestedObject))

    def test_succeedsWithSameModule(self):
        """
        L{isOriginalLocation} returns True when the attribute refers to an
        object inside the module where that object was defined.
        """
        mSource = textwrap.dedent("""
        class ThisClassWasDefinedHere(object):
            pass
        anObject = ThisClassWasDefinedHere()
        aNestingObject = ThisClassWasDefinedHere()
        aNestingObject.nestedObject = ThisClassWasDefinedHere()
        """)
        mDict = self.makeModuleAsDict(mSource, self.pathDir, 'm.py')
        self.assertTrue(self.isOriginalLocation(
            mDict['m.ThisClassWasDefinedHere']))
        self.assertTrue(self.isOriginalLocation(mDict['m.aNestingObject']))

        nestingObject = mDict['m.aNestingObject']
        nestingObjectDict = self.attributesAsDict(nestingObject)
        nestedObject = nestingObjectDict['m.aNestingObject.nestedObject']

        self.assertTrue(self.isOriginalLocation(nestedObject))


@skipIf(not isTwistedInstalled(), "Twisted is not installed.")
class FindMachinesViaWrapperTests(_WritesPythonModules):
    """
    L{findMachinesViaWrapper} recursively yields FQPN,
    L{MethodicalMachine} pairs in and under a given
    L{twisted.python.modules.PythonModule} or
    L{twisted.python.modules.PythonAttribute}.
    """
    TEST_MODULE_SOURCE = """
    from automat import MethodicalMachine


    class PythonClass(object):
        _classMachine = MethodicalMachine()

        class NestedClass(object):
            _nestedClassMachine = MethodicalMachine()

        ignoredAttribute = "I am ignored."

        def ignoredMethod(self):
            "I am also ignored."

    rootLevelMachine = MethodicalMachine()
    ignoredPythonObject = PythonClass()
    anotherIgnoredPythonObject = "I am ignored."
    """

    def setUp(self):
        super(FindMachinesViaWrapperTests, self).setUp()
        from .._discover import findMachinesViaWrapper
        self.findMachinesViaWrapper = findMachinesViaWrapper

    def test_yieldsMachine(self):
        """
        When given a L{twisted.python.modules.PythonAttribute} that refers
        directly to a L{MethodicalMachine}, L{findMachinesViaWrapper}
        yields that machine and its FQPN.
        """
        source = """\
        from automat import MethodicalMachine

        rootMachine = MethodicalMachine()
        """

        moduleDict = self.makeModuleAsDict(source, self.pathDir, 'root.py')
        rootMachine = moduleDict['root.rootMachine']
        self.assertIn(('root.rootMachine', rootMachine.load()),
                      list(self.findMachinesViaWrapper(rootMachine)))

    def test_yieldsMachineInClass(self):
        """
        When given a L{twisted.python.modules.PythonAttribute} that refers
        to a class that contains a L{MethodicalMachine} as a class
        variable, L{findMachinesViaWrapper} yields that machine and
        its FQPN.
        """
        source = """\
        from automat import MethodicalMachine

        class PythonClass(object):
            _classMachine = MethodicalMachine()
        """
        moduleDict = self.makeModuleAsDict(source, self.pathDir, 'clsmod.py')
        PythonClass = moduleDict['clsmod.PythonClass']
        self.assertIn(('clsmod.PythonClass._classMachine',
                       PythonClass.load()._classMachine),
                      list(self.findMachinesViaWrapper(PythonClass)))

    def test_yieldsMachineInNestedClass(self):
        """
        When given a L{twisted.python.modules.PythonAttribute} that refers
        to a nested class that contains a L{MethodicalMachine} as a
        class variable, L{findMachinesViaWrapper} yields that machine
        and its FQPN.
        """
        source = """\
        from automat import MethodicalMachine

        class PythonClass(object):
            class NestedClass(object):
                _classMachine = MethodicalMachine()
        """
        moduleDict = self.makeModuleAsDict(source,
                                           self.pathDir,
                                           'nestedcls.py')

        PythonClass = moduleDict['nestedcls.PythonClass']
        self.assertIn(('nestedcls.PythonClass.NestedClass._classMachine',
                       PythonClass.load().NestedClass._classMachine),
                      list(self.findMachinesViaWrapper(PythonClass)))

    def test_yieldsMachineInModule(self):
        """
        When given a L{twisted.python.modules.PythonModule} that refers to
        a module that contains a L{MethodicalMachine},
        L{findMachinesViaWrapper} yields that machine and its FQPN.
        """
        source = """\
        from automat import MethodicalMachine

        rootMachine = MethodicalMachine()
        """
        module = self.makeModule(source, self.pathDir, 'root.py')
        rootMachine = self.loadModuleAsDict(module)['root.rootMachine'].load()
        self.assertIn(('root.rootMachine', rootMachine),
                      list(self.findMachinesViaWrapper(module)))

    def test_yieldsMachineInClassInModule(self):
        """
        When given a L{twisted.python.modules.PythonModule} that refers to
        the original module of a class containing a
        L{MethodicalMachine}, L{findMachinesViaWrapper} yields that
        machine and its FQPN.
        """
        source = """\
        from automat import MethodicalMachine

        class PythonClass(object):
            _classMachine = MethodicalMachine()
        """
        module = self.makeModule(source, self.pathDir, 'clsmod.py')
        PythonClass = self.loadModuleAsDict(
            module)['clsmod.PythonClass'].load()
        self.assertIn(('clsmod.PythonClass._classMachine',
                       PythonClass._classMachine),
                      list(self.findMachinesViaWrapper(module)))

    def test_yieldsMachineInNestedClassInModule(self):
        """
        When given a L{twisted.python.modules.PythonModule} that refers to
        the original module of a nested class containing a
        L{MethodicalMachine}, L{findMachinesViaWrapper} yields that
        machine and its FQPN.
        """
        source = """\
        from automat import MethodicalMachine

        class PythonClass(object):
            class NestedClass(object):
                _classMachine = MethodicalMachine()
        """
        module = self.makeModule(source, self.pathDir, 'nestedcls.py')
        PythonClass = self.loadModuleAsDict(
            module)['nestedcls.PythonClass'].load()

        self.assertIn(('nestedcls.PythonClass.NestedClass._classMachine',
                       PythonClass.NestedClass._classMachine),
                      list(self.findMachinesViaWrapper(module)))

    def test_ignoresImportedClass(self):
        """
        When given a L{twisted.python.modules.PythonAttribute} that refers
        to a class imported from another module, any
        L{MethodicalMachine}s on that class are ignored.

        This behavior ensures that a machine is only discovered on a
        class when visiting the module where that class was defined.
        """
        originalSource = """
        from automat import MethodicalMachine

        class PythonClass(object):
            _classMachine = MethodicalMachine()
        """

        importingSource = """
        from original import PythonClass
        """

        self.makeModule(originalSource, self.pathDir, 'original.py')
        importingModule = self.makeModule(importingSource,
                                          self.pathDir,
                                          'importing.py')

        self.assertFalse(list(self.findMachinesViaWrapper(importingModule)))

    def test_descendsIntoPackages(self):
        """
        L{findMachinesViaWrapper} descends into packages to discover
        machines.
        """
        pythonPath = self.PythonPath([self.pathDir])
        package = self.FilePath(self.pathDir).child("test_package")
        package.makedirs()
        package.child('__init__.py').touch()

        source = """
        from automat import MethodicalMachine


        class PythonClass(object):
            _classMachine = MethodicalMachine()


        rootMachine = MethodicalMachine()
        """
        self.makeModule(source, package.path, 'module.py')

        test_package = pythonPath['test_package']
        machines = sorted(self.findMachinesViaWrapper(test_package),
                          key=operator.itemgetter(0))

        moduleDict = self.loadModuleAsDict(test_package['module'])
        rootMachine = moduleDict['test_package.module.rootMachine'].load()
        PythonClass = moduleDict['test_package.module.PythonClass'].load()

        expectedMachines = sorted(
            [('test_package.module.rootMachine',
              rootMachine),
             ('test_package.module.PythonClass._classMachine',
              PythonClass._classMachine)], key=operator.itemgetter(0))

        self.assertEqual(expectedMachines, machines)

    def test_infiniteLoop(self):
        """
        L{findMachinesViaWrapper} ignores infinite loops.

        Note this test can't fail - it can only run forever!
        """
        source = """
        class InfiniteLoop(object):
            pass

        InfiniteLoop.loop = InfiniteLoop
        """
        module = self.makeModule(source, self.pathDir, 'loop.py')
        self.assertFalse(list(self.findMachinesViaWrapper(module)))


@skipIf(not isTwistedInstalled(), "Twisted is not installed.")
class WrapFQPNTests(TestCase):
    """
    Tests that ensure L{wrapFQPN} loads the
    L{twisted.python.modules.PythonModule} or
    L{twisted.python.modules.PythonAttribute} for a given FQPN.
    """

    def setUp(self):
        from twisted.python.modules import PythonModule, PythonAttribute
        from .._discover import wrapFQPN, InvalidFQPN, NoModule, NoObject

        self.PythonModule = PythonModule
        self.PythonAttribute = PythonAttribute
        self.wrapFQPN = wrapFQPN
        self.InvalidFQPN = InvalidFQPN
        self.NoModule = NoModule
        self.NoObject = NoObject

    def assertModuleWrapperRefersTo(self, moduleWrapper, module):
        """
        Assert that a L{twisted.python.modules.PythonModule} refers to a
        particular Python module.
        """
        self.assertIsInstance(moduleWrapper, self.PythonModule)
        self.assertEqual(moduleWrapper.name, module.__name__)
        self.assertIs(moduleWrapper.load(), module)

    def assertAttributeWrapperRefersTo(self, attributeWrapper, fqpn, obj):
        """
        Assert that a L{twisted.python.modules.PythonAttribute} refers to a
        particular Python object.
        """
        self.assertIsInstance(attributeWrapper, self.PythonAttribute)
        self.assertEqual(attributeWrapper.name, fqpn)
        self.assertIs(attributeWrapper.load(), obj)

    def test_failsWithEmptyFQPN(self):
        """
        L{wrapFQPN} raises L{InvalidFQPN} when given an empty string.
        """
        with self.assertRaises(self.InvalidFQPN):
            self.wrapFQPN('')

    def test_failsWithBadDotting(self):
        """"
        L{wrapFQPN} raises L{InvalidFQPN} when given a badly-dotted
        FQPN.  (e.g., x..y).
        """
        for bad in ('.fails', 'fails.', 'this..fails'):
            with self.assertRaises(self.InvalidFQPN):
                self.wrapFQPN(bad)

    def test_singleModule(self):
        """
        L{wrapFQPN} returns a L{twisted.python.modules.PythonModule}
        referring to the single module a dotless FQPN describes.
        """
        import os

        moduleWrapper = self.wrapFQPN('os')

        self.assertIsInstance(moduleWrapper, self.PythonModule)
        self.assertIs(moduleWrapper.load(), os)

    def test_failsWithMissingSingleModuleOrPackage(self):
        """
        L{wrapFQPN} raises L{NoModule} when given a dotless FQPN that does
        not refer to a module or package.
        """
        with self.assertRaises(self.NoModule):
            self.wrapFQPN("this is not an acceptable name!")

    def test_singlePackage(self):
        """
        L{wrapFQPN} returns a L{twisted.python.modules.PythonModule}
        referring to the single package a dotless FQPN describes.
        """
        import xml
        self.assertModuleWrapperRefersTo(self.wrapFQPN('xml'), xml)

    def test_multiplePackages(self):
        """
        L{wrapFQPN} returns a L{twisted.python.modules.PythonModule}
        referring to the deepest package described by dotted FQPN.
        """
        import xml.etree
        self.assertModuleWrapperRefersTo(self.wrapFQPN('xml.etree'), xml.etree)

    def test_multiplePackagesFinalModule(self):
        """
        L{wrapFQPN} returns a L{twisted.python.modules.PythonModule}
        referring to the deepest module described by dotted FQPN.
        """
        import xml.etree.ElementTree
        self.assertModuleWrapperRefersTo(
            self.wrapFQPN('xml.etree.ElementTree'), xml.etree.ElementTree)

    def test_singleModuleObject(self):
        """
        L{wrapFQPN} returns a L{twisted.python.modules.PythonAttribute}
        referring to the deepest object an FQPN names, traversing one module.
        """
        import os
        self.assertAttributeWrapperRefersTo(
            self.wrapFQPN('os.path'), 'os.path', os.path)

    def test_multiplePackagesObject(self):
        """
        L{wrapFQPN} returns a L{twisted.python.modules.PythonAttribute}
        referring to the deepest object described by an FQPN,
        descending through several packages.
        """
        import xml.etree.ElementTree
        import automat

        for fqpn, obj in [('xml.etree.ElementTree.fromstring',
                           xml.etree.ElementTree.fromstring),
                          ('automat.MethodicalMachine.__doc__',
                           automat.MethodicalMachine.__doc__)]:
            self.assertAttributeWrapperRefersTo(
                self.wrapFQPN(fqpn), fqpn, obj)

    def test_failsWithMultiplePackagesMissingModuleOrPackage(self):
        """
        L{wrapFQPN} raises L{NoObject} when given an FQPN that contains a
        missing attribute, module, or package.
        """
        for bad in ('xml.etree.nope!',
                    'xml.etree.nope!.but.the.rest.is.believable'):
            with self.assertRaises(self.NoObject):
                self.wrapFQPN(bad)


@skipIf(not isTwistedInstalled(), "Twisted is not installed.")
class FindMachinesIntegrationTests(_WritesPythonModules):
    """
    Integration tests to check that L{findMachines} yields all
    machines discoverable at or below an FQPN.
    """

    SOURCE = """
    from automat import MethodicalMachine

    class PythonClass(object):
        _machine = MethodicalMachine()
        ignored = "i am ignored"

    rootLevel = MethodicalMachine()

    ignored = "i am ignored"
    """

    def setUp(self):
        super(FindMachinesIntegrationTests, self).setUp()
        from .._discover import findMachines

        self.findMachines = findMachines

        packageDir = self.FilePath(self.pathDir).child("test_package")
        packageDir.makedirs()
        self.pythonPath = self.PythonPath([self.pathDir])
        self.writeSourceInto(self.SOURCE, packageDir.path, '__init__.py')

        subPackageDir = packageDir.child('subpackage')
        subPackageDir.makedirs()
        subPackageDir.child('__init__.py').touch()

        self.makeModule(self.SOURCE, subPackageDir.path, 'module.py')

        self.packageDict = self.loadModuleAsDict(
            self.pythonPath['test_package'])
        self.moduleDict = self.loadModuleAsDict(
            self.pythonPath['test_package']['subpackage']['module'])

    def test_discoverAll(self):
        """
        Given a top-level package FQPN, L{findMachines} discovers all
        L{MethodicalMachine} instances in and below it.
        """
        machines = sorted(self.findMachines('test_package'),
                          key=operator.itemgetter(0))

        tpRootLevel = self.packageDict['test_package.rootLevel'].load()
        tpPythonClass = self.packageDict['test_package.PythonClass'].load()

        mRLAttr = self.moduleDict['test_package.subpackage.module.rootLevel']
        mRootLevel = mRLAttr.load()
        mPCAttr = self.moduleDict['test_package.subpackage.module.PythonClass']
        mPythonClass = mPCAttr.load()

        expectedMachines = sorted(
            [('test_package.rootLevel', tpRootLevel),
             ('test_package.PythonClass._machine', tpPythonClass._machine),
             ('test_package.subpackage.module.rootLevel', mRootLevel),
             ('test_package.subpackage.module.PythonClass._machine',
              mPythonClass._machine)],
            key=operator.itemgetter(0))

        self.assertEqual(expectedMachines, machines)
