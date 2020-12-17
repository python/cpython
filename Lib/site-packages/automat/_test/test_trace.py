from unittest import TestCase
from .._methodical import MethodicalMachine

class SampleObject(object):
    mm = MethodicalMachine()

    @mm.state(initial=True)
    def begin(self):
        "initial state"
    @mm.state()
    def middle(self):
        "middle state"
    @mm.state()
    def end(self):
        "end state"

    @mm.input()
    def go1(self):
        "sample input"
    @mm.input()
    def go2(self):
        "sample input"
    @mm.input()
    def back(self):
        "sample input"

    @mm.output()
    def out(self):
        "sample output"

    setTrace = mm._setTrace

    begin.upon(go1, middle, [out])
    middle.upon(go2, end, [out])
    end.upon(back, middle, [])
    middle.upon(back, begin, [])

class TraceTests(TestCase):
    def test_only_inputs(self):
        traces = []
        def tracer(old_state, input, new_state):
            traces.append((old_state, input, new_state))
            return None # "I only care about inputs, not outputs"
        s = SampleObject()
        s.setTrace(tracer)

        s.go1()
        self.assertEqual(traces, [("begin", "go1", "middle"),
                                  ])

        s.go2()
        self.assertEqual(traces, [("begin", "go1", "middle"),
                                  ("middle", "go2", "end"),
                                  ])
        s.setTrace(None)
        s.back()
        self.assertEqual(traces, [("begin", "go1", "middle"),
                                  ("middle", "go2", "end"),
                                  ])
        s.go2()
        self.assertEqual(traces, [("begin", "go1", "middle"),
                                  ("middle", "go2", "end"),
                                  ])

    def test_inputs_and_outputs(self):
        traces = []
        def tracer(old_state, input, new_state):
            traces.append((old_state, input, new_state, None))
            def trace_outputs(output):
                traces.append((old_state, input, new_state, output))
            return trace_outputs # "I care about outputs too"
        s = SampleObject()
        s.setTrace(tracer)

        s.go1()
        self.assertEqual(traces, [("begin", "go1", "middle", None),
                                  ("begin", "go1", "middle", "out"),
                                  ])

        s.go2()
        self.assertEqual(traces, [("begin", "go1", "middle", None),
                                  ("begin", "go1", "middle", "out"),
                                  ("middle", "go2", "end", None),
                                  ("middle", "go2", "end", "out"),
                                  ])
        s.setTrace(None)
        s.back()
        self.assertEqual(traces, [("begin", "go1", "middle", None),
                                  ("begin", "go1", "middle", "out"),
                                  ("middle", "go2", "end", None),
                                  ("middle", "go2", "end", "out"),
                                  ])
        s.go2()
        self.assertEqual(traces, [("begin", "go1", "middle", None),
                                  ("begin", "go1", "middle", "out"),
                                  ("middle", "go2", "end", None),
                                  ("middle", "go2", "end", "out"),
                                  ])
