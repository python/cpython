
from test.support.bytecode_helper import CodegenTestCase

# Tests for the code-generation stage of the compiler.
# Examine the un-optimized code generated from the AST.

class IsolatedCodeGenTests(CodegenTestCase):

    def codegen_test(self, snippet, expected_insts):
        import ast
        a = ast.parse(snippet, "my_file.py", "exec");
        insts = self.generate_code(a)
        self.assertInstructionsMatch(insts, expected_insts)

    def test_if_expression(self):
        snippet = "42 if True else 24"
        false_lbl = self.Label()
        expected = [
            ('RESUME', 0, 0),
            ('LOAD_CONST', 0, 1),
            ('POP_JUMP_IF_FALSE', false_lbl := self.Label(), 1),
            ('LOAD_CONST', 1, 1),
            ('JUMP', exit_lbl := self.Label()),
            false_lbl,
            ('LOAD_CONST', 2, 1),
            exit_lbl,
            ('POP_TOP', None),
            ('LOAD_CONST', 3),
            ('RETURN_VALUE', None),
        ]
        self.codegen_test(snippet, expected)

    def test_for_loop(self):
        snippet = "for x in l:\n\tprint(x)"
        false_lbl = self.Label()
        expected = [
            ('RESUME', 0, 0),
            ('LOAD_NAME', 0, 1),
            ('GET_ITER', None, 1),
            loop_lbl := self.Label(),
            ('FOR_ITER', exit_lbl := self.Label(), 1),
            ('NOP', None, 1, 1),
            ('STORE_NAME', 1, 1),
            ('PUSH_NULL', None, 2),
            ('LOAD_NAME', 2, 2),
            ('LOAD_NAME', 1, 2),
            ('CALL', 1, 2),
            ('POP_TOP', None),
            ('JUMP', loop_lbl),
            exit_lbl,
            ('END_FOR', None),
            ('LOAD_CONST', 0),
            ('RETURN_VALUE', None),
        ]
        self.codegen_test(snippet, expected)
