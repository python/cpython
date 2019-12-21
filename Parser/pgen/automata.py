"""Classes representing state-machine concepts"""

class NFA:
    """A non deterministic finite automata

    A non deterministic automata is a form of a finite state
    machine. An NFA's rules are less restrictive than a DFA.
    The NFA rules are:

      * A transition can be non-deterministic and can result in
        nothing, one, or two or more states.

      * An epsilon transition consuming empty input is valid.
        Transitions consuming labeled symbols are also permitted.

    This class assumes that there is only one starting state and one
    accepting (ending) state.

    Attributes:
        name (str): The name of the rule the NFA is representing.
        start (NFAState): The starting state.
        end (NFAState): The ending state
    """

    def __init__(self, start, end):
        self.name = start.rule_name
        self.start = start
        self.end = end

    def __repr__(self):
        return "NFA(start={}, end={})".format(self.start, self.end)

    def dump(self, writer=print):
        """Dump a graphical representation of the NFA"""
        todo = [self.start]
        for i, state in enumerate(todo):
            writer("  State", i, state is self.end and "(final)" or "")
            for arc in state.arcs:
                label = arc.label
                next = arc.target
                if next in todo:
                    j = todo.index(next)
                else:
                    j = len(todo)
                    todo.append(next)
                if label is None:
                    writer("    -> %d" % j)
                else:
                    writer("    %s -> %d" % (label, j))


class NFAArc:
    """An arc representing a transition between two NFA states.

    NFA states can be connected via two ways:

        * A label transition: An input equal to the label must
          be consumed to perform the transition.
        * An epsilon transition: The transition can be taken without
          consuming any input symbol.

        Attributes:
            target (NFAState): The ending state of the transition arc.
            label (Optional[str]): The label that must be consumed to make
                the transition. An epsilon transition is represented
                using `None`.
    """

    def __init__(self, target, label):
        self.target = target
        self.label = label

    def __repr__(self):
        return "<%s: %s>" % (self.__class__.__name__, self.label)


class NFAState:
    """A state of a NFA, non deterministic finite automata.

    Attributes:
        target (rule_name): The name of the rule used to represent the NFA's
            ending state after a transition.
        arcs (Dict[Optional[str], NFAState]): A mapping representing transitions
            between the current NFA state and another NFA state via following
            a label.
    """

    def __init__(self, rule_name):
        self.rule_name = rule_name
        self.arcs = []

    def add_arc(self, target, label=None):
        """Add a new arc to connect the state to a target state within the NFA

        The method adds a new arc to the list of arcs available as transitions
        from the present state. An optional label indicates a named transition
        that consumes an input while the absence of a label represents an epsilon
        transition.

        Attributes:
            target (NFAState): The end of the transition that the arc represents.
            label (Optional[str]): The label that must be consumed for making
                the transition. If the label is not provided the transition is assumed
                to be an epsilon-transition.
        """
        assert label is None or isinstance(label, str)
        assert isinstance(target, NFAState)
        self.arcs.append(NFAArc(target, label))

    def __repr__(self):
        return "<%s: from %s>" % (self.__class__.__name__, self.rule_name)


