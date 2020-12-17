
from .._core import Automaton, NoTransition

from unittest import TestCase

class CoreTests(TestCase):
    """
    Tests for Automat's (currently private, implementation detail) core.
    """

    def test_NoTransition(self):
        """
        A L{NoTransition} exception describes the state and input symbol
        that caused it.
        """
        # NoTransition requires two arguments
        with self.assertRaises(TypeError):
            NoTransition()

        state = "current-state"
        symbol = "transitionless-symbol"
        noTransitionException = NoTransition(state=state, symbol=symbol)

        self.assertIs(noTransitionException.symbol, symbol)

        self.assertIn(state, str(noTransitionException))
        self.assertIn(symbol, str(noTransitionException))


    def test_noOutputForInput(self):
        """
        L{Automaton.outputForInput} raises L{NoTransition} if no
        transition for that input is defined.
        """
        a = Automaton()
        self.assertRaises(NoTransition, a.outputForInput,
                          "no-state", "no-symbol")


    def test_oneTransition(self):
        """
        L{Automaton.addTransition} adds its input symbol to
        L{Automaton.inputAlphabet}, all its outputs to
        L{Automaton.outputAlphabet}, and causes L{Automaton.outputForInput} to
        start returning the new state and output symbols.
        """
        a = Automaton()
        a.addTransition("beginning", "begin", "ending", ["end"])
        self.assertEqual(a.inputAlphabet(), {"begin"})
        self.assertEqual(a.outputAlphabet(), {"end"})
        self.assertEqual(a.outputForInput("beginning", "begin"),
                         ("ending", ["end"]))
        self.assertEqual(a.states(), {"beginning", "ending"})


    def test_oneTransition_nonIterableOutputs(self):
        """
        L{Automaton.addTransition} raises a TypeError when given outputs
        that aren't iterable and doesn't add any transitions.
        """
        a = Automaton()
        nonIterableOutputs = 1
        self.assertRaises(
            TypeError,
            a.addTransition,
            "fromState", "viaSymbol", "toState", nonIterableOutputs)
        self.assertFalse(a.inputAlphabet())
        self.assertFalse(a.outputAlphabet())
        self.assertFalse(a.states())
        self.assertFalse(a.allTransitions())


    def test_initialState(self):
        """
        L{Automaton.initialState} is a descriptor that sets the initial
        state if it's not yet set, and raises L{ValueError} if it is.

        """
        a = Automaton()
        a.initialState = "a state"
        self.assertEqual(a.initialState, "a state")
        with self.assertRaises(ValueError):
            a.initialState = "another state"


# FIXME: addTransition for transition that's been added before
