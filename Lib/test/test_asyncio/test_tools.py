import unittest

from asyncio import tools


# mock output of get_all_awaited_by function.
TEST_INPUTS_TREE = [
    [
        # test case containing a task called timer being awaited in two
        # different subtasks part of a TaskGroup (root1 and root2) which call
        # awaiter functions.
        (
            (
                1,
                [
                    (2, "Task-1", []),
                    (
                        3,
                        "timer",
                        [
                            [[("awaiter3", "/path/to/app.py", 130),
                              ("awaiter2", "/path/to/app.py", 120),
                              ("awaiter", "/path/to/app.py", 110)], 4],
                            [[("awaiterB3", "/path/to/app.py", 190),
                              ("awaiterB2", "/path/to/app.py", 180),
                              ("awaiterB", "/path/to/app.py", 170)], 5],
                            [[("awaiterB3", "/path/to/app.py", 190),
                              ("awaiterB2", "/path/to/app.py", 180),
                              ("awaiterB", "/path/to/app.py", 170)], 6],
                            [[("awaiter3", "/path/to/app.py", 130),
                              ("awaiter2", "/path/to/app.py", 120),
                              ("awaiter", "/path/to/app.py", 110)], 7],
                        ],
                    ),
                    (
                        8,
                        "root1",
                        [[["_aexit", "__aexit__", "main"], 2]],
                    ),
                    (
                        9,
                        "root2",
                        [[["_aexit", "__aexit__", "main"], 2]],
                    ),
                    (
                        4,
                        "child1_1",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                8,
                            ]
                        ],
                    ),
                    (
                        6,
                        "child2_1",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                8,
                            ]
                        ],
                    ),
                    (
                        7,
                        "child1_2",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                9,
                            ]
                        ],
                    ),
                    (
                        5,
                        "child2_2",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                9,
                            ]
                        ],
                    ),
                ],
            ),
            (0, []),
        ),
        (
            [
                [
                    "└── (T) Task-1",
                    "    └──  main",
                    "        └──  __aexit__",
                    "            └──  _aexit",
                    "                ├── (T) root1",
                    "                │   └──  bloch",
                    "                │       └──  blocho_caller",
                    "                │           └──  __aexit__",
                    "                │               └──  _aexit",
                    "                │                   ├── (T) child1_1",
                    "                │                   │   └──  awaiter /path/to/app.py:110",
                    "                │                   │       └──  awaiter2 /path/to/app.py:120",
                    "                │                   │           └──  awaiter3 /path/to/app.py:130",
                    "                │                   │               └── (T) timer",
                    "                │                   └── (T) child2_1",
                    "                │                       └──  awaiterB /path/to/app.py:170",
                    "                │                           └──  awaiterB2 /path/to/app.py:180",
                    "                │                               └──  awaiterB3 /path/to/app.py:190",
                    "                │                                   └── (T) timer",
                    "                └── (T) root2",
                    "                    └──  bloch",
                    "                        └──  blocho_caller",
                    "                            └──  __aexit__",
                    "                                └──  _aexit",
                    "                                    ├── (T) child1_2",
                    "                                    │   └──  awaiter /path/to/app.py:110",
                    "                                    │       └──  awaiter2 /path/to/app.py:120",
                    "                                    │           └──  awaiter3 /path/to/app.py:130",
                    "                                    │               └── (T) timer",
                    "                                    └── (T) child2_2",
                    "                                        └──  awaiterB /path/to/app.py:170",
                    "                                            └──  awaiterB2 /path/to/app.py:180",
                    "                                                └──  awaiterB3 /path/to/app.py:190",
                    "                                                    └── (T) timer",
                ]
            ]
        ),
    ],
    [
        # test case containing two roots
        (
            (
                9,
                [
                    (5, "Task-5", []),
                    (6, "Task-6", [[["main2"], 5]]),
                    (7, "Task-7", [[["main2"], 5]]),
                    (8, "Task-8", [[["main2"], 5]]),
                ],
            ),
            (
                10,
                [
                    (1, "Task-1", []),
                    (2, "Task-2", [[["main"], 1]]),
                    (3, "Task-3", [[["main"], 1]]),
                    (4, "Task-4", [[["main"], 1]]),
                ],
            ),
            (11, []),
            (0, []),
        ),
        (
            [
                [
                    "└── (T) Task-5",
                    "    └──  main2",
                    "        ├── (T) Task-6",
                    "        ├── (T) Task-7",
                    "        └── (T) Task-8",
                ],
                [
                    "└── (T) Task-1",
                    "    └──  main",
                    "        ├── (T) Task-2",
                    "        ├── (T) Task-3",
                    "        └── (T) Task-4",
                ],
            ]
        ),
    ],
    [
        # test case containing two roots, one of them without subtasks
        (
            [
                (1, [(2, "Task-5", [])]),
                (
                    3,
                    [
                        (4, "Task-1", []),
                        (5, "Task-2", [[["main"], 4]]),
                        (6, "Task-3", [[["main"], 4]]),
                        (7, "Task-4", [[["main"], 4]]),
                    ],
                ),
                (8, []),
                (0, []),
            ]
        ),
        (
            [
                ["└── (T) Task-5"],
                [
                    "└── (T) Task-1",
                    "    └──  main",
                    "        ├── (T) Task-2",
                    "        ├── (T) Task-3",
                    "        └── (T) Task-4",
                ],
            ]
        ),
    ],
]

