from __future__ import print_function
import functools

import os
import subprocess
from unittest import TestCase, skipIf

import attr

from .._methodical import MethodicalMachine

from .test_discover import isTwistedInstalled


def isGraphvizModuleInstalled():
    """
    Is the graphviz Python module installed?
    """
    try:
        __import__("graphviz")
    except ImportError:
        return False
    else:
        return True


def isGraphvizInstalled():
    """
    Are the graphviz tools installed?
    """
    r, w = os.pipe()
    os.close(w)
    try:
        return not subprocess.call("dot", stdin=r, shell=True)
    finally:
        os.close(r)



def sampleMachine():
    """
    Create a sample L{MethodicalMachine} with some sample states.
    """
    mm = MethodicalMachine()
    class SampleObject(object):
        @mm.state(initial=True)
        def begin(self):
            "initial state"
        @mm.state()
        def end(self):
            "end state"
        @mm.input()
        def go(self):
            "sample input"
        @mm.output()
        def out(self):
            "sample output"
        begin.upon(go, end, [out])
    so = SampleObject()
    so.go()
    return mm


@skipIf(not isGraphvizModuleInstalled(), "Graphviz module is not installed.")
class ElementMakerTests(TestCase):
    """
    L{elementMaker} generates HTML representing the specified element.
    """

    def setUp(self):
        from .._visualize import elementMaker
        self.elementMaker = elementMaker

    def test_sortsAttrs(self):
        """
        L{elementMaker} orders HTML attributes lexicographically.
        """
        expected = r'<div a="1" b="2" c="3"></div>'
        self.assertEqual(expected,
                         self.elementMaker("div",
                                           b='2',
                                           a='1',
                                           c='3'))

    def test_quotesAttrs(self):
        """
        L{elementMaker} quotes HTML attributes according to DOT's quoting rule.

        See U{http://www.graphviz.org/doc/info/lang.html}, footnote 1.
        """
        expected = r'<div a="1" b="a \" quote" c="a string"></div>'
        self.assertEqual(expected,
                         self.elementMaker("div",
                                           b='a " quote',
                                           a=1,
                                           c="a string"))

    def test_noAttrs(self):
        """
        L{elementMaker} should render an element with no attributes.
        """
        expected = r'<div ></div>'
        self.assertEqual(expected, self.elementMaker("div"))


@attr.s
class HTMLElement(object):
    """Holds an HTML element, as created by elementMaker."""
    name = attr.ib()
    children = attr.ib()
    attributes = attr.ib()


def findElements(element, predicate):
    """
    Recursively collect all elements in an L{HTMLElement} tree that
    match the optional predicate.
    """
    if predicate(element):
        return [element]
    elif isLeaf(element):
        return []

    return [result
            for child in element.children
            for result in findElements(child, predicate)]


def isLeaf(element):
    """
    This HTML element is actually leaf node.
    """
    return not isinstance(element, HTMLElement)


