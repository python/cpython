import dis
import io
import textwrap
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

    def insts_to_code_object(self, insts, metadata):
        metadata = self.complete_metadata(metadata)
        seq = self.seq_from_insts(insts)
        return self.get_code_object(metadata['filename'], seq, metadata)

    def assemble_test(self, insts, metadata, expected):
        co = self.insts_to_code_object(insts, metadata)
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
            ('RETURN_VALUE', None, 1),
        ]
        expected = {(3, 4) : 3.5, (-100, 200) : 50, (10, 18) : 14}
        self.assemble_test(insts, metadata, expected)


    def test_expression_with_pseudo_instruction_load_closure(self):

        def mod_two(x):
            def inner():
                return x
            return inner() % 2

        inner_code = mod_two.__code__.co_consts[0]
        assert isinstance(inner_code, types.CodeType)

        metadata = {
            'filename' : 'mod_two.py',
            'name'     : 'mod_two',
            'qualname' : 'nested.mod_two',
            'cellvars' : {'x' : 0},
            'consts': {None: 0, inner_code: 1, 2: 2},
            'argcount' : 1,
            'varnames' : {'x' : 0},
        }

        instructions = [
            ('RESUME', 0,),
            ('LOAD_CLOSURE', 0, 1),
            ('BUILD_TUPLE', 1, 1),
            ('LOAD_CONST', 1, 1),
            ('MAKE_FUNCTION', None, 2),
            ('SET_FUNCTION_ATTRIBUTE', 8, 2),
            ('PUSH_NULL', None, 1),
            ('CALL', 0, 2),                     # (lambda: x)()
            ('LOAD_CONST', 2, 2),               # 2
            ('BINARY_OP', 6, 2),                # %
            ('RETURN_VALUE', None, 2)
        ]

        expected = {(0,): 0, (1,): 1, (2,): 0, (120,): 0, (121,): 1}
        self.assemble_test(instructions, metadata, expected)


    def test_exception_table(self):
        metadata = {
            'filename' : 'exc.py',
            'name'     : 'exc',
            'consts'   : {2 : 0},
        }

        # code for "try: pass\n except: pass"
        insts = [
            ('RESUME', 0),
            ('SETUP_FINALLY', 4),
            ('LOAD_CONST', 0),
            ('RETURN_VALUE', None),
            ('SETUP_CLEANUP', 10),
            ('PUSH_EXC_INFO', None),
            ('POP_TOP', None),
            ('POP_EXCEPT', None),
            ('LOAD_CONST', 0),
            ('RETURN_VALUE', None),
            ('COPY', 3),
            ('POP_EXCEPT', None),
            ('RERAISE', 1),
        ]
        co = self.insts_to_code_object(insts, metadata)
        output = io.StringIO()
        dis.dis(co, file=output)
        exc_table = textwrap.dedent("""
                                       ExceptionTable:
                                         L1 to L2 -> L2 [0]
                                         L2 to L3 -> L3 [1] lasti
                                    """)
        self.assertEndsWith(output.getvalue(), exc_table)
