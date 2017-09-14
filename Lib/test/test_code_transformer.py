"""
Test the implemenentation of the PEP 511: code transformers.
"""
import unittest
import ast
import os.path
import sys
import types
from test.support.script_helper import assert_python_ok, assert_python_failure


class BytecodeTransformer:
    name = "bytecode"

    def __init__(self):
        self.call = None

    def code_transformer(self, code, context):
        self.call = (code, context)
        consts = ['Ni! Ni! Ni!' if isinstance(const, str) else const
                  for const in code.co_consts]
        return types.CodeType(code.co_argcount,
                              code.co_kwonlyargcount,
                              code.co_nlocals,
                              code.co_stacksize,
                              code.co_flags,
                              code.co_code,
                              tuple(consts),
                              code.co_names,
                              code.co_varnames,
                              code.co_filename,
                              code.co_name,
                              code.co_firstlineno,
                              code.co_lnotab,
                              code.co_freevars,
                              code.co_cellvars)


class KnightsWhoSayNi(ast.NodeTransformer):
    def visit_Str(self, node):
        node.s = 'Ni! Ni! Ni!'
        return node


class ASTTransformer:
    name = "ast"

    def __init__(self):
        self.transformer = KnightsWhoSayNi()
        self.call = None

    def ast_transformer(self, tree, context):
        self.call = (tree, context)
        self.transformer.visit(tree)
        return tree


class CodeTransformerTests(unittest.TestCase):
    def setUp(self):
        transformers = sys.get_code_transformers()
        self.addCleanup(sys.set_code_transformers, transformers)

        sys.set_code_transformers([])

    def test_bytecode(self):
        sys.set_code_transformers([BytecodeTransformer()])
        code = compile('print("Hello World")', '<string>', 'exec')
        self.assertEqual(code.co_consts, ('Ni! Ni! Ni!', None))

    def test_bytecode_call(self):
        expr = 'x + 1'
        filename = "test.py"
        expected = compile(expr, filename, "exec")

        transformer = BytecodeTransformer()
        sys.set_code_transformers([transformer])
        code = compile(expr, filename, "exec")

        code, context = transformer.call
        self.assertEqual(code, expected)
        self.assertEqual(context.filename, filename)

    def test_ast(self):
        expected = ast.parse('print("Ni! Ni! Ni!")')

        sys.set_code_transformers([ASTTransformer()])
        tree = compile('print("Hello World")', 'string', 'exec',
                       flags=ast.PyCF_TRANSFORMED_AST)
        self.assertEqual(ast.dump(tree), ast.dump(expected))

    def test_ast_call(self):
        code = '1 + 1'
        expected_ast = ast.dump(ast.parse(code))

        filename = "test.py"
        transformer = ASTTransformer()
        sys.set_code_transformers([transformer])
        compile(code, filename, "exec")

        tree, context = transformer.call
        self.assertEqual(ast.dump(tree), expected_ast)
        self.assertEqual(context.filename, filename)


class DefaultTransformerTests(unittest.TestCase):
    def check_default(self, expected, *args):
        code = ('import sys; ',
                'transformers = list(map(type, sys.get_code_transformers()))',
                'print(transformers, sys.implementation.optim_tag)')
        res = assert_python_ok(*args, '-c', '\n'.join(code))
        self.assertEqual(res.out.rstrip(), expected)

    def test_default(self):
        self.check_default(b"[<class 'PeepholeOptimizer'>] opt")

    def test_opt(self):
        self.check_default(b"[<class 'PeepholeOptimizer'>] opt", '-o', 'opt')

    def test_noopt(self):
        self.check_default(b"[] noopt", '-o', 'noopt')


class OptimTagTests(unittest.TestCase):
    """Test -o command line option."""

    def test_invalid_optim_tags(self):
        invalid_tags = ['a.b', 'a%sb' % os.path.sep]
        if os.path.altsep:
            invalid_tags.append('a%sb' % os.path.altsep)
        for optim_tag in invalid_tags:
            code = 'import sys; print(sys.implementation.optim_tag)'
            res = assert_python_failure('-o', optim_tag, '-c', code)
            self.assertIn(b'invalid optimization tag', res.err.rstrip())


if __name__ == "__main__":
    unittest.main()