@skipIf(not isGraphvizModuleInstalled(), "Graphviz module is not installed.")
class TableMakerTests(TestCase):
    """
    Tests that ensure L{tableMaker} generates HTML tables usable as
    labels in DOT graphs.

    For more information, read the "HTML-Like Labels" section of
    U{http://www.graphviz.org/doc/info/shapes.html}.
    """

    def fakeElementMaker(self, name, *children, **attributes):
        return HTMLElement(name=name, children=children, attributes=attributes)

    def setUp(self):
        from .._visualize import tableMaker

        self.inputLabel = "input label"
        self.port = "the port"
        self.tableMaker = functools.partial(tableMaker,
                                            _E=self.fakeElementMaker)

    def test_inputLabelRow(self):
        """
        The table returned by L{tableMaker} always contains the input
        symbol label in its first row, and that row contains one cell
        with a port attribute set to the provided port.
        """

        def hasPort(element):
            return (not isLeaf(element)
                    and element.attributes.get("port") == self.port)

        for outputLabels in ([], ["an output label"]):
            table = self.tableMaker(self.inputLabel, outputLabels,
                                    port=self.port)
            self.assertGreater(len(table.children), 0)
            inputLabelRow = table.children[0]

            portCandidates = findElements(table, hasPort)

            self.assertEqual(len(portCandidates), 1)
            self.assertEqual(portCandidates[0].name, "td")
            self.assertEqual(findElements(inputLabelRow, isLeaf),
                             [self.inputLabel])

    def test_noOutputLabels(self):
        """
        L{tableMaker} does not add a colspan attribute to the input
        label's cell or a second row if there no output labels.
        """
        table = self.tableMaker("input label", (), port=self.port)
        self.assertEqual(len(table.children), 1)
        (inputLabelRow,) = table.children
        self.assertNotIn("colspan", inputLabelRow.attributes)

    def test_withOutputLabels(self):
        """
        L{tableMaker} adds a colspan attribute to the input label's cell
        equal to the number of output labels and a second row that
        contains the output labels.
        """
        table = self.tableMaker(self.inputLabel, ("output label 1",
                                                  "output label 2"),
                                port=self.port)

        self.assertEqual(len(table.children), 2)
        inputRow, outputRow = table.children

        def hasCorrectColspan(element):
            return (not isLeaf(element)
                    and element.name == "td"
                    and element.attributes.get('colspan') == "2")

        self.assertEqual(len(findElements(inputRow, hasCorrectColspan)),
                         1)
        self.assertEqual(findElements(outputRow, isLeaf), ["output label 1",
                                                           "output label 2"])


@skipIf(not isGraphvizModuleInstalled(), "Graphviz module is not installed.")
@skipIf(not isGraphvizInstalled(), "Graphviz tools are not installed.")
class IntegrationTests(TestCase):
    """
    Tests which make sure Graphviz can understand the output produced by
    Automat.
    """

    def test_validGraphviz(self):
        """
        L{graphviz} emits valid graphviz data.
        """
        p = subprocess.Popen("dot", stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE)
        out, err = p.communicate("".join(sampleMachine().asDigraph())
                                 .encode("utf-8"))
        self.assertEqual(p.returncode, 0)


@skipIf(not isGraphvizModuleInstalled(), "Graphviz module is not installed.")
class SpotChecks(TestCase):
    """
    Tests to make sure that the output contains salient features of the machine
    being generated.
    """

    def test_containsMachineFeatures(self):
        """
        The output of L{graphviz} should contain the names of the states,
        inputs, outputs in the state machine.
        """
        gvout = "".join(sampleMachine().asDigraph())
        self.assertIn("begin", gvout)
        self.assertIn("end", gvout)
        self.assertIn("go", gvout)
        self.assertIn("out", gvout)


class RecordsDigraphActions(object):
    """
    Records calls made to L{FakeDigraph}.
    """

    def __init__(self):
        self.reset()

    def reset(self):
        self.renderCalls = []
        self.saveCalls = []


class FakeDigraph(object):
    """
    A fake L{graphviz.Digraph}.  Instantiate it with a
    L{RecordsDigraphActions}.
    """

    def __init__(self, recorder):
        self._recorder = recorder

    def render(self, **kwargs):
        self._recorder.renderCalls.append(kwargs)

    def save(self, **kwargs):
        self._recorder.saveCalls.append(kwargs)


class FakeMethodicalMachine(object):
    """
    A fake L{MethodicalMachine}.  Instantiate it with a L{FakeDigraph}
    """

    def __init__(self, digraph):
        self._digraph = digraph

    def asDigraph(self):
        return self._digraph


