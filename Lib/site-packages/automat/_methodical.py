# -*- test-case-name: automat._test.test_methodical -*-

import collections
from functools import wraps
from itertools import count

try:
    # Python 3
    from inspect import getfullargspec as getArgsSpec
except ImportError:
    # Python 2
    from inspect import getargspec as getArgsSpec

import attr
import six

from ._core import Transitioner, Automaton
from ._introspection import preserveName


ArgSpec = collections.namedtuple('ArgSpec', ['args', 'varargs', 'varkw',
                                             'defaults', 'kwonlyargs',
                                             'kwonlydefaults', 'annotations'])


def _getArgSpec(func):
    """
    Normalize inspect.ArgSpec across python versions
    and convert mutable attributes to immutable types.

    :param Callable func: A function.
    :return: The function's ArgSpec.
    :rtype: ArgSpec
    """
    spec = getArgsSpec(func)
    return ArgSpec(
        args=tuple(spec.args),
        varargs=spec.varargs,
        varkw=spec.varkw if six.PY3 else spec.keywords,
        defaults=spec.defaults if spec.defaults else (),
        kwonlyargs=tuple(spec.kwonlyargs) if six.PY3 else (),
        kwonlydefaults=(
            tuple(spec.kwonlydefaults.items())
            if spec.kwonlydefaults else ()
        ) if six.PY3 else (),
        annotations=tuple(spec.annotations.items()) if six.PY3 else (),
    )


def _getArgNames(spec):
    """
    Get the name of all arguments defined in a function signature.

    The name of * and ** arguments is normalized to "*args" and "**kwargs".

    :param ArgSpec spec: A function to interrogate for a signature.
    :return: The set of all argument names in `func`s signature.
    :rtype: Set[str]
    """
    return set(
        spec.args
        + spec.kwonlyargs
        + (('*args',) if spec.varargs else ())
        + (('**kwargs',) if spec.varkw else ())
        + spec.annotations
    )


def _keywords_only(f):
    """
    Decorate a function so all its arguments must be passed by keyword.

    A useful utility for decorators that take arguments so that they don't
    accidentally get passed the thing they're decorating as their first
    argument.

    Only works for methods right now.
    """
    @wraps(f)
    def g(self, **kw):
        return f(self, **kw)
    return g


@attr.s(frozen=True)
class MethodicalState(object):
    """
    A state for a L{MethodicalMachine}.
    """
    machine = attr.ib(repr=False)
    method = attr.ib()
    serialized = attr.ib(repr=False)

    def upon(self, input, enter, outputs, collector=list):
        """
        Declare a state transition within the :class:`automat.MethodicalMachine`
        associated with this :class:`automat.MethodicalState`:
        upon the receipt of the `input`, enter the `state`,
        emitting each output in `outputs`.

        :param MethodicalInput input: The input triggering a state transition.
        :param MethodicalState enter: The resulting state.
        :param Iterable[MethodicalOutput] outputs: The outputs to be triggered
            as a result of the declared state transition.
        :param Callable collector: The function to be used when collecting
            output return values.

        :raises TypeError: if any of the `outputs` signatures do not match
            the `inputs` signature.
        :raises ValueError: if the state transition from `self` via `input`
            has already been defined.
        """
        inputArgs = _getArgNames(input.argSpec)
        for output in outputs:
            outputArgs = _getArgNames(output.argSpec)
            if not outputArgs.issubset(inputArgs):
                raise TypeError(
                    "method {input} signature {inputSignature} "
                    "does not match output {output} "
                    "signature {outputSignature}".format(
                        input=input.method.__name__,
                        output=output.method.__name__,
                        inputSignature=getArgsSpec(input.method),
                        outputSignature=getArgsSpec(output.method),
                ))
        self.machine._oneTransition(self, input, enter, outputs, collector)

    def _name(self):
        return self.method.__name__


def _transitionerFromInstance(oself, symbol, automaton):
    """
    Get a L{Transitioner}
    """
    transitioner = getattr(oself, symbol, None)
    if transitioner is None:
        transitioner = Transitioner(
            automaton,
            automaton.initialState,
        )
        setattr(oself, symbol, transitioner)
    return transitioner


def _empty():
    pass

def _docstring():
    """docstring"""

