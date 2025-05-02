"""Tests for the asyncio tools script."""

import unittest

from Lib.asyncio import tools


# mock output of get_all_awaited_by function.
TEST_INPUTS = [
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
        ([]),
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
                    "                │                   │   └──  awaiter",
                    "                │                   │       └──  awaiter2",
                    "                │                   │           └──  awaiter3",
                    "                │                   │               └── (T) timer",
                    "                │                   └── (T) child2_1",
                    "                │                       └──  awaiter1",
                    "                │                           └──  awaiter1_2",
                    "                │                               └──  awaiter1_3",
                    "                │                                   └── (T) timer",
                    "                └── (T) root2",
                    "                    └──  bloch",
                    "                        └──  blocho_caller",
                    "                            └──  __aexit__",
                    "                                └──  _aexit",
                    "                                    ├── (T) child1_2",
                    "                                    │   └──  awaiter",
                    "                                    │       └──  awaiter2",
                    "                                    │           └──  awaiter3",
                    "                                    │               └── (T) timer",
                    "                                    └── (T) child2_2",
                    "                                        └──  awaiter1",
                    "                                            └──  awaiter1_2",
                    "                                                └──  awaiter1_3",
                    "                                                    └── (T) timer",
                ]
            ]
        ),
        (
            [
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
        ([]),
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
        (
            [
                [9, "0x6", "Task-6", "main2", "Task-5", "0x5"],
                [9, "0x7", "Task-7", "main2", "Task-5", "0x5"],
                [9, "0x8", "Task-8", "main2", "Task-5", "0x5"],
                [10, "0x2", "Task-2", "main", "Task-1", "0x1"],
                [10, "0x3", "Task-3", "main", "Task-1", "0x1"],
                [10, "0x4", "Task-4", "main", "Task-1", "0x1"],
            ]
        ),
    ],
    # Tests cycle detection.
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
        ([]),
        (
            [
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
        ([[4, 3, 4], [4, 6, 5, 4]]),
        ([]),
        (
            [
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


class TestAsyncioTools(unittest.TestCase):

    def test_asyncio_utils(self):
        for input_, cycles, tree, table in TEST_INPUTS:
            if cycles:
                try:
                    tools.print_async_tree(input_)
                except tools.CycleFoundException as e:
                    self.assertEqual(e.cycles, cycles)
            else:
                self.assertEqual(tools.print_async_tree(input_), tree)
            self.assertEqual(tools.build_task_table(input_), table)