TEST_INPUTS_CYCLES_TREE = [
    [
        # this test case contains a cycle: two tasks awaiting each other.
        (
            [
                (
                    1,
                    [
                        (2, "Task-1", []),
                        (
                            3,
                            "a",
                            [[["awaiter2"], 4], [["main"], 2]],
                        ),
                        (4, "b", [[["awaiter"], 3]]),
                    ],
                ),
                (0, []),
            ]
        ),
        ([[4, 3, 4]]),
    ],
    [
        # this test case contains two cycles
        (
            [
                (
                    1,
                    [
                        (2, "Task-1", []),
                        (
                            3,
                            "A",
                            [[["nested", "nested", "task_b"], 4]],
                        ),
                        (
                            4,
                            "B",
                            [
                                [["nested", "nested", "task_c"], 5],
                                [["nested", "nested", "task_a"], 3],
                            ],
                        ),
                        (5, "C", [[["nested", "nested"], 6]]),
                        (
                            6,
                            "Task-2",
                            [[["nested", "nested", "task_b"], 4]],
                        ),
                    ],
                ),
                (0, []),
            ]
        ),
        ([[4, 3, 4], [4, 6, 5, 4]]),
    ],
]

TEST_INPUTS_TABLE = [
    [
        # test case containing a task called timer being awaited in two
        # different subtasks part of a TaskGroup (root1 and root2) which call
        # awaiter functions.
        (
            (
                1,
                [
                    (2, "Task-1", []),
                    (
                        3,
                        "timer",
                        [
                            [["awaiter3", "awaiter2", "awaiter"], 4],
                            [["awaiter1_3", "awaiter1_2", "awaiter1"], 5],
                            [["awaiter1_3", "awaiter1_2", "awaiter1"], 6],
                            [["awaiter3", "awaiter2", "awaiter"], 7],
                        ],
                    ),
                    (
                        8,
                        "root1",
                        [[["_aexit", "__aexit__", "main"], 2]],
                    ),
                    (
                        9,
                        "root2",
                        [[["_aexit", "__aexit__", "main"], 2]],
                    ),
                    (
                        4,
                        "child1_1",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                8,
                            ]
                        ],
                    ),
                    (
                        6,
                        "child2_1",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                8,
                            ]
                        ],
                    ),
                    (
                        7,
                        "child1_2",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                9,
                            ]
                        ],
                    ),
                    (
                        5,
                        "child2_2",
                        [
                            [
                                ["_aexit", "__aexit__", "blocho_caller", "bloch"],
                                9,
                            ]
                        ],
                    ),
                ],
            ),
            (0, []),
        ),
        (
            [
                [1, "0x2", "Task-1", "", "", "0x0"],
                [
                    1,
                    "0x3",
                    "timer",
                    "awaiter3 -> awaiter2 -> awaiter",
                    "child1_1",
                    "0x4",
                ],
                [
                    1,
                    "0x3",
                    "timer",
                    "awaiter1_3 -> awaiter1_2 -> awaiter1",
                    "child2_2",
                    "0x5",
                ],
                [
                    1,
                    "0x3",
                    "timer",
                    "awaiter1_3 -> awaiter1_2 -> awaiter1",
                    "child2_1",
                    "0x6",
                ],
                [
                    1,
                    "0x3",
                    "timer",
                    "awaiter3 -> awaiter2 -> awaiter",
                    "child1_2",
                    "0x7",
                ],
                [
                    1,
                    "0x8",
                    "root1",
                    "_aexit -> __aexit__ -> main",
                    "Task-1",
                    "0x2",
                ],
                [
                    1,
                    "0x9",
                    "root2",
                    "_aexit -> __aexit__ -> main",
                    "Task-1",
                    "0x2",
                ],
                [
                    1,
                    "0x4",
                    "child1_1",
                    "_aexit -> __aexit__ -> blocho_caller -> bloch",
                    "root1",
                    "0x8",
                ],
                [
                    1,
                    "0x6",
                    "child2_1",
                    "_aexit -> __aexit__ -> blocho_caller -> bloch",
                    "root1",
                    "0x8",
                ],
                [
                    1,
                    "0x7",
                    "child1_2",
                    "_aexit -> __aexit__ -> blocho_caller -> bloch",
                    "root2",
                    "0x9",
                ],
                [
                    1,
                    "0x5",
                    "child2_2",
                    "_aexit -> __aexit__ -> blocho_caller -> bloch",
                    "root2",
                    "0x9",
                ],
            ]
        ),
    ],
    [
        # test case containing two roots
        (
            (
                9,
                [
                    (5, "Task-5", []),
                    (6, "Task-6", [[["main2"], 5]]),
                    (7, "Task-7", [[["main2"], 5]]),
                    (8, "Task-8", [[["main2"], 5]]),
                ],
            ),
            (
                10,
                [
                    (1, "Task-1", []),
                    (2, "Task-2", [[["main"], 1]]),
                    (3, "Task-3", [[["main"], 1]]),
                    (4, "Task-4", [[["main"], 1]]),
                ],
            ),
            (11, []),
            (0, []),
        ),
        (
            [
                [9, "0x5", "Task-5", "", "", "0x0"],
                [9, "0x6", "Task-6", "main2", "Task-5", "0x5"],
                [9, "0x7", "Task-7", "main2", "Task-5", "0x5"],
                [9, "0x8", "Task-8", "main2", "Task-5", "0x5"],
                [10, "0x1", "Task-1", "", "", "0x0"],
                [10, "0x2", "Task-2", "main", "Task-1", "0x1"],
                [10, "0x3", "Task-3", "main", "Task-1", "0x1"],
                [10, "0x4", "Task-4", "main", "Task-1", "0x1"],
            ]
        ),
    ],
    [
        # test case containing two roots, one of them without subtasks
        (
            [
                (1, [(2, "Task-5", [])]),
                (
                    3,
                    [
                        (4, "Task-1", []),
                        (5, "Task-2", [[["main"], 4]]),
                        (6, "Task-3", [[["main"], 4]]),
                        (7, "Task-4", [[["main"], 4]]),
                    ],
                ),
                (8, []),
                (0, []),
            ]
        ),
        (
            [
                [1, "0x2", "Task-5", "", "", "0x0"],
                [3, "0x4", "Task-1", "", "", "0x0"],
                [3, "0x5", "Task-2", "main", "Task-1", "0x4"],
                [3, "0x6", "Task-3", "main", "Task-1", "0x4"],
                [3, "0x7", "Task-4", "main", "Task-1", "0x4"],
            ]
        ),
    ],
    # CASES WITH CYCLES
    [
        # this test case contains a cycle: two tasks awaiting each other.
        (
            [
                (
                    1,
                    [
                        (2, "Task-1", []),
                        (
                            3,
                            "a",
                            [[["awaiter2"], 4], [["main"], 2]],
                        ),
                        (4, "b", [[["awaiter"], 3]]),
                    ],
                ),
                (0, []),
            ]
        ),
        (
            [
                [1, "0x2", "Task-1", "", "", "0x0"],
                [1, "0x3", "a", "awaiter2", "b", "0x4"],
                [1, "0x3", "a", "main", "Task-1", "0x2"],
                [1, "0x4", "b", "awaiter", "a", "0x3"],
            ]
        ),
    ],
    [
        # this test case contains two cycles
        (
            [
                (
                    1,
                    [
                        (2, "Task-1", []),
                        (
                            3,
                            "A",
                            [[["nested", "nested", "task_b"], 4]],
                        ),
                        (
                            4,
                            "B",
                            [
                                [["nested", "nested", "task_c"], 5],
                                [["nested", "nested", "task_a"], 3],
                            ],
                        ),
                        (5, "C", [[["nested", "nested"], 6]]),
                        (
                            6,
                            "Task-2",
                            [[["nested", "nested", "task_b"], 4]],
                        ),
                    ],
                ),
                (0, []),
            ]
        ),
        (
            [
                [1, "0x2", "Task-1", "", "", "0x0"],
                [
                    1,
                    "0x3",
                    "A",
                    "nested -> nested -> task_b",
                    "B",
                    "0x4",
                ],
                [
                    1,
                    "0x4",
                    "B",
                    "nested -> nested -> task_c",
                    "C",
                    "0x5",
                ],
                [
                    1,
                    "0x4",
                    "B",
                    "nested -> nested -> task_a",
                    "A",
                    "0x3",
                ],
                [
                    1,
                    "0x5",
                    "C",
                    "nested -> nested",
                    "Task-2",
                    "0x6",
                ],
                [
                    1,
                    "0x6",
                    "Task-2",
                    "nested -> nested -> task_b",
                    "B",
                    "0x4",
                ],
            ]
        ),
    ],
]


