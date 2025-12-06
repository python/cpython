"""Tests for async stack reconstruction in the sampling profiler.

Each test covers a distinct algorithm path or edge case:
1. Graph building: _build_task_graph()
2. Leaf identification: _find_leaf_tasks()
3. Stack traversal: _build_linear_stacks() with BFS
"""

import unittest

try:
    import _remote_debugging  # noqa: F401
    from profiling.sampling.pstats_collector import PstatsCollector
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from .mocks import MockFrameInfo, MockCoroInfo, MockTaskInfo, MockAwaitedInfo


class TestAsyncStackReconstruction(unittest.TestCase):
    """Test async task tree linear stack reconstruction algorithm."""

    def test_empty_input(self):
        """Test _build_task_graph with empty awaited_info_list."""
        collector = PstatsCollector(sample_interval_usec=1000)
        stacks = list(collector._iter_async_frames([]))
        self.assertEqual(len(stacks), 0)

    def test_single_root_task(self):
        """Test _find_leaf_tasks: root task with no parents is its own leaf."""
        collector = PstatsCollector(sample_interval_usec=1000)

        root = MockTaskInfo(
            task_id=123,
            task_name="Task-1",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="Task-1",
                    call_stack=[MockFrameInfo("main.py", 10, "main")]
                )
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=100, awaited_by=[root])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Single root is both leaf and root
        self.assertEqual(len(stacks), 1)
        frames, thread_id, leaf_id = stacks[0]
        self.assertEqual(leaf_id, 123)
        self.assertEqual(thread_id, 100)

    def test_parent_child_chain(self):
        """Test _build_linear_stacks: BFS follows parent links from leaf to root.

        Task graph:

            Parent (id=1)
                |
            Child (id=2)
        """
        collector = PstatsCollector(sample_interval_usec=1000)

        child = MockTaskInfo(
            task_id=2,
            task_name="Child",
            coroutine_stack=[
                MockCoroInfo(task_name="Child", call_stack=[MockFrameInfo("c.py", 5, "child_fn")])
            ],
            awaited_by=[
                MockCoroInfo(task_name=1, call_stack=[MockFrameInfo("p.py", 10, "parent_await")])
            ]
        )

        parent = MockTaskInfo(
            task_id=1,
            task_name="Parent",
            coroutine_stack=[
                MockCoroInfo(task_name="Parent", call_stack=[MockFrameInfo("p.py", 15, "parent_fn")])
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=200, awaited_by=[child, parent])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Leaf is child, traverses to parent
        self.assertEqual(len(stacks), 1)
        frames, thread_id, leaf_id = stacks[0]
        self.assertEqual(leaf_id, 2)

        # Verify both child and parent frames present
        func_names = [f.funcname for f in frames]
        self.assertIn("child_fn", func_names)
        self.assertIn("parent_fn", func_names)

    def test_multiple_leaf_tasks(self):
        """Test _find_leaf_tasks: identifies multiple leaves correctly.

        Task graph (fan-out from root):

                Root (id=1)
                /        \
          Leaf1 (id=10)  Leaf2 (id=20)

        Expected: 2 stacks (one for each leaf).
        """
        collector = PstatsCollector(sample_interval_usec=1000)
        leaf1 = MockTaskInfo(
            task_id=10,
            task_name="Leaf1",
            coroutine_stack=[MockCoroInfo(task_name="Leaf1", call_stack=[MockFrameInfo("l1.py", 1, "f1")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[MockFrameInfo("r.py", 5, "root")])]
        )

        leaf2 = MockTaskInfo(
            task_id=20,
            task_name="Leaf2",
            coroutine_stack=[MockCoroInfo(task_name="Leaf2", call_stack=[MockFrameInfo("l2.py", 2, "f2")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[MockFrameInfo("r.py", 5, "root")])]
        )

        root = MockTaskInfo(
            task_id=1,
            task_name="Root",
            coroutine_stack=[MockCoroInfo(task_name="Root", call_stack=[MockFrameInfo("r.py", 10, "main")])],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=300, awaited_by=[leaf1, leaf2, root])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Two leaves = two stacks
        self.assertEqual(len(stacks), 2)
        leaf_ids = {leaf_id for _, _, leaf_id in stacks}
        self.assertEqual(leaf_ids, {10, 20})

    def test_cycle_detection(self):
        """Test _build_linear_stacks: cycle detection prevents infinite loops.

        Task graph (cyclic dependency):

            A (id=1) <---> B (id=2)

        Neither task is a leaf (both have parents), so no stacks are produced.
        """
        collector = PstatsCollector(sample_interval_usec=1000)
        task_a = MockTaskInfo(
            task_id=1,
            task_name="A",
            coroutine_stack=[MockCoroInfo(task_name="A", call_stack=[MockFrameInfo("a.py", 1, "a")])],
            awaited_by=[MockCoroInfo(task_name=2, call_stack=[MockFrameInfo("b.py", 5, "b")])]
        )

        task_b = MockTaskInfo(
            task_id=2,
            task_name="B",
            coroutine_stack=[MockCoroInfo(task_name="B", call_stack=[MockFrameInfo("b.py", 10, "b")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[MockFrameInfo("a.py", 15, "a")])]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=400, awaited_by=[task_a, task_b])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # No leaves (both have parents), should return empty
        self.assertEqual(len(stacks), 0)

    def test_orphaned_parent_reference(self):
        """Test _build_linear_stacks: handles parent ID not in task_map."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Task references non-existent parent
        orphan = MockTaskInfo(
            task_id=5,
            task_name="Orphan",
            coroutine_stack=[MockCoroInfo(task_name="Orphan", call_stack=[MockFrameInfo("o.py", 1, "orphan")])],
            awaited_by=[MockCoroInfo(task_name=999, call_stack=[])]  # 999 doesn't exist
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=500, awaited_by=[orphan])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Stops at missing parent, yields what it has
        self.assertEqual(len(stacks), 1)
        frames, _, leaf_id = stacks[0]
        self.assertEqual(leaf_id, 5)

    def test_multiple_coroutines_per_task(self):
        """Test _build_linear_stacks: collects frames from all coroutines in task."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Task with multiple coroutines (e.g., nested async generators)
        task = MockTaskInfo(
            task_id=7,
            task_name="Multi",
            coroutine_stack=[
                MockCoroInfo(task_name="Multi", call_stack=[MockFrameInfo("g.py", 5, "gen1")]),
                MockCoroInfo(task_name="Multi", call_stack=[MockFrameInfo("g.py", 10, "gen2")]),
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=600, awaited_by=[task])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        self.assertEqual(len(stacks), 1)
        frames, _, _ = stacks[0]

        # Both coroutine frames should be present
        func_names = [f.funcname for f in frames]
        self.assertIn("gen1", func_names)
        self.assertIn("gen2", func_names)

    def test_multiple_threads(self):
        """Test _build_task_graph: handles multiple AwaitedInfo (different threads)."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Two threads with separate task trees
        thread1_task = MockTaskInfo(
            task_id=100,
            task_name="T1",
            coroutine_stack=[MockCoroInfo(task_name="T1", call_stack=[MockFrameInfo("t1.py", 1, "t1")])],
            awaited_by=[]
        )

        thread2_task = MockTaskInfo(
            task_id=200,
            task_name="T2",
            coroutine_stack=[MockCoroInfo(task_name="T2", call_stack=[MockFrameInfo("t2.py", 1, "t2")])],
            awaited_by=[]
        )

        awaited_info_list = [
            MockAwaitedInfo(thread_id=1, awaited_by=[thread1_task]),
            MockAwaitedInfo(thread_id=2, awaited_by=[thread2_task]),
        ]

        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Two threads = two stacks
        self.assertEqual(len(stacks), 2)

        # Verify thread IDs preserved
        thread_ids = {thread_id for _, thread_id, _ in stacks}
        self.assertEqual(thread_ids, {1, 2})

    def test_collect_public_interface(self):
        """Test collect() method correctly routes to async frame processing."""
        collector = PstatsCollector(sample_interval_usec=1000)

        child = MockTaskInfo(
            task_id=50,
            task_name="Child",
            coroutine_stack=[MockCoroInfo(task_name="Child", call_stack=[MockFrameInfo("c.py", 1, "child")])],
            awaited_by=[MockCoroInfo(task_name=51, call_stack=[])]
        )

        parent = MockTaskInfo(
            task_id=51,
            task_name="Parent",
            coroutine_stack=[MockCoroInfo(task_name="Parent", call_stack=[MockFrameInfo("p.py", 1, "parent")])],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=999, awaited_by=[child, parent])]

        # Public interface: collect()
        collector.collect(awaited_info_list)

        # Verify stats collected
        self.assertGreater(len(collector.result), 0)
        func_names = [loc[2] for loc in collector.result.keys()]
        self.assertIn("child", func_names)
        self.assertIn("parent", func_names)

    def test_diamond_pattern_multiple_parents(self):
        """Test _build_linear_stacks: task with 2+ parents picks one deterministically.

        CRITICAL: Tests that when a task has multiple parents, we pick one parent
        deterministically (sorted, first one) and annotate the task name with parent count.
        """
        collector = PstatsCollector(sample_interval_usec=1000)

        # Diamond pattern: Root spawns A and B, both await Child
        #
        #         Root (id=1)
        #         /        \
        #     A (id=2)    B (id=3)
        #         \        /
        #        Child (id=4)
        #

        child = MockTaskInfo(
            task_id=4,
            task_name="Child",
            coroutine_stack=[MockCoroInfo(task_name="Child", call_stack=[MockFrameInfo("c.py", 1, "child_work")])],
            awaited_by=[
                MockCoroInfo(task_name=2, call_stack=[MockFrameInfo("a.py", 5, "a_await")]),  # Parent A
                MockCoroInfo(task_name=3, call_stack=[MockFrameInfo("b.py", 5, "b_await")]),  # Parent B
            ]
        )

        parent_a = MockTaskInfo(
            task_id=2,
            task_name="A",
            coroutine_stack=[MockCoroInfo(task_name="A", call_stack=[MockFrameInfo("a.py", 10, "a_work")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[MockFrameInfo("root.py", 5, "root_spawn")])]
        )

        parent_b = MockTaskInfo(
            task_id=3,
            task_name="B",
            coroutine_stack=[MockCoroInfo(task_name="B", call_stack=[MockFrameInfo("b.py", 10, "b_work")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[MockFrameInfo("root.py", 5, "root_spawn")])]
        )

        root = MockTaskInfo(
            task_id=1,
            task_name="Root",
            coroutine_stack=[MockCoroInfo(task_name="Root", call_stack=[MockFrameInfo("root.py", 20, "main")])],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=777, awaited_by=[child, parent_a, parent_b, root])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Should get 1 stack: Child->A->Root (picks parent with lowest ID: 2)
        self.assertEqual(len(stacks), 1, "Diamond should create only 1 path, picking first sorted parent")

        # Verify the single stack
        frames, thread_id, leaf_id = stacks[0]
        self.assertEqual(leaf_id, 4)
        self.assertEqual(thread_id, 777)

        func_names = [f.funcname for f in frames]
        # Stack should contain child, parent A (id=2, first when sorted), and root
        self.assertIn("child_work", func_names)
        self.assertIn("a_work", func_names, "Should use parent A (id=2, first when sorted)")
        self.assertNotIn("b_work", func_names, "Should not include parent B")
        self.assertIn("main", func_names)

        # Verify Child task is annotated with parent count
        self.assertIn("Child (2 parents)", func_names, "Child task should be annotated with parent count")

    def test_empty_coroutine_stack(self):
        """Test _build_linear_stacks: handles empty coroutine_stack (line 109 condition false)."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Task with no coroutine_stack
        task = MockTaskInfo(
            task_id=99,
            task_name="EmptyStack",
            coroutine_stack=[],  # Empty!
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=111, awaited_by=[task])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        self.assertEqual(len(stacks), 1)
        frames, _, _ = stacks[0]

        # Should only have task marker, no function frames
        func_names = [f.funcname for f in frames]
        self.assertEqual(len(func_names), 1, "Should only have task marker")
        self.assertIn("EmptyStack", func_names)

    def test_orphaned_parent_with_no_frames_collected(self):
        """Test _build_linear_stacks: orphaned parent at start with empty frames (line 94-96)."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Leaf that doesn't exist in task_map (should not happen normally, but test robustness)
        # We'll create a scenario where the leaf_id is present but empty

        # Task references non-existent parent, and has no coroutine_stack
        orphan = MockTaskInfo(
            task_id=88,
            task_name="Orphan",
            coroutine_stack=[],  # No frames
            awaited_by=[MockCoroInfo(task_name=999, call_stack=[])]  # Parent doesn't exist
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=222, awaited_by=[orphan])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # Should yield because we have the task marker even with no function frames
        self.assertEqual(len(stacks), 1)
        frames, _, leaf_id = stacks[0]
        self.assertEqual(leaf_id, 88)
        # Has task marker but no function frames
        self.assertGreater(len(frames), 0, "Should have at least task marker")

    def test_frame_ordering(self):
        """Test _build_linear_stacks: frames are collected in correct order (leaf->root).

        Task graph (3-level chain):

            Root (id=1)   <- root_bottom, root_top
                |
            Middle (id=2) <- mid_bottom, mid_top
                |
            Leaf (id=3)   <- leaf_bottom, leaf_top

        Expected frame order: leaf_bottom, leaf_top, mid_bottom, mid_top, root_bottom, root_top
        (stack is built bottom-up: leaf frames first, then parent frames).
        """
        collector = PstatsCollector(sample_interval_usec=1000)
        leaf = MockTaskInfo(
            task_id=3,
            task_name="Leaf",
            coroutine_stack=[
                MockCoroInfo(task_name="Leaf", call_stack=[
                    MockFrameInfo("leaf.py", 1, "leaf_bottom"),
                    MockFrameInfo("leaf.py", 2, "leaf_top"),
                ])
            ],
            awaited_by=[MockCoroInfo(task_name=2, call_stack=[])]
        )

        middle = MockTaskInfo(
            task_id=2,
            task_name="Middle",
            coroutine_stack=[
                MockCoroInfo(task_name="Middle", call_stack=[
                    MockFrameInfo("mid.py", 1, "mid_bottom"),
                    MockFrameInfo("mid.py", 2, "mid_top"),
                ])
            ],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[])]
        )

        root = MockTaskInfo(
            task_id=1,
            task_name="Root",
            coroutine_stack=[
                MockCoroInfo(task_name="Root", call_stack=[
                    MockFrameInfo("root.py", 1, "root_bottom"),
                    MockFrameInfo("root.py", 2, "root_top"),
                ])
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=333, awaited_by=[leaf, middle, root])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        self.assertEqual(len(stacks), 1)
        frames, _, _ = stacks[0]

        func_names = [f.funcname for f in frames]

        # Order should be: leaf frames, leaf marker, middle frames, middle marker, root frames, root marker
        leaf_bottom_idx = func_names.index("leaf_bottom")
        leaf_top_idx = func_names.index("leaf_top")
        mid_bottom_idx = func_names.index("mid_bottom")
        root_bottom_idx = func_names.index("root_bottom")

        # Verify leaf comes before middle comes before root
        self.assertLess(leaf_bottom_idx, leaf_top_idx, "Leaf frames in order")
        self.assertLess(leaf_top_idx, mid_bottom_idx, "Leaf before middle")
        self.assertLess(mid_bottom_idx, root_bottom_idx, "Middle before root")

    def test_complex_multi_parent_convergence(self):
        """Test _build_linear_stacks: multiple leaves with same parents pick deterministically.

        Tests that when multiple leaves have multiple parents, each leaf picks the same
        parent (sorted, first one) and all leaves are annotated with parent count.

        Task graph structure (both leaves awaited by both A and B)::

                    Root (id=1)
                    /        \\
               A (id=2)    B (id=3)
                  |  \\    /  |
                  |   \\  /   |
                  |    \\/    |
                  |    /\\    |
                  |   /  \\   |
            LeafX (id=4)  LeafY (id=5)

        Expected behavior: Both leaves pick parent A (lowest id=2) for their stack path.
        Result: 2 stacks, both going through A -> Root (B is skipped).
        """
        collector = PstatsCollector(sample_interval_usec=1000)

        leaf_x = MockTaskInfo(
            task_id=4,
            task_name="LeafX",
            coroutine_stack=[MockCoroInfo(task_name="LeafX", call_stack=[MockFrameInfo("x.py", 1, "x")])],
            awaited_by=[
                MockCoroInfo(task_name=2, call_stack=[]),
                MockCoroInfo(task_name=3, call_stack=[]),
            ]
        )

        leaf_y = MockTaskInfo(
            task_id=5,
            task_name="LeafY",
            coroutine_stack=[MockCoroInfo(task_name="LeafY", call_stack=[MockFrameInfo("y.py", 1, "y")])],
            awaited_by=[
                MockCoroInfo(task_name=2, call_stack=[]),
                MockCoroInfo(task_name=3, call_stack=[]),
            ]
        )

        parent_a = MockTaskInfo(
            task_id=2,
            task_name="A",
            coroutine_stack=[MockCoroInfo(task_name="A", call_stack=[MockFrameInfo("a.py", 1, "a")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[])]
        )

        parent_b = MockTaskInfo(
            task_id=3,
            task_name="B",
            coroutine_stack=[MockCoroInfo(task_name="B", call_stack=[MockFrameInfo("b.py", 1, "b")])],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[])]
        )

        root = MockTaskInfo(
            task_id=1,
            task_name="Root",
            coroutine_stack=[MockCoroInfo(task_name="Root", call_stack=[MockFrameInfo("r.py", 1, "root")])],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=444, awaited_by=[leaf_x, leaf_y, parent_a, parent_b, root])]
        stacks = list(collector._iter_async_frames(awaited_info_list))

        # 2 leaves, each picks same parent (A, id=2) = 2 paths
        self.assertEqual(len(stacks), 2, "Should create 2 paths: X->A->Root, Y->A->Root")

        # Verify both leaves pick parent A (id=2, first when sorted)
        leaf_ids_seen = set()
        for frames, _, leaf_id in stacks:
            leaf_ids_seen.add(leaf_id)
            func_names = [f.funcname for f in frames]

            # Both stacks should go through parent A only
            self.assertIn("a", func_names, "Should use parent A (id=2, first when sorted)")
            self.assertNotIn("b", func_names, "Should not include parent B")
            self.assertIn("root", func_names, "Should reach root")

            # Check for parent count annotation on the leaf
            if leaf_id == 4:
                self.assertIn("x", func_names)
                self.assertIn("LeafX (2 parents)", func_names, "LeafX should be annotated with parent count")
            elif leaf_id == 5:
                self.assertIn("y", func_names)
                self.assertIn("LeafY (2 parents)", func_names, "LeafY should be annotated with parent count")

        # Both leaves should be represented
        self.assertEqual(leaf_ids_seen, {4, 5}, "Both LeafX and LeafY should have paths")


class TestFlamegraphCollectorAsync(unittest.TestCase):
    """Test FlamegraphCollector with async frames."""

    def test_flamegraph_with_async_frames(self):
        """Test FlamegraphCollector correctly processes async task frames."""
        from profiling.sampling.stack_collector import FlamegraphCollector

        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Build async task tree: Root -> Child
        child = MockTaskInfo(
            task_id=2,
            task_name="ChildTask",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="ChildTask",
                    call_stack=[MockFrameInfo("child.py", 10, "child_work")]
                )
            ],
            awaited_by=[MockCoroInfo(task_name=1, call_stack=[])]
        )

        root = MockTaskInfo(
            task_id=1,
            task_name="RootTask",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="RootTask",
                    call_stack=[MockFrameInfo("root.py", 20, "root_work")]
                )
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=100, awaited_by=[child, root])]

        # Collect async frames
        collector.collect(awaited_info_list)

        # Verify samples were collected
        self.assertGreater(collector._total_samples, 0)

        # Verify the flamegraph tree structure contains our functions
        root_node = collector._root
        self.assertGreater(root_node["samples"], 0)

        # Check that thread ID was tracked
        self.assertIn(100, collector._all_threads)

    def test_flamegraph_with_task_markers(self):
        """Test FlamegraphCollector includes <task> boundary markers."""
        from profiling.sampling.stack_collector import FlamegraphCollector

        collector = FlamegraphCollector(sample_interval_usec=1000)

        task = MockTaskInfo(
            task_id=42,
            task_name="MyTask",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="MyTask",
                    call_stack=[MockFrameInfo("work.py", 5, "do_work")]
                )
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=200, awaited_by=[task])]
        collector.collect(awaited_info_list)

        # Find <task> marker in the tree
        def find_task_marker(node, depth=0):
            for func, child in node.get("children", {}).items():
                if func[0] == "<task>":
                    return func
                result = find_task_marker(child, depth + 1)
                if result:
                    return result
            return None

        task_marker = find_task_marker(collector._root)
        self.assertIsNotNone(task_marker, "Should have <task> marker in tree")
        self.assertEqual(task_marker[0], "<task>")
        self.assertIn("MyTask", task_marker[2])

    def test_flamegraph_multiple_async_samples(self):
        """Test FlamegraphCollector aggregates multiple async samples correctly."""
        from profiling.sampling.stack_collector import FlamegraphCollector

        collector = FlamegraphCollector(sample_interval_usec=1000)

        task = MockTaskInfo(
            task_id=1,
            task_name="Task",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="Task",
                    call_stack=[MockFrameInfo("work.py", 10, "work")]
                )
            ],
            awaited_by=[]
        )

        awaited_info_list = [MockAwaitedInfo(thread_id=300, awaited_by=[task])]

        # Collect multiple samples
        for _ in range(5):
            collector.collect(awaited_info_list)

        # Verify sample count
        self.assertEqual(collector._sample_count, 5)
        self.assertEqual(collector._total_samples, 5)


class TestAsyncAwareParameterFlow(unittest.TestCase):
    """Integration tests for async_aware parameter flow from CLI to unwinder."""

    def test_sample_function_accepts_async_aware(self):
        """Test that sample() function accepts async_aware parameter."""
        from profiling.sampling.sample import sample
        import inspect

        sig = inspect.signature(sample)
        self.assertIn("async_aware", sig.parameters)

    def test_sample_live_function_accepts_async_aware(self):
        """Test that sample_live() function accepts async_aware parameter."""
        from profiling.sampling.sample import sample_live
        import inspect

        sig = inspect.signature(sample_live)
        self.assertIn("async_aware", sig.parameters)

    def test_sample_profiler_sample_accepts_async_aware(self):
        """Test that SampleProfiler.sample() accepts async_aware parameter."""
        from profiling.sampling.sample import SampleProfiler
        import inspect

        sig = inspect.signature(SampleProfiler.sample)
        self.assertIn("async_aware", sig.parameters)

    def test_async_aware_all_sees_sleeping_and_running_tasks(self):
        """Test async_aware='all' captures both sleeping and CPU-running tasks."""
        # Sleeping task (awaiting)
        sleeping_task = MockTaskInfo(
            task_id=1,
            task_name="SleepingTask",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="SleepingTask",
                    call_stack=[MockFrameInfo("sleeper.py", 10, "sleep_work")]
                )
            ],
            awaited_by=[]
        )

        # CPU-running task (active)
        running_task = MockTaskInfo(
            task_id=2,
            task_name="RunningTask",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="RunningTask",
                    call_stack=[MockFrameInfo("runner.py", 20, "cpu_work")]
                )
            ],
            awaited_by=[]
        )

        # Both tasks returned by get_all_awaited_by
        awaited_info_list = [MockAwaitedInfo(thread_id=100, awaited_by=[sleeping_task, running_task])]

        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect(awaited_info_list)
        collector.create_stats()

        # Both tasks should be visible
        sleeping_key = ("sleeper.py", 10, "sleep_work")
        running_key = ("runner.py", 20, "cpu_work")

        self.assertIn(sleeping_key, collector.stats)
        self.assertIn(running_key, collector.stats)

        # Task markers should also be present
        task_keys = [k for k in collector.stats if k[0] == "<task>"]
        self.assertGreater(len(task_keys), 0, "Should have <task> markers in stats")

        # Verify task names are in the markers
        task_names = [k[2] for k in task_keys]
        self.assertTrue(
            any("SleepingTask" in name for name in task_names),
            "SleepingTask should be in task markers"
        )
        self.assertTrue(
            any("RunningTask" in name for name in task_names),
            "RunningTask should be in task markers"
        )

    def test_async_aware_running_sees_only_running_task(self):
        """Test async_aware='running' only shows the currently running task stack."""
        # Only the running task's stack is returned by get_async_stack_trace
        running_task = MockTaskInfo(
            task_id=2,
            task_name="RunningTask",
            coroutine_stack=[
                MockCoroInfo(
                    task_name="RunningTask",
                    call_stack=[MockFrameInfo("runner.py", 20, "cpu_work")]
                )
            ],
            awaited_by=[]
        )

        # get_async_stack_trace only returns the running task
        awaited_info_list = [MockAwaitedInfo(thread_id=100, awaited_by=[running_task])]

        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect(awaited_info_list)
        collector.create_stats()

        # Only running task should be visible
        running_key = ("runner.py", 20, "cpu_work")
        self.assertIn(running_key, collector.stats)

        # Verify we don't see the sleeping task (it wasn't in the input)
        sleeping_key = ("sleeper.py", 10, "sleep_work")
        self.assertNotIn(sleeping_key, collector.stats)

        # Task marker for running task should be present
        task_keys = [k for k in collector.stats if k[0] == "<task>"]
        self.assertGreater(len(task_keys), 0, "Should have <task> markers in stats")

        task_names = [k[2] for k in task_keys]
        self.assertTrue(
            any("RunningTask" in name for name in task_names),
            "RunningTask should be in task markers"
        )


if __name__ == "__main__":
    unittest.main()
