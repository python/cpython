
import ast
import types

from test.support.bytecode_helper import AssemblerTestCase


# Tests for the code-object creation stage of the compiler.

class IsolatedAssembleTests(AssemblerTestCase):

    def complete_metadata(self, metadata, filename="myfile.py"):
        if metadata is None:
            metadata = {}
        for key in ['name', 'qualname']:
            metadata.setdefault(key, key)
        for key in ['consts']:
            metadata.setdefault(key, [])
        for key in ['names', 'varnames', 'cellvars', 'freevars', 'fasthidden']:
            metadata.setdefault(key, {})
        for key in ['argcount', 'posonlyargcount', 'kwonlyargcount']:
            metadata.setdefault(key, 0)
        metadata.setdefault('firstlineno', 1)
        metadata.setdefault('filename', filename)
        return metadata

    def assemble_test(self, insts, metadata, expected):
        metadata = self.complete_metadata(metadata)
        insts = self.complete_insts_info(insts)

        co = self.get_code_object(metadata['filename'], insts, metadata)
        self.assertIsInstance(co, types.CodeType)

        expected_metadata = {}
        for key, value in metadata.items():
            if key == "fasthidden":
                # not exposed on code object
                continue
            if isinstance(value, list):
                expected_metadata[key] = tuple(value)
            elif isinstance(value, dict):
                expected_metadata[key] = tuple(value.keys())
            else:
                expected_metadata[key] = value

        for key, value in expected_metadata.items():
            self.assertEqual(getattr(co, "co_" + key), value)

        f = types.FunctionType(co, {})
        for args, res in expected.items():
            self.assertEqual(f(*args), res)

    def test_simple_expr(self):
        metadata = {
            'filename' : 'avg.py',
            'name'     : 'avg',
            'qualname' : 'stats.avg',
            'consts'   : {2 : 0},
            'argcount' : 2,
            'varnames' : {'x' : 0, 'y' : 1},
        }

        # code for "return (x+y)/2"
        insts = [
            ('RESUME', 0),
            ('LOAD_FAST', 0, 1),   # 'x'
            ('LOAD_FAST', 1, 1),   # 'y'
            ('BINARY_OP', 0, 1),   # '+'
            ('LOAD_CONST', 0, 1),  # 2
            ('BINARY_OP', 11, 1),   # '/'
            ('RETURN_VALUE', 1),
        ]
        expected = {(3, 4) : 3.5, (-100, 200) : 50, (10, 18) : 14}
        self.assemble_test(insts, metadata, expected)