class DFA:
    """A deterministic finite automata

    A deterministic finite automata is a form of a finite state machine
    that obeys the following rules:

       * Each of the transitions is uniquely determined by
         the source state and input symbol
       * Reading an input symbol is required for each state
         transition (no epsilon transitions).

    The finite-state machine will accept or reject a string of symbols
    and only produces a unique computation of the automaton for each input
    string. The DFA must have a unique starting state (represented as the first
    element in the list of states) but can have multiple final states.

    Attributes:
        name (str): The name of the rule the DFA is representing.
        states (List[DFAState]): A collection of DFA states.
    """

    def __init__(self, name, states):
        self.name = name
        self.states = states

    @classmethod
    def from_nfa(cls, nfa):
        """Constructs a DFA from a NFA using the Rabin–Scott construction algorithm.

        To simulate the operation of a DFA on a given input string, it's
        necessary to keep track of a single state at any time, or more precisely,
        the state that the automaton will reach after seeing a prefix of the
        input. In contrast, to simulate an NFA, it's necessary to keep track of
        a set of states: all of the states that the automaton could reach after
        seeing the same prefix of the input, according to the nondeterministic
        choices made by the automaton. There are two possible sources of
        non-determinism:

        1) Multiple (one or more) transitions with the same label

                         'A'     +-------+
                    +----------->+ State +----------->+
                    |            |   2   |
            +-------+            +-------+
            | State |
            |   1   |            +-------+
            +-------+            | State |
                    +----------->+   3   +----------->+
                         'A'     +-------+

        2) Epsilon transitions (transitions that can be taken without consuming any input)

            +-------+            +-------+
            | State |     ε      | State |
            |   1   +----------->+   2   +----------->+
            +-------+            +-------+

        Looking at the first case above, we can't determine which transition should be
        followed when given an input A. We could choose whether or not to follow the
        transition while in the second case the problem is that we can choose both to
        follow the transition or not doing it. To solve this problem we can imagine that
        we follow all possibilities at the same time and we construct new states from the
        set of all possible reachable states. For every case in the previous example:


        1) For multiple transitions with the same label we colapse all of the
           final states under the same one

            +-------+            +-------+
            | State |     'A'    | State |
            |   1   +----------->+  2-3  +----------->+
            +-------+            +-------+

        2) For epsilon transitions we collapse all epsilon-reachable states
           into the same one

            +-------+
            | State |
            |  1-2  +----------->
            +-------+

        Because the DFA states consist of sets of NFA states, an n-state NFA
        may be converted to a DFA with at most 2**n states. Notice that the
        constructed DFA is not minimal and can be simplified or reduced
        afterwards.

        Parameters:
            name (NFA): The NFA to transform to DFA.
        """
        assert isinstance(nfa, NFA)

        def add_closure(nfa_state, base_nfa_set):
            """Calculate the epsilon-closure of a given state

            Add to the *base_nfa_set* all the states that are
            reachable from *nfa_state* via epsilon-transitions.
            """
            assert isinstance(nfa_state, NFAState)
            if nfa_state in base_nfa_set:
                return
            base_nfa_set.add(nfa_state)
            for nfa_arc in nfa_state.arcs:
                if nfa_arc.label is None:
                    add_closure(nfa_arc.target, base_nfa_set)

        # Calculte the epsilon-closure of the starting state
        base_nfa_set = set()
        add_closure(nfa.start, base_nfa_set)

        # Start by visiting the NFA starting state (there is only one).
        states = [DFAState(nfa.name, base_nfa_set, nfa.end)]

        for state in states:  # NB states grow while we're iterating

            # Find transitions from the current state to other reachable states
            # and store them in mapping that correlates the label to all the
            # possible reachable states that can be obtained by consuming a
            # token equal to the label. Each set of all the states that can
            # be reached after following a label will be the a DFA state.
            arcs = {}
            for nfa_state in state.nfa_set:
                for nfa_arc in nfa_state.arcs:
                    if nfa_arc.label is not None:
                        nfa_set = arcs.setdefault(nfa_arc.label, set())
                        # All states that can be reached by epsilon-transitions
                        # are also included in the set of reachable states.
                        add_closure(nfa_arc.target, nfa_set)

            # Now create new DFAs by visiting all posible transitions between
            # the current DFA state and the new power-set states (each nfa_set)
            # via the different labels. As the nodes are appended to *states* this
            # is performing a breadth-first search traversal over the power-set of
            # the states of the original NFA.
            for label, nfa_set in sorted(arcs.items()):
                for exisisting_state in states:
                    if exisisting_state.nfa_set == nfa_set:
                        # The DFA state already exists for this rule.
                        next_state = exisisting_state
                        break
                else:
                    next_state = DFAState(nfa.name, nfa_set, nfa.end)
                    states.append(next_state)

                # Add a transition between the current DFA state and the new
                # DFA state (the power-set state) via the current label.
                state.add_arc(next_state, label)

        return cls(nfa.name, states)

    def __iter__(self):
        return iter(self.states)

    def simplify(self):
        """Attempt to reduce the number of states of the DFA

        Transform the DFA into an equivalent DFA that has fewer states. Two
        classes of states can be removed or merged from the original DFA without
        affecting the language it accepts to minimize it:

            * Unreachable states can not be reached from the initial
              state of the DFA, for any input string.
            * Nondistinguishable states are those that cannot be distinguished
              from one another for any input string.

        This algorithm does not achieve the optimal fully-reduced solution, but it
        works well enough for the particularities of the Python grammar. The
        algorithm repeatedly looks for two states that have the same set of
        arcs (same labels pointing to the same nodes) and unifies them, until
        things stop changing.
        """
        changes = True
        while changes:
            changes = False
            for i, state_i in enumerate(self.states):
                for j in range(i + 1, len(self.states)):
                    state_j = self.states[j]
                    if state_i == state_j:
                        del self.states[j]
                        for state in self.states:
                            state.unifystate(state_j, state_i)
                        changes = True
                        break

    def dump(self, writer=print):
        """Dump a graphical representation of the DFA"""
        for i, state in enumerate(self.states):
            writer("  State", i, state.is_final and "(final)" or "")
            for label, next in sorted(state.arcs.items()):
                writer("    %s -> %d" % (label, self.states.index(next)))


class DFAState(object):
    """A state of a DFA

    Attributes:
        rule_name (rule_name): The name of the DFA rule containing the represented state.
        nfa_set (Set[NFAState]): The set of NFA states used to create this state.
        final (bool): True if the state represents an accepting state of the DFA
            containing this state.
        arcs (Dict[label, DFAState]): A mapping representing transitions between
            the current DFA state and another DFA state via following a label.
    """

    def __init__(self, rule_name, nfa_set, final):
        assert isinstance(nfa_set, set)
        assert isinstance(next(iter(nfa_set)), NFAState)
        assert isinstance(final, NFAState)
        self.rule_name = rule_name
        self.nfa_set = nfa_set
        self.arcs = {}  # map from terminals/nonterminals to DFAState
        self.is_final = final in nfa_set

    def add_arc(self, target, label):
        """Add a new arc to the current state.

        Parameters:
            target (DFAState): The DFA state at the end of the arc.
            label (str): The label respresenting the token that must be consumed
                to perform this transition.
        """
        assert isinstance(label, str)
        assert label not in self.arcs
        assert isinstance(target, DFAState)
        self.arcs[label] = target

    def unifystate(self, old, new):
        """Replace all arcs from the current node to *old* with *new*.

        Parameters:
            old (DFAState): The  DFA state to remove from all existing arcs.
            new (DFAState): The DFA state to replace in all existing arcs.
        """
        for label, next_ in self.arcs.items():
            if next_ is old:
                self.arcs[label] = new

    def __eq__(self, other):
        # The nfa_set does not matter for  equality
        assert isinstance(other, DFAState)
        if self.is_final != other.is_final:
            return False
        # We cannot just return self.arcs == other.arcs because that
        # would invoke this method recursively if there are any cycles.
        if len(self.arcs) != len(other.arcs):
            return False
        for label, next_ in self.arcs.items():
            if next_ is not other.arcs.get(label):
                return False
        return True

    __hash__ = None  # For Py3 compatibility.

    def __repr__(self):
        return "<%s: %s is_final=%s>" % (
            self.__class__.__name__,
            self.rule_name,
            self.is_final,
        )
