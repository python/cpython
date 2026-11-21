import re
import unittest

from .util import DebuggerTests, setUpModule as setUpModule


VALUES = [
    0, 42, -7, 2**100, -(2**100), True, False, None,
    b'', b'a\0b', b'\t\n\r\\\'"', bytes(range(256)),
    '', 'a\0b', '\t\n\r\\\'"', '\xff\u2620\U0001d121', '\ud800',
    '\u2000\u2028',
    [], [1, [2, 3], 'hello'], (), (1,), ('foo', 'bar'),
    {}, {'foo': 'bar', 'douglas': 42}, {1: 'one', (2,): [3]},
    set(), {4, 5, 6}, frozenset({1}), frozendict({'answer': 42}),
]

OBJECTS_SETUP = """\
import _testcapi
class C: pass
class L(list): pass
instance = C(); instance.x = 42; instance.y = 'hello'
split = C(); split.y = 3
lst = L([1, 2]); lst.x = 42
rec_list = [1]; rec_list.append(rec_list)
removed = {1, 2}; removed.remove(1)
neg = _testcapi.HeapCTypeWithNegativeDict(); neg.a = 1
"""

OBJECTS = {
    "ValueError('hello')": "ValueError('hello',)",
    'instance': "<C(x=42, y='hello') at remote 0xADDR>",
    'split.__dict__': "{'y': 3}",
    'lst': '<L(x=42) at remote 0xADDR>',
    '[].append': '<built-in method append of list object at remote 0xADDR>',
    'rec_list': '[1, [...]]',
    'removed': '{2}',
    'neg': '<_testcapi.HeapCTypeWithNegativeDict(a=1) at remote 0xADDR>',
}


def normalize(output):
    return re.sub(r'0x[0-9a-f]{2,}', '0xADDR', output)


class PrettyPrintTests(DebuggerTests):
    def assertReprs(self, source, expected):
        reprs = self.get_reprs(source, len(expected))
        for got, want in zip(reprs, expected):
            with self.subTest(expected=want):
                self.assertEqual(normalize(got), want)

    def test_values(self):
        self.assertReprs(
            f'for v in {ascii(VALUES)}: id(v)', [repr(v) for v in VALUES]
        )

    def test_objects(self):
        self.assertReprs(
            f'{OBJECTS_SETUP}for v in [{", ".join(OBJECTS)}]: id(v)',
            list(OBJECTS.values()),
        )

    def test_dict_index_widths(self):
        source = (
            'for n in (8, 256, 65536):\n'
            '    for keys in (range(n), map(str, range(n))):\n'
            '        d = {k: i for i, k in enumerate(keys)}\n'
            '        for k in list(d)[3:]: del d[k]\n'
            '        id(d)'
        )
        expected = ["{0: 0, 1: 1, 2: 2}", "{'0': 0, '1': 1, '2': 2}"] * 3
        self.assertReprs(source, expected)

    def test_summary_types(self):
        commands = [
            'expression (PyListObject *)v',
            'expression *(PyListObject *)v',
            'expression (const PyObject *)v',
            'expression (const PyListObject *)v',
            'expression (_PyStackRef){.bits = (uintptr_t)v}',
        ]
        output = self.get_stack_trace('id([1, 2])', commands=commands)
        summaries = re.findall(r'^\(.+\) \$[0-9]+ = .*\[1, 2\]', output, re.M)
        self.assertEqual(len(summaries), len(commands), output)

    def test_damaged_objects(self):
        unreadable = '<unable to read Python object>'
        for value, damage, expected in (
            ('42', 'v = (PyObject *)0', 'NULL'),
            ('42', 'v = (PyObject *)1', '<unknown at remote 0x1>'),
            (
                '[1]',
                '((PyListObject *)v)->ob_item = (PyObject **)1',
                unreadable,
            ),
            (
                'list(range(1000))',
                '((PyVarObject *)v)->ob_size = 0x7fffffffffffffff',
                repr(list(range(1000)))[:1024] + '...(truncated)',
            ),
        ):
            with self.subTest(damage=damage):
                output = self.get_repr(
                    f'id({value})', commands=[f'expression {damage}']
                )
                self.assertEqual(normalize(output), expected)


if __name__ == '__main__':
    unittest.main()
