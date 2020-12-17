
"""
Tests for the public interface of Automat.
"""

from functools import reduce
from unittest import TestCase

from automat._methodical import ArgSpec, _getArgNames, _getArgSpec, _filterArgs
from .. import MethodicalMachine, NoTransition
from .. import _methodical


class MethodicalTests(TestCase):
    """
    Tests for L{MethodicalMachine}.
    """

    def test_oneTransition(self):
        """
        L{MethodicalMachine} provides a way for you to declare a state machine
        with inputs, outputs, and states as methods.  When you have declared an
        input, an output, and a state, calling the input method in that state
        will produce the specified output.
        """

        class Machination(object):
            machine = MethodicalMachine()
            @machine.input()
            def anInput(self):
                "an input"

            @machine.output()
            def anOutput(self):
                "an output"
                return "an-output-value"

            @machine.output()
            def anotherOutput(self):
                "another output"
                return "another-output-value"

            @machine.state(initial=True)
            def anState(self):
                "a state"

            @machine.state()
            def anotherState(self):
                "another state"

            anState.upon(anInput, enter=anotherState, outputs=[anOutput])
            anotherState.upon(anInput, enter=anotherState,
                              outputs=[anotherOutput])

        m = Machination()
        self.assertEqual(m.anInput(), ["an-output-value"])
        self.assertEqual(m.anInput(), ["another-output-value"])


    def test_machineItselfIsPrivate(self):
        """
        L{MethodicalMachine} is an implementation detail.  If you attempt to
        access it on an instance of your class, you will get an exception.
        However, since tools may need to access it for the purposes of, for
        example, visualization, you may access it on the class itself.
        """
        expectedMachine = MethodicalMachine()
        class Machination(object):
            machine = expectedMachine
        machination = Machination()
        with self.assertRaises(AttributeError) as cm:
            machination.machine
        self.assertIn("MethodicalMachine is an implementation detail",
                      str(cm.exception))
        self.assertIs(Machination.machine, expectedMachine)


    def test_outputsArePrivate(self):
        """
        One of the benefits of using a state machine is that your output method
        implementations don't need to take invalid state transitions into
        account - the methods simply won't be called.  This property would be
        broken if client code called output methods directly, so output methods
        are not directly visible under their names.
        """
        class Machination(object):
            machine = MethodicalMachine()
            counter = 0
            @machine.input()
            def anInput(self):
                "an input"
            @machine.output()
            def anOutput(self):
                self.counter += 1
            @machine.state(initial=True)
            def state(self):
                "a machine state"
            state.upon(anInput, enter=state, outputs=[anOutput])
        mach1 = Machination()
        mach1.anInput()
        self.assertEqual(mach1.counter, 1)
        mach2 = Machination()
        with self.assertRaises(AttributeError) as cm:
            mach2.anOutput
        self.assertEqual(mach2.counter, 0)

        self.assertIn(
            "Machination.anOutput is a state-machine output method; to "
            "produce this output, call an input method instead.",
            str(cm.exception)
        )


    def test_multipleMachines(self):
        """
        Two machines may co-exist happily on the same instance; they don't
        interfere with each other.
        """
        class MultiMach(object):
            a = MethodicalMachine()
            b = MethodicalMachine()

            @a.input()
            def inputA(self):
                "input A"
            @b.input()
            def inputB(self):
                "input B"
            @a.state(initial=True)
            def initialA(self):
                "initial A"
            @b.state(initial=True)
            def initialB(self):
                "initial B"
            @a.output()
            def outputA(self):
                return "A"
            @b.output()
            def outputB(self):
                return "B"
            initialA.upon(inputA, initialA, [outputA])
            initialB.upon(inputB, initialB, [outputB])

        mm = MultiMach()
        self.assertEqual(mm.inputA(), ["A"])
        self.assertEqual(mm.inputB(), ["B"])


    def test_collectOutputs(self):
        """
        Outputs can be combined with the "collector" argument to "upon".
        """
        import operator
        class Machine(object):
            m = MethodicalMachine()
            @m.input()
            def input(self):
                "an input"
            @m.output()
            def outputA(self):
                return "A"
            @m.output()
            def outputB(self):
                return "B"
            @m.state(initial=True)
            def state(self):
                "a state"
            state.upon(input, state, [outputA, outputB],
                       collector=lambda x: reduce(operator.add, x))
        m = Machine()
        self.assertEqual(m.input(), "AB")


    def test_methodName(self):
        """
        Input methods preserve their declared names.
        """
        class Mech(object):
            m = MethodicalMachine()
            @m.input()
            def declaredInputName(self):
                "an input"
            @m.state(initial=True)
            def aState(self):
                "state"
        m = Mech()
        with self.assertRaises(TypeError) as cm:
            m.declaredInputName("too", "many", "arguments")
        self.assertIn("declaredInputName", str(cm.exception))


    def test_inputWithArguments(self):
        """
        If an input takes an argument, it will pass that along to its output.
        """
        class Mechanism(object):
            m = MethodicalMachine()
            @m.input()
            def input(self, x, y=1):
                "an input"
            @m.state(initial=True)
            def state(self):
                "a state"
            @m.output()
            def output(self, x, y=1):
                self._x = x
                return x + y
            state.upon(input, state, [output])

        m = Mechanism()
        self.assertEqual(m.input(3), [4])
        self.assertEqual(m._x, 3)


    def test_outputWithSubsetOfArguments(self):
        """
        Inputs pass arguments that output will accept.
        """
        class Mechanism(object):
            m = MethodicalMachine()
            @m.input()
            def input(self, x, y=1):
                "an input"
            @m.state(initial=True)
            def state(self):
                "a state"
            @m.output()
            def outputX(self, x):
                self._x = x
                return x
            @m.output()
            def outputY(self, y):
                self._y = y
                return y
            @m.output()
            def outputNoArgs(self):
                return None
            state.upon(input, state, [outputX, outputY, outputNoArgs])

        m = Mechanism()

        # Pass x as positional argument.
        self.assertEqual(m.input(3), [3, 1, None])
        self.assertEqual(m._x, 3)
        self.assertEqual(m._y, 1)

        # Pass x as key word argument.
        self.assertEqual(m.input(x=4), [4, 1, None])
        self.assertEqual(m._x, 4)
        self.assertEqual(m._y, 1)

        # Pass y as positional argument.
        self.assertEqual(m.input(6, 3), [6, 3, None])
        self.assertEqual(m._x, 6)
        self.assertEqual(m._y, 3)

        # Pass y as key word argument.
        self.assertEqual(m.input(5, y=2), [5, 2, None])
        self.assertEqual(m._x, 5)
        self.assertEqual(m._y, 2)


    def test_inputFunctionsMustBeEmpty(self):
        """
        The wrapped input function must have an empty body.
        """
        # input functions are executed to assert that the signature matches,
        # but their body must be empty

        _methodical._empty() # chase coverage
        _methodical._docstring()

        class Mechanism(object):
            m = MethodicalMachine()
            with self.assertRaises(ValueError) as cm:
                @m.input()
                def input(self):
                    "an input"
                    list() # pragma: no cover
            self.assertEqual(str(cm.exception), "function body must be empty")

        # all three of these cases should be valid. Functions/methods with
        # docstrings produce slightly different bytecode than ones without.

        class MechanismWithDocstring(object):
            m = MethodicalMachine()
            @m.input()
            def input(self):
                "an input"
            @m.state(initial=True)
            def start(self):
                "starting state"
            start.upon(input, enter=start, outputs=[])
        MechanismWithDocstring().input()

        class MechanismWithPass(object):
            m = MethodicalMachine()
            @m.input()
            def input(self):
                pass
            @m.state(initial=True)
            def start(self):
                "starting state"
            start.upon(input, enter=start, outputs=[])
        MechanismWithPass().input()

        class MechanismWithDocstringAndPass(object):
            m = MethodicalMachine()
            @m.input()
            def input(self):
                "an input"
                pass
            @m.state(initial=True)
            def start(self):
                "starting state"
            start.upon(input, enter=start, outputs=[])
        MechanismWithDocstringAndPass().input()

        class MechanismReturnsNone(object):
            m = MethodicalMachine()
            @m.input()
            def input(self):
                return None
            @m.state(initial=True)
            def start(self):
                "starting state"
            start.upon(input, enter=start, outputs=[])
        MechanismReturnsNone().input()

        class MechanismWithDocstringAndReturnsNone(object):
            m = MethodicalMachine()
            @m.input()
            def input(self):
                "an input"
                return None
            @m.state(initial=True)
            def start(self):
                "starting state"
            start.upon(input, enter=start, outputs=[])
        MechanismWithDocstringAndReturnsNone().input()


    def test_inputOutputMismatch(self):
        """
        All the argument lists of the outputs for a given input must match; if
        one does not the call to C{upon} will raise a C{TypeError}.
        """
        class Mechanism(object):
            m = MethodicalMachine()
            @m.input()
            def nameOfInput(self, a):
                "an input"
            @m.output()
            def outputThatMatches(self, a):
                "an output that matches"
            @m.output()
            def outputThatDoesntMatch(self, b):
                "an output that doesn't match"
            @m.state()
            def state(self):
                "a state"
            with self.assertRaises(TypeError) as cm:
                state.upon(nameOfInput, state, [outputThatMatches,
                                                outputThatDoesntMatch])
            self.assertIn("nameOfInput", str(cm.exception))
            self.assertIn("outputThatDoesntMatch", str(cm.exception))


    def test_getArgNames(self):
        """
        Type annotations should be included in the set of
        """
        spec = ArgSpec(
            args=('a', 'b'),
            varargs=None,
            varkw=None,
            defaults=None,
            kwonlyargs=(),
            kwonlydefaults=None,
            annotations=(('a', int), ('b', str)),
        )
        self.assertEqual(
            _getArgNames(spec),
            {'a', 'b', ('a', int), ('b', str)},
        )


    def test_filterArgs(self):
        """
        filterArgs() should not filter the `args` parameter
        if outputSpec accepts `*args`.
        """
        inputSpec = _getArgSpec(lambda *args, **kwargs: None)
        outputSpec = _getArgSpec(lambda *args, **kwargs: None)
        argsIn = ()
        argsOut, _ = _filterArgs(argsIn, {}, inputSpec, outputSpec)
        self.assertIs(argsIn, argsOut)


    def test_multipleInitialStatesFailure(self):
        """
        A L{MethodicalMachine} can only have one initial state.
        """

        class WillFail(object):
            m = MethodicalMachine()

            @m.state(initial=True)
            def firstInitialState(self):
                "The first initial state -- this is OK."

            with self.assertRaises(ValueError):
                @m.state(initial=True)
                def secondInitialState(self):
                    "The second initial state -- results in a ValueError."


    def test_multipleTransitionsFailure(self):
        """
        A L{MethodicalMachine} can only have one transition per start/event
        pair.
        """

        class WillFail(object):
            m = MethodicalMachine()

            @m.state(initial=True)
            def start(self):
                "We start here."
            @m.state()
            def end(self):
                "Rainbows end."

            @m.input()
            def event(self):
                "An event."
            start.upon(event, enter=end, outputs=[])
            with self.assertRaises(ValueError):
                start.upon(event, enter=end, outputs=[])


    def test_badTransitionForCurrentState(self):
        """
        Calling any input method that lacks a transition for the machine's
        current state raises an informative L{NoTransition}.
        """

        class OnlyOnePath(object):
            m = MethodicalMachine()
            @m.state(initial=True)
            def start(self):
                "Start state."
            @m.state()
            def end(self):
                "End state."
            @m.input()
            def advance(self):
                "Move from start to end."
            @m.input()
            def deadEnd(self):
                "A transition from nowhere to nowhere."
            start.upon(advance, end, [])

        machine = OnlyOnePath()
        with self.assertRaises(NoTransition) as cm:
            machine.deadEnd()
        self.assertIn("deadEnd", str(cm.exception))
        self.assertIn("start", str(cm.exception))
        machine.advance()
        with self.assertRaises(NoTransition) as cm:
            machine.deadEnd()
        self.assertIn("deadEnd", str(cm.exception))
        self.assertIn("end", str(cm.exception))


    def test_saveState(self):
        """
        L{MethodicalMachine.serializer} is a decorator that modifies its
        decoratee's signature to take a "state" object as its first argument,
        which is the "serialized" argument to the L{MethodicalMachine.state}
        decorator.
        """

        class Mechanism(object):
            m = MethodicalMachine()
            def __init__(self):
                self.value = 1
            @m.state(serialized="first-state", initial=True)
            def first(self):
                "First state."
            @m.state(serialized="second-state")
            def second(self):
                "Second state."
            @m.serializer()
            def save(self, state):
                return {
                    'machine-state': state,
                    'some-value': self.value,
                }

        self.assertEqual(
            Mechanism().save(),
            {
                "machine-state": "first-state",
                "some-value": 1,
            }
        )


    def test_restoreState(self):
        """
        L{MethodicalMachine.unserializer} decorates a function that becomes a
        machine-state unserializer; its return value is mapped to the
        C{serialized} parameter to C{state}, and the L{MethodicalMachine}
        associated with that instance's state is updated to that state.
        """

        class Mechanism(object):
            m = MethodicalMachine()
            def __init__(self):
                self.value = 1
                self.ranOutput = False
            @m.state(serialized="first-state", initial=True)
            def first(self):
                "First state."
            @m.state(serialized="second-state")
            def second(self):
                "Second state."
            @m.input()
            def input(self):
                "an input"
            @m.output()
            def output(self):
                self.value = 2
                self.ranOutput = True
                return 1
            @m.output()
            def output2(self):
                return 2
            first.upon(input, second, [output],
                       collector=lambda x: list(x)[0])
            second.upon(input, second, [output2],
                        collector=lambda x: list(x)[0])
            @m.serializer()
            def save(self, state):
                return {
                    'machine-state': state,
                    'some-value': self.value,
                }

            @m.unserializer()
            def _restore(self, blob):
                self.value = blob['some-value']
                return blob['machine-state']

            @classmethod
            def fromBlob(cls, blob):
                self = cls()
                self._restore(blob)
                return self

        m1 = Mechanism()
        m1.input()
        blob = m1.save()
        m2 = Mechanism.fromBlob(blob)
        self.assertEqual(m2.ranOutput, False)
        self.assertEqual(m2.input(), 2)
        self.assertEqual(
            m2.save(),
            {
                'machine-state': 'second-state',
                'some-value': 2,
            }
        )



# FIXME: error for wrong types on any call to _oneTransition
# FIXME: better public API for .upon; maybe a context manager?
# FIXME: when transitions are defined, validate that we can always get to
# terminal? do we care about this?
# FIXME: implementation (and use-case/example) for passing args from in to out

# FIXME: possibly these need some kind of support from core
# FIXME: wildcard state (in all states, when input X, emit Y and go to Z)
# FIXME: wildcard input (in state X, when any input, emit Y and go to Z)
# FIXME: combined wildcards (in any state for any input, emit Y go to Z)