class TestAsyncioToolsTree(unittest.TestCase):
    def test_asyncio_utils(self):
        for input_, tree in TEST_INPUTS_TREE:
            with self.subTest(input_):
                self.assertEqual(tools.build_async_tree(input_), tree)

    def test_asyncio_utils_cycles(self):
        for input_, cycles in TEST_INPUTS_CYCLES_TREE:
            with self.subTest(input_):
                try:
                    tools.build_async_tree(input_)
                except tools.CycleFoundException as e:
                    self.assertEqual(e.cycles, cycles)


class TestAsyncioToolsTable(unittest.TestCase):
    def test_asyncio_utils(self):
        for input_, table in TEST_INPUTS_TABLE:
            with self.subTest(input_):
                self.assertEqual(tools.build_task_table(input_), table)


class TestAsyncioToolsBasic(unittest.TestCase):
    def test_empty_input_tree(self):
        """Test build_async_tree with empty input."""
        result = []
        expected_output = []
        self.assertEqual(tools.build_async_tree(result), expected_output)

    def test_empty_input_table(self):
        """Test build_task_table with empty input."""
        result = []
        expected_output = []
        self.assertEqual(tools.build_task_table(result), expected_output)

    def test_only_independent_tasks_tree(self):
        input_ = [(1, [(10, "taskA", []), (11, "taskB", [])])]
        expected = [["└── (T) taskA"], ["└── (T) taskB"]]
        result = tools.build_async_tree(input_)
        self.assertEqual(sorted(result), sorted(expected))

    def test_only_independent_tasks_table(self):
        input_ = [(1, [(10, "taskA", []), (11, "taskB", [])])]
        self.assertEqual(
            tools.build_task_table(input_),
            [[1, "0xa", "taskA", "", "", "0x0"], [1, "0xb", "taskB", "", "", "0x0"]],
        )

    def test_single_task_tree(self):
        """Test build_async_tree with a single task and no awaits."""
        result = [
            (
                1,
                [
                    (2, "Task-1", []),
                ],
            )
        ]
        expected_output = [
            [
                "└── (T) Task-1",
            ]
        ]
        self.assertEqual(tools.build_async_tree(result), expected_output)

    def test_single_task_table(self):
        """Test build_task_table with a single task and no awaits."""
        result = [
            (
                1,
                [
                    (2, "Task-1", []),
                ],
            )
        ]
        expected_output = [[1, "0x2", "Task-1", "", "", "0x0"]]
        self.assertEqual(tools.build_task_table(result), expected_output)

    def test_cycle_detection(self):
        """Test build_async_tree raises CycleFoundException for cyclic input."""
        result = [
            (
                1,
                [
                    (2, "Task-1", [[["main"], 3]]),
                    (3, "Task-2", [[["main"], 2]]),
                ],
            )
        ]
        with self.assertRaises(tools.CycleFoundException) as context:
            tools.build_async_tree(result)
        self.assertEqual(context.exception.cycles, [[3, 2, 3]])

    def test_complex_tree(self):
        """Test build_async_tree with a more complex tree structure."""
        result = [
            (
                1,
                [
                    (2, "Task-1", []),
                    (3, "Task-2", [[["main"], 2]]),
                    (4, "Task-3", [[["main"], 3]]),
                ],
            )
        ]
        expected_output = [
            [
                "└── (T) Task-1",
                "    └──  main",
                "        └── (T) Task-2",
                "            └──  main",
                "                └── (T) Task-3",
            ]
        ]
        self.assertEqual(tools.build_async_tree(result), expected_output)

    def test_complex_table(self):
        """Test build_task_table with a more complex tree structure."""
        result = [
            (
                1,
                [
                    (2, "Task-1", []),
                    (3, "Task-2", [[["main"], 2]]),
                    (4, "Task-3", [[["main"], 3]]),
                ],
            )
        ]
        expected_output = [
            [1, "0x2", "Task-1", "", "", "0x0"],
            [1, "0x3", "Task-2", "main", "Task-1", "0x2"],
            [1, "0x4", "Task-3", "main", "Task-2", "0x3"],
        ]
        self.assertEqual(tools.build_task_table(result), expected_output)

    def test_deep_coroutine_chain(self):
        input_ = [
            (
                1,
                [
                    (10, "leaf", [[["c1", "c2", "c3", "c4", "c5"], 11]]),
                    (11, "root", []),
                ],
            )
        ]
        expected = [
            [
                "└── (T) root",
                "    └──  c5",
                "        └──  c4",
                "            └──  c3",
                "                └──  c2",
                "                    └──  c1",
                "                        └── (T) leaf",
            ]
        ]
        result = tools.build_async_tree(input_)
        self.assertEqual(result, expected)

    def test_multiple_cycles_same_node(self):
        input_ = [
            (
                1,
                [
                    (1, "Task-A", [[["call1"], 2]]),
                    (2, "Task-B", [[["call2"], 3]]),
                    (3, "Task-C", [[["call3"], 1], [["call4"], 2]]),
                ],
            )
        ]
        with self.assertRaises(tools.CycleFoundException) as ctx:
            tools.build_async_tree(input_)
        cycles = ctx.exception.cycles
        self.assertTrue(any(set(c) == {1, 2, 3} for c in cycles))

    def test_table_output_format(self):
        input_ = [(1, [(1, "Task-A", [[["foo"], 2]]), (2, "Task-B", [])])]
        table = tools.build_task_table(input_)
        for row in table:
            self.assertEqual(len(row), 6)
            self.assertIsInstance(row[0], int)  # thread ID
            self.assertTrue(
                isinstance(row[1], str) and row[1].startswith("0x")
            )  # hex task ID
            self.assertIsInstance(row[2], str)  # task name
            self.assertIsInstance(row[3], str)  # coroutine chain
            self.assertIsInstance(row[4], str)  # awaiter name
            self.assertTrue(
                isinstance(row[5], str) and row[5].startswith("0x")
            )  # hex awaiter ID