def assertNoCode(inst, attribute, f):
    # The function body must be empty, i.e. "pass" or "return None", which
    # both yield the same bytecode: LOAD_CONST (None), RETURN_VALUE. We also
    # accept functions with only a docstring, which yields slightly different
    # bytecode, because the "None" is put in a different constant slot.

    # Unfortunately, this does not catch function bodies that return a
    # constant value, e.g. "return 1", because their code is identical to a
    # "return None". They differ in the contents of their constant table, but
    # checking that would require us to parse the bytecode, find the index
    # being returned, then making sure the table has a None at that index.

    if f.__code__.co_code not in (_empty.__code__.co_code,
                                  _docstring.__code__.co_code):
        raise ValueError("function body must be empty")


def _filterArgs(args, kwargs, inputSpec, outputSpec):
    """
    Filter out arguments that were passed to input that output won't accept.

    :param tuple args: The *args that input received.
    :param dict kwargs: The **kwargs that input received.
    :param ArgSpec inputSpec: The input's arg spec.
    :param ArgSpec outputSpec: The output's arg spec.
    :return: The args and kwargs that output will accept.
    :rtype: Tuple[tuple, dict]
    """
    named_args = tuple(zip(inputSpec.args[1:], args))
    if outputSpec.varargs:
        # Only return all args if the output accepts *args.
        return_args = args
    else:
        # Filter out arguments that don't appear
        # in the output's method signature.
        return_args = [v for n, v in named_args if n in outputSpec.args]

    # Get any of input's default arguments that were not passed.
    passed_arg_names = tuple(kwargs)
    for name, value in named_args:
        passed_arg_names += (name, value)
    defaults = zip(inputSpec.args[::-1], inputSpec.defaults[::-1])
    full_kwargs = {n: v for n, v in defaults if n not in passed_arg_names}
    full_kwargs.update(kwargs)

    if outputSpec.varkw:
        # Only pass all kwargs if the output method accepts **kwargs.
        return_kwargs = full_kwargs
    else:
        # Filter out names that the output method does not accept.
        all_accepted_names = outputSpec.args[1:] + outputSpec.kwonlyargs
        return_kwargs = {n: v for n, v in full_kwargs.items()
                         if n in all_accepted_names}

    return return_args, return_kwargs


@attr.s(eq=False, hash=False)
class MethodicalInput(object):
    """
    An input for a L{MethodicalMachine}.
    """
    automaton = attr.ib(repr=False)
    method = attr.ib(validator=assertNoCode)
    symbol = attr.ib(repr=False)
    collectors = attr.ib(default=attr.Factory(dict), repr=False)
    argSpec = attr.ib(init=False, repr=False)

    @argSpec.default
    def _buildArgSpec(self):
        return _getArgSpec(self.method)

    def __get__(self, oself, type=None):
        """
        Return a function that takes no arguments and returns values returned
        by output functions produced by the given L{MethodicalInput} in
        C{oself}'s current state.
        """
        transitioner = _transitionerFromInstance(oself, self.symbol,
                                                 self.automaton)
        @preserveName(self.method)
        @wraps(self.method)
        def doInput(*args, **kwargs):
            self.method(oself, *args, **kwargs)
            previousState = transitioner._state
            (outputs, outTracer) = transitioner.transition(self)
            collector = self.collectors[previousState]
            values = []
            for output in outputs:
                if outTracer:
                    outTracer(output._name())
                a, k = _filterArgs(args, kwargs, self.argSpec, output.argSpec)
                value = output(oself, *a, **k)
                values.append(value)
            return collector(values)
        return doInput

    def _name(self):
        return self.method.__name__


@attr.s(frozen=True)
class MethodicalOutput(object):
    """
    An output for a L{MethodicalMachine}.
    """
    machine = attr.ib(repr=False)
    method = attr.ib()
    argSpec = attr.ib(init=False, repr=False)

    @argSpec.default
    def _buildArgSpec(self):
        return _getArgSpec(self.method)

    def __get__(self, oself, type=None):
        """
        Outputs are private, so raise an exception when we attempt to get one.
        """
        raise AttributeError(
            "{cls}.{method} is a state-machine output method; "
            "to produce this output, call an input method instead.".format(
                cls=type.__name__,
                method=self.method.__name__
            )
        )


    def __call__(self, oself, *args, **kwargs):
        """
        Call the underlying method.
        """
        return self.method(oself, *args, **kwargs)

    def _name(self):
        return self.method.__name__

@attr.s(eq=False, hash=False)
class MethodicalTracer(object):
    automaton = attr.ib(repr=False)
    symbol = attr.ib(repr=False)


    def __get__(self, oself, type=None):
        transitioner = _transitionerFromInstance(oself, self.symbol,
                                                 self.automaton)
        def setTrace(tracer):
            transitioner.setTrace(tracer)
        return setTrace