@skipIf(not isGraphvizModuleInstalled(), "Graphviz module is not installed.")
@skipIf(not isGraphvizInstalled(), "Graphviz tools are not installed.")
@skipIf(not isTwistedInstalled(), "Twisted is not installed.")
class VisualizeToolTests(TestCase):

    def setUp(self):
        self.digraphRecorder = RecordsDigraphActions()
        self.fakeDigraph = FakeDigraph(self.digraphRecorder)

        self.fakeProgname = 'tool-test'
        self.fakeSysPath = ['ignored']
        self.collectedOutput = []
        self.fakeFQPN = 'fake.fqpn'

    def collectPrints(self, *args):
        self.collectedOutput.append(' '.join(args))

    def fakeFindMachines(self, fqpn):
        yield fqpn, FakeMethodicalMachine(self.fakeDigraph)

    def tool(self,
             progname=None,
             argv=None,
             syspath=None,
             findMachines=None,
             print=None):
        from .._visualize import tool
        return tool(
            _progname=progname or self.fakeProgname,
            _argv=argv or [self.fakeFQPN],
            _syspath=syspath or self.fakeSysPath,
            _findMachines=findMachines or self.fakeFindMachines,
            _print=print or self.collectPrints)

    def test_checksCurrentDirectory(self):
        """
        L{tool} adds '' to sys.path to ensure
        L{automat._discover.findMachines} searches the current
        directory.
        """
        self.tool(argv=[self.fakeFQPN])
        self.assertEqual(self.fakeSysPath[0], '')

    def test_quietHidesOutput(self):
        """
        Passing -q/--quiet hides all output.
        """
        self.tool(argv=[self.fakeFQPN, '--quiet'])
        self.assertFalse(self.collectedOutput)
        self.tool(argv=[self.fakeFQPN, '-q'])
        self.assertFalse(self.collectedOutput)

    def test_onlySaveDot(self):
        """
        Passing an empty string for --image-directory/-i disables
        rendering images.
        """
        for arg in ('--image-directory', '-i'):
            self.digraphRecorder.reset()
            self.collectedOutput = []

            self.tool(argv=[self.fakeFQPN, arg, ''])
            self.assertFalse(any("image" in line
                                 for line in self.collectedOutput))

            self.assertEqual(len(self.digraphRecorder.saveCalls), 1)
            (call,) = self.digraphRecorder.saveCalls
            self.assertEqual("{}.dot".format(self.fakeFQPN),
                             call['filename'])

            self.assertFalse(self.digraphRecorder.renderCalls)

    def test_saveOnlyImage(self):
        """
        Passing an empty string for --dot-directory/-d disables saving dot
        files.
        """
        for arg in ('--dot-directory', '-d'):
            self.digraphRecorder.reset()
            self.collectedOutput = []
            self.tool(argv=[self.fakeFQPN, arg, ''])

            self.assertFalse(any("dot" in line
                                 for line in self.collectedOutput))

            self.assertEqual(len(self.digraphRecorder.renderCalls), 1)
            (call,) = self.digraphRecorder.renderCalls
            self.assertEqual("{}.dot".format(self.fakeFQPN),
                             call['filename'])
            self.assertTrue(call['cleanup'])

            self.assertFalse(self.digraphRecorder.saveCalls)

    def test_saveDotAndImagesInDifferentDirectories(self):
        """
        Passing different directories to --image-directory and --dot-directory
        writes images and dot files to those directories.
        """
        imageDirectory = 'image'
        dotDirectory = 'dot'
        self.tool(argv=[self.fakeFQPN,
                        '--image-directory', imageDirectory,
                        '--dot-directory', dotDirectory])

        self.assertTrue(any("image" in line
                            for line in self.collectedOutput))
        self.assertTrue(any("dot" in line
                            for line in self.collectedOutput))

        self.assertEqual(len(self.digraphRecorder.renderCalls), 1)
        (renderCall,) = self.digraphRecorder.renderCalls
        self.assertEqual(renderCall["directory"], imageDirectory)
        self.assertTrue(renderCall['cleanup'])

        self.assertEqual(len(self.digraphRecorder.saveCalls), 1)
        (saveCall,) = self.digraphRecorder.saveCalls
        self.assertEqual(saveCall["directory"], dotDirectory)

    def test_saveDotAndImagesInSameDirectory(self):
        """
        Passing the same directory to --image-directory and --dot-directory
        writes images and dot files to that one directory.
        """
        directory = 'imagesAndDot'
        self.tool(argv=[self.fakeFQPN,
                        '--image-directory', directory,
                        '--dot-directory', directory])

        self.assertTrue(any("image and dot" in line
                            for line in self.collectedOutput))

        self.assertEqual(len(self.digraphRecorder.renderCalls), 1)
        (renderCall,) = self.digraphRecorder.renderCalls
        self.assertEqual(renderCall["directory"], directory)
        self.assertFalse(renderCall['cleanup'])

        self.assertFalse(len(self.digraphRecorder.saveCalls))