class TestAsyncioToolsEdgeCases(unittest.TestCase):

    def test_task_awaits_self(self):
        """A task directly awaits itself - should raise a cycle."""
        input_ = [(1, [(1, "Self-Awaiter", [[["loopback"], 1]])])]
        with self.assertRaises(tools.CycleFoundException) as ctx:
            tools.build_async_tree(input_)
        self.assertIn([1, 1], ctx.exception.cycles)

    def test_task_with_missing_awaiter_id(self):
        """Awaiter ID not in task list - should not crash, just show 'Unknown'."""
        input_ = [(1, [(1, "Task-A", [[["coro"], 999]])])]  # 999 not defined
        table = tools.build_task_table(input_)
        self.assertEqual(len(table), 1)
        self.assertEqual(table[0][4], "Unknown")

    def test_duplicate_coroutine_frames(self):
        """Same coroutine frame repeated under a parent - should deduplicate."""
        input_ = [
            (
                1,
                [
                    (1, "Task-1", [[["frameA"], 2], [["frameA"], 3]]),
                    (2, "Task-2", []),
                    (3, "Task-3", []),
                ],
            )
        ]
        tree = tools.build_async_tree(input_)
        # Both children should be under the same coroutine node
        flat = "\n".join(tree[0])
        self.assertIn("frameA", flat)
        self.assertIn("Task-2", flat)
        self.assertIn("Task-1", flat)

        flat = "\n".join(tree[1])
        self.assertIn("frameA", flat)
        self.assertIn("Task-3", flat)
        self.assertIn("Task-1", flat)

    def test_task_with_no_name(self):
        """Task with no name in id2name - should still render with fallback."""
        input_ = [(1, [(1, "root", [[["f1"], 2]]), (2, None, [])])]
        # If name is None, fallback to string should not crash
        tree = tools.build_async_tree(input_)
        self.assertIn("(T) None", "\n".join(tree[0]))

    def test_tree_rendering_with_custom_emojis(self):
        """Pass custom emojis to the tree renderer."""
        input_ = [(1, [(1, "MainTask", [[["f1", "f2"], 2]]), (2, "SubTask", [])])]
        tree = tools.build_async_tree(input_, task_emoji="🧵", cor_emoji="🔁")
        flat = "\n".join(tree[0])
        self.assertIn("🧵 MainTask", flat)
        self.assertIn("🔁 f1", flat)
        self.assertIn("🔁 f2", flat)
        self.assertIn("🧵 SubTask", flat)