counter = count()
def gensym():
    """
    Create a unique Python identifier.
    """
    return "_symbol_" + str(next(counter))



class MethodicalMachine(object):
    """
    A :class:`MethodicalMachine` is an interface to an `Automaton`
    that uses methods on a class.
    """

    def __init__(self):
        self._automaton = Automaton()
        self._reducers = {}
        self._symbol = gensym()


    def __get__(self, oself, type=None):
        """
        L{MethodicalMachine} is an implementation detail for setting up
        class-level state; applications should never need to access it on an
        instance.
        """
        if oself is not None:
            raise AttributeError(
                "MethodicalMachine is an implementation detail.")
        return self


    @_keywords_only
    def state(self, initial=False, terminal=False,
              serialized=None):
        """
        Declare a state, possibly an initial state or a terminal state.

        This is a decorator for methods, but it will modify the method so as
        not to be callable any more.

        :param bool initial: is this state the initial state?
            Only one state on this :class:`automat.MethodicalMachine`
            may be an initial state; more than one is an error.

        :param bool terminal: Is this state a terminal state?
            i.e. a state that the machine can end up in?
            (This is purely informational at this point.)

        :param Hashable serialized: a serializable value
            to be used to represent this state to external systems.
            This value should be hashable;
            :py:func:`unicode` is a good type to use.
        """
        def decorator(stateMethod):
            state = MethodicalState(machine=self,
                                    method=stateMethod,
                                    serialized=serialized)
            if initial:
                self._automaton.initialState = state
            return state
        return decorator


    @_keywords_only
    def input(self):
        """
        Declare an input.

        This is a decorator for methods.
        """
        def decorator(inputMethod):
            return MethodicalInput(automaton=self._automaton,
                                   method=inputMethod,
                                   symbol=self._symbol)
        return decorator


    @_keywords_only
    def output(self):
        """
        Declare an output.

        This is a decorator for methods.

        This method will be called when the state machine transitions to this
        state as specified in the decorated `output` method.
        """
        def decorator(outputMethod):
            return MethodicalOutput(machine=self, method=outputMethod)
        return decorator


    def _oneTransition(self, startState, inputToken, endState, outputTokens,
                       collector):
        """
        See L{MethodicalState.upon}.
        """
        # FIXME: tests for all of this (some of it is wrong)
        # if not isinstance(startState, MethodicalState):
        #     raise NotImplementedError("start state {} isn't a state"
        #                               .format(startState))
        # if not isinstance(inputToken, MethodicalInput):
        #     raise NotImplementedError("start state {} isn't an input"
        #                               .format(inputToken))
        # if not isinstance(endState, MethodicalState):
        #     raise NotImplementedError("end state {} isn't a state"
        #                               .format(startState))
        # for output in outputTokens:
        #     if not isinstance(endState, MethodicalState):
        #         raise NotImplementedError("output state {} isn't a state"
        #                                   .format(endState))
        self._automaton.addTransition(startState, inputToken, endState,
                                      tuple(outputTokens))
        inputToken.collectors[startState] = collector


    @_keywords_only
    def serializer(self):
        """

        """
        def decorator(decoratee):
            @wraps(decoratee)
            def serialize(oself):
                transitioner = _transitionerFromInstance(oself, self._symbol,
                                                         self._automaton)
                return decoratee(oself, transitioner._state.serialized)
            return serialize
        return decorator

    @_keywords_only
    def unserializer(self):
        """

        """
        def decorator(decoratee):
            @wraps(decoratee)
            def unserialize(oself, *args, **kwargs):
                state = decoratee(oself, *args, **kwargs)
                mapping = {}
                for eachState in self._automaton.states():
                    mapping[eachState.serialized] = eachState
                transitioner = _transitionerFromInstance(
                    oself, self._symbol, self._automaton)
                transitioner._state = mapping[state]
                return None # it's on purpose
            return unserialize
        return decorator

    @property
    def _setTrace(self):
        return MethodicalTracer(self._automaton, self._symbol)

    def asDigraph(self):
        """
        Generate a L{graphviz.Digraph} that represents this machine's
        states and transitions.

        @return: L{graphviz.Digraph} object; for more information, please
            see the documentation for
            U{graphviz<https://graphviz.readthedocs.io/>}

        """
        from ._visualize import makeDigraph
        return makeDigraph(
            self._automaton,
            stateAsString=lambda state: state.method.__name__,
            inputAsString=lambda input: input.method.__name__,
            outputAsString=lambda output: output.method.__name__,
        )
