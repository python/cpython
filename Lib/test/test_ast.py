import ast
import dis
import os
import sys
import unittest
import warnings
import weakref
from textwrap import dedent

from test import support

def to_tuple(t):
    if t is None or isinstance(t, (str, int, complex)):
        return t
    elif isinstance(t, list):
        return [to_tuple(e) for e in t]
    result = [t.__class__.__name__]
    if hasattr(t, 'lineno') and hasattr(t, 'col_offset'):
        result.append((t.lineno, t.col_offset))
    if t._fields is None:
        return tuple(result)
    for f in t._fields:
        result.append(to_tuple(getattr(t, f)))
    return tuple(result)


# These tests are compiled through "exec"
# There should be at least one test per statement
exec_tests = [
    # None
    "None",
    # Module docstring
    "'module docstring'",
    # FunctionDef
    "def f(): pass",
    # FunctionDef with docstring
    "def f(): 'function docstring'",
    # FunctionDef with arg
    "def f(a): pass",
    # FunctionDef with arg and default value
    "def f(a=0): pass",
    # FunctionDef with varargs
    "def f(*args): pass",
    # FunctionDef with kwargs
    "def f(**kwargs): pass",
    # FunctionDef with all kind of args and docstring
    "def f(a, b=1, c=None, d=[], e={}, *args, f=42, **kwargs): 'doc for f()'",
    # ClassDef
    "class C:pass",
    # ClassDef with docstring
    "class C: 'docstring for class C'",
    # ClassDef, new style class
    "class C(object): pass",
    # Return
    "def f():return 1",
    # Delete
    "del v",
    # Assign
    "v = 1",
    "a,b = c",
    "(a,b) = c",
    "[a,b] = c",
    # AugAssign
    "v += 1",
    # For
    "for v in v:pass",
    # While
    "while v:pass",
    # If
    "if v:pass",
    # If-Elif
    "if a:\n  pass\nelif b:\n  pass",
    # If-Elif-Else
    "if a:\n  pass\nelif b:\n  pass\nelse:\n  pass",
    # With
    "with x as y: pass",
    "with x as y, z as q: pass",
    # Raise
    "raise Exception('string')",
    # TryExcept
    "try:\n  pass\nexcept Exception:\n  pass",
    # TryFinally
    "try:\n  pass\nfinally:\n  pass",
    # Assert
    "assert v",
    # Import
    "import sys",
    # ImportFrom
    "from sys import v",
    # Global
    "global v",
    # Expr
    "1",
    # Pass,
    "pass",
    # Break
    "for v in v:break",
    # Continue
    "for v in v:continue",
    # for statements with naked tuples (see http://bugs.python.org/issue6704)
    "for a,b in c: pass",
    "for (a,b) in c: pass",
    "for [a,b] in c: pass",
    # Multiline generator expression (test for .lineno & .col_offset)
    """(
    (
    Aa
    ,
       Bb
    )
    for
    Aa
    ,
    Bb in Cc
    )""",
    # dictcomp
    "{a : b for w in x for m in p if g}",
    # dictcomp with naked tuple
    "{a : b for v,w in x}",
    # setcomp
    "{r for l in x if g}",
    # setcomp with naked tuple
    "{r for l,m in x}",
    # AsyncFunctionDef
    "async def f():\n 'async function'\n await something()",
    # AsyncFor
    "async def f():\n async for e in i: 1\n else: 2",
    # AsyncWith
    "async def f():\n async with a as b: 1",
    # PEP 448: Additional Unpacking Generalizations
    "{**{1:2}, 2:3}",
    "{*{1, 2}, 3}",
    # Asynchronous comprehensions
    "async def f():\n [i async for b in c]",
    # Decorated FunctionDef
    "@deco1\n@deco2()\n@deco3(1)\ndef f(): pass",
    # Decorated AsyncFunctionDef
    "@deco1\n@deco2()\n@deco3(1)\nasync def f(): pass",
    # Decorated ClassDef
    "@deco1\n@deco2()\n@deco3(1)\nclass C: pass",
    # Decorator with generator argument
    "@deco(a for a in b)\ndef f(): pass",
    # Simple assignment expression
    "(a := 1)",
    # Positional-only arguments
    "def f(a, /,): pass",
    "def f(a, /, c, d, e): pass",
    "def f(a, /, c, *, d, e): pass",
    "def f(a, /, c, *, d, e, **kwargs): pass",
    # Positional-only arguments with defaults
    "def f(a=1, /,): pass",
    "def f(a=1, /, b=2, c=4): pass",
    "def f(a=1, /, b=2, *, c=4): pass",
    "def f(a=1, /, b=2, *, c): pass",
    "def f(a=1, /, b=2, *, c=4, **kwargs): pass",
    "def f(a=1, /, b=2, *, c, **kwargs): pass",

]

# These are compiled through "single"
# because of overlap with "eval", it just tests what
# can't be tested with "eval"
single_tests = [
    "1+2"
]

# These are compiled through "eval"
# It should test all expressions
eval_tests = [
  # None
  "None",
  # BoolOp
  "a and b",
  # BinOp
  "a + b",
  # UnaryOp
  "not v",
  # Lambda
  "lambda:None",
  # Dict
  "{ 1:2 }",
  # Empty dict
  "{}",
  # Set
  "{None,}",
  # Multiline dict (test for .lineno & .col_offset)
  """{
      1
        :
          2
     }""",
  # ListComp
  "[a for b in c if d]",
  # GeneratorExp
  "(a for b in c if d)",
  # Comprehensions with multiple for targets
  "[(a,b) for a,b in c]",
  "[(a,b) for (a,b) in c]",
  "[(a,b) for [a,b] in c]",
  "{(a,b) for a,b in c}",
  "{(a,b) for (a,b) in c}",
  "{(a,b) for [a,b] in c}",
  "((a,b) for a,b in c)",
  "((a,b) for (a,b) in c)",
  "((a,b) for [a,b] in c)",
  # Yield - yield expressions can't work outside a function
  #
  # Compare
  "1 < 2 < 3",
  # Call
  "f(1,2,c=3,*d,**e)",
  # Call with multi-character starred
  "f(*[0, 1])",
  # Call with a generator argument
  "f(a for a in b)",
  # Num
  "10",
  # Str
  "'string'",
  # Attribute
  "a.b",
  # Subscript
  "a[b:c]",
  # Name
  "v",
  # List
  "[1,2,3]",
  # Empty list
  "[]",
  # Tuple
  "1,2,3",
  # Tuple
  "(1,2,3)",
  # Empty tuple
  "()",
  # Combination
  "a.b.c.d(a.b[1:2])",

]

# TODO: expr_context, slice, boolop, operator, unaryop, cmpop, comprehension
# excepthandler, arguments, keywords, alias

class AST_Tests(unittest.TestCase):

    def _assertTrueorder(self, ast_node, parent_pos):
        if not isinstance(ast_node, ast.AST) or ast_node._fields is None:
            return
        if isinstance(ast_node, (ast.expr, ast.stmt, ast.excepthandler)):
            node_pos = (ast_node.lineno, ast_node.col_offset)
            self.assertGreaterEqual(node_pos, parent_pos)
            parent_pos = (ast_node.lineno, ast_node.col_offset)
        for name in ast_node._fields:
            value = getattr(ast_node, name)
            if isinstance(value, list):
                first_pos = parent_pos
                if value and name == 'decorator_list':
                    first_pos = (value[0].lineno, value[0].col_offset)
                for child in value:
                    self._assertTrueorder(child, first_pos)
            elif value is not None:
                self._assertTrueorder(value, parent_pos)

    def test_AST_objects(self):
        x = ast.AST()
        self.assertEqual(x._fields, ())
        x.foobar = 42
        self.assertEqual(x.foobar, 42)
        self.assertEqual(x.__dict__["foobar"], 42)

        with self.assertRaises(AttributeError):
            x.vararg

        with self.assertRaises(TypeError):
            # "_ast.AST constructor takes 0 positional arguments"
            ast.AST(2)

    def test_AST_garbage_collection(self):
        class X:
            pass
        a = ast.AST()
        a.x = X()
        a.x.a = a
        ref = weakref.ref(a.x)
        del a
        support.gc_collect()
        self.assertIsNone(ref())

    def test_snippets(self):
        for input, output, kind in ((exec_tests, exec_results, "exec"),
                                    (single_tests, single_results, "single"),
                                    (eval_tests, eval_results, "eval")):
            for i, o in zip(input, output):
                with self.subTest(action="parsing", input=i):
                    ast_tree = compile(i, "?", kind, ast.PyCF_ONLY_AST)
                    self.assertEqual(to_tuple(ast_tree), o)
                    self._assertTrueorder(ast_tree, (0, 0))
                with self.subTest(action="compiling", input=i, kind=kind):
                    compile(ast_tree, "?", kind)

    def test_ast_validation(self):
        # compile() is the only function that calls PyAST_Validate
        snippets_to_validate = exec_tests + single_tests + eval_tests
        for snippet in snippets_to_validate:
            tree = ast.parse(snippet)
            compile(tree, '<string>', 'exec')

    def test_slice(self):
        slc = ast.parse("x[::]").body[0].value.slice
        self.assertIsNone(slc.upper)
        self.assertIsNone(slc.lower)
        self.assertIsNone(slc.step)

    def test_from_import(self):
        im = ast.parse("from . import y").body[0]
        self.assertIsNone(im.module)

    def test_non_interned_future_from_ast(self):
        mod = ast.parse("from __future__ import division")
        self.assertIsInstance(mod.body[0], ast.ImportFrom)
        mod.body[0].module = " __future__ ".strip()
        compile(mod, "<test>", "exec")

    def test_base_classes(self):
        self.assertTrue(issubclass(ast.For, ast.stmt))
        self.assertTrue(issubclass(ast.Name, ast.expr))
        self.assertTrue(issubclass(ast.stmt, ast.AST))
        self.assertTrue(issubclass(ast.expr, ast.AST))
        self.assertTrue(issubclass(ast.comprehension, ast.AST))
        self.assertTrue(issubclass(ast.Gt, ast.AST))

    def test_field_attr_existence(self):
        for name, item in ast.__dict__.items():
            if isinstance(item, type) and name != 'AST' and name[0].isupper():
                x = item()
                if isinstance(x, ast.AST):
                    self.assertEqual(type(x._fields), tuple)

    def test_arguments(self):
        x = ast.arguments()
        self.assertEqual(x._fields, ('posonlyargs', 'args', 'vararg', 'kwonlyargs',
                                     'kw_defaults', 'kwarg', 'defaults'))

        with self.assertRaises(AttributeError):
            x.vararg

        x = ast.arguments(*range(1, 8))
        self.assertEqual(x.vararg, 3)

    def test_field_attr_writable(self):
        x = ast.Num()
        # We can assign to _fields
        x._fields = 666
        self.assertEqual(x._fields, 666)

    def test_classattrs(self):
        x = ast.Num()
        self.assertEqual(x._fields, ('value', 'kind'))

        with self.assertRaises(AttributeError):
            x.value

        with self.assertRaises(AttributeError):
            x.n

        x = ast.Num(42)
        self.assertEqual(x.value, 42)
        self.assertEqual(x.n, 42)

        with self.assertRaises(AttributeError):
            x.lineno

        with self.assertRaises(AttributeError):
            x.foobar

        x = ast.Num(lineno=2)
        self.assertEqual(x.lineno, 2)

        x = ast.Num(42, lineno=0)
        self.assertEqual(x.lineno, 0)
        self.assertEqual(x._fields, ('value', 'kind'))
        self.assertEqual(x.value, 42)
        self.assertEqual(x.n, 42)

        self.assertRaises(TypeError, ast.Num, 1, None, 2)
        self.assertRaises(TypeError, ast.Num, 1, None, 2, lineno=0)

        self.assertEqual(ast.Num(42).n, 42)
        self.assertEqual(ast.Num(4.25).n, 4.25)
        self.assertEqual(ast.Num(4.25j).n, 4.25j)
        self.assertEqual(ast.Str('42').s, '42')
        self.assertEqual(ast.Bytes(b'42').s, b'42')
        self.assertIs(ast.NameConstant(True).value, True)
        self.assertIs(ast.NameConstant(False).value, False)
        self.assertIs(ast.NameConstant(None).value, None)

        self.assertEqual(ast.Constant(42).value, 42)
        self.assertEqual(ast.Constant(4.25).value, 4.25)
        self.assertEqual(ast.Constant(4.25j).value, 4.25j)
        self.assertEqual(ast.Constant('42').value, '42')
        self.assertEqual(ast.Constant(b'42').value, b'42')
        self.assertIs(ast.Constant(True).value, True)
        self.assertIs(ast.Constant(False).value, False)
        self.assertIs(ast.Constant(None).value, None)
        self.assertIs(ast.Constant(...).value, ...)

    def test_realtype(self):
        self.assertEqual(type(ast.Num(42)), ast.Constant)
        self.assertEqual(type(ast.Num(4.25)), ast.Constant)
        self.assertEqual(type(ast.Num(4.25j)), ast.Constant)
        self.assertEqual(type(ast.Str('42')), ast.Constant)
        self.assertEqual(type(ast.Bytes(b'42')), ast.Constant)
        self.assertEqual(type(ast.NameConstant(True)), ast.Constant)
        self.assertEqual(type(ast.NameConstant(False)), ast.Constant)
        self.assertEqual(type(ast.NameConstant(None)), ast.Constant)
        self.assertEqual(type(ast.Ellipsis()), ast.Constant)

    def test_isinstance(self):
        self.assertTrue(isinstance(ast.Num(42), ast.Num))
        self.assertTrue(isinstance(ast.Num(4.2), ast.Num))
        self.assertTrue(isinstance(ast.Num(4.2j), ast.Num))
        self.assertTrue(isinstance(ast.Str('42'), ast.Str))
        self.assertTrue(isinstance(ast.Bytes(b'42'), ast.Bytes))
        self.assertTrue(isinstance(ast.NameConstant(True), ast.NameConstant))
        self.assertTrue(isinstance(ast.NameConstant(False), ast.NameConstant))
        self.assertTrue(isinstance(ast.NameConstant(None), ast.NameConstant))
        self.assertTrue(isinstance(ast.Ellipsis(), ast.Ellipsis))

        self.assertTrue(isinstance(ast.Constant(42), ast.Num))
        self.assertTrue(isinstance(ast.Constant(4.2), ast.Num))
        self.assertTrue(isinstance(ast.Constant(4.2j), ast.Num))
        self.assertTrue(isinstance(ast.Constant('42'), ast.Str))
        self.assertTrue(isinstance(ast.Constant(b'42'), ast.Bytes))
        self.assertTrue(isinstance(ast.Constant(True), ast.NameConstant))
        self.assertTrue(isinstance(ast.Constant(False), ast.NameConstant))
        self.assertTrue(isinstance(ast.Constant(None), ast.NameConstant))
        self.assertTrue(isinstance(ast.Constant(...), ast.Ellipsis))

        self.assertFalse(isinstance(ast.Str('42'), ast.Num))
        self.assertFalse(isinstance(ast.Num(42), ast.Str))
        self.assertFalse(isinstance(ast.Str('42'), ast.Bytes))
        self.assertFalse(isinstance(ast.Num(42), ast.NameConstant))
        self.assertFalse(isinstance(ast.Num(42), ast.Ellipsis))
        self.assertFalse(isinstance(ast.NameConstant(True), ast.Num))
        self.assertFalse(isinstance(ast.NameConstant(False), ast.Num))

        self.assertFalse(isinstance(ast.Constant('42'), ast.Num))
        self.assertFalse(isinstance(ast.Constant(42), ast.Str))
        self.assertFalse(isinstance(ast.Constant('42'), ast.Bytes))
        self.assertFalse(isinstance(ast.Constant(42), ast.NameConstant))
        self.assertFalse(isinstance(ast.Constant(42), ast.Ellipsis))
        self.assertFalse(isinstance(ast.Constant(True), ast.Num))
        self.assertFalse(isinstance(ast.Constant(False), ast.Num))

        self.assertFalse(isinstance(ast.Constant(), ast.Num))
        self.assertFalse(isinstance(ast.Constant(), ast.Str))
        self.assertFalse(isinstance(ast.Constant(), ast.Bytes))
        self.assertFalse(isinstance(ast.Constant(), ast.NameConstant))
        self.assertFalse(isinstance(ast.Constant(), ast.Ellipsis))

        class S(str): pass
        self.assertTrue(isinstance(ast.Constant(S('42')), ast.Str))
        self.assertFalse(isinstance(ast.Constant(S('42')), ast.Num))

    def test_subclasses(self):
        class N(ast.Num):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.z = 'spam'
        class N2(ast.Num):
            pass

        n = N(42)
        self.assertEqual(n.n, 42)
        self.assertEqual(n.z, 'spam')
        self.assertEqual(type(n), N)
        self.assertTrue(isinstance(n, N))
        self.assertTrue(isinstance(n, ast.Num))
        self.assertFalse(isinstance(n, N2))
        self.assertFalse(isinstance(ast.Num(42), N))
        n = N(n=42)
        self.assertEqual(n.n, 42)
        self.assertEqual(type(n), N)

    def test_module(self):
        body = [ast.Num(42)]
        x = ast.Module(body, [])
        self.assertEqual(x.body, body)

    def test_nodeclasses(self):
        # Zero arguments constructor explicitly allowed
        x = ast.BinOp()
        self.assertEqual(x._fields, ('left', 'op', 'right'))

        # Random attribute allowed too
        x.foobarbaz = 5
        self.assertEqual(x.foobarbaz, 5)

        n1 = ast.Num(1)
        n3 = ast.Num(3)
        addop = ast.Add()
        x = ast.BinOp(n1, addop, n3)
        self.assertEqual(x.left, n1)
        self.assertEqual(x.op, addop)
        self.assertEqual(x.right, n3)

        x = ast.BinOp(1, 2, 3)
        self.assertEqual(x.left, 1)
        self.assertEqual(x.op, 2)
        self.assertEqual(x.right, 3)

        x = ast.BinOp(1, 2, 3, lineno=0)
        self.assertEqual(x.left, 1)
        self.assertEqual(x.op, 2)
        self.assertEqual(x.right, 3)
        self.assertEqual(x.lineno, 0)

        # node raises exception when given too many arguments
        self.assertRaises(TypeError, ast.BinOp, 1, 2, 3, 4)
        # node raises exception when given too many arguments
        self.assertRaises(TypeError, ast.BinOp, 1, 2, 3, 4, lineno=0)

        # can set attributes through kwargs too
        x = ast.BinOp(left=1, op=2, right=3, lineno=0)
        self.assertEqual(x.left, 1)
        self.assertEqual(x.op, 2)
        self.assertEqual(x.right, 3)
        self.assertEqual(x.lineno, 0)

        # Random kwargs also allowed
        x = ast.BinOp(1, 2, 3, foobarbaz=42)
        self.assertEqual(x.foobarbaz, 42)

    def test_no_fields(self):
        # this used to fail because Sub._fields was None
        x = ast.Sub()
        self.assertEqual(x._fields, ())

    def test_pickling(self):
        import pickle
        mods = [pickle]
        try:
            import cPickle
            mods.append(cPickle)
        except ImportError:
            pass
        protocols = [0, 1, 2]
        for mod in mods:
            for protocol in protocols:
                for ast in (compile(i, "?", "exec", 0x400) for i in exec_tests):
                    ast2 = mod.loads(mod.dumps(ast, protocol))
                    self.assertEqual(to_tuple(ast2), to_tuple(ast))

    def test_invalid_sum(self):
        pos = dict(lineno=2, col_offset=3)
        m = ast.Module([ast.Expr(ast.expr(**pos), **pos)], [])
        with self.assertRaises(TypeError) as cm:
            compile(m, "<test>", "exec")
        self.assertIn("but got <_ast.expr", str(cm.exception))

    def test_invalid_identitifer(self):
        m = ast.Module([ast.Expr(ast.Name(42, ast.Load()))], [])
        ast.fix_missing_locations(m)
        with self.assertRaises(TypeError) as cm:
            compile(m, "<test>", "exec")
        self.assertIn("identifier must be of type str", str(cm.exception))

    def test_empty_yield_from(self):
        # Issue 16546: yield from value is not optional.
        empty_yield_from = ast.parse("def f():\n yield from g()")
        empty_yield_from.body[0].body[0].value.value = None
        with self.assertRaises(ValueError) as cm:
            compile(empty_yield_from, "<test>", "exec")
        self.assertIn("field value is required", str(cm.exception))

    @support.cpython_only
    def test_issue31592(self):
        # There shouldn't be an assertion failure in case of a bad
        # unicodedata.normalize().
        import unicodedata
        def bad_normalize(*args):
            return None
        with support.swap_attr(unicodedata, 'normalize', bad_normalize):
            self.assertRaises(TypeError, ast.parse, '\u03D5')

    def test_issue18374_binop_col_offset(self):
        tree = ast.parse('4+5+6+7')
        parent_binop = tree.body[0].value
        child_binop = parent_binop.left
        grandchild_binop = child_binop.left
        self.assertEqual(parent_binop.col_offset, 0)
        self.assertEqual(parent_binop.end_col_offset, 7)
        self.assertEqual(child_binop.col_offset, 0)
        self.assertEqual(child_binop.end_col_offset, 5)
        self.assertEqual(grandchild_binop.col_offset, 0)
        self.assertEqual(grandchild_binop.end_col_offset, 3)

        tree = ast.parse('4+5-\\\n 6-7')
        parent_binop = tree.body[0].value
        child_binop = parent_binop.left
        grandchild_binop = child_binop.left
        self.assertEqual(parent_binop.col_offset, 0)
        self.assertEqual(parent_binop.lineno, 1)
        self.assertEqual(parent_binop.end_col_offset, 4)
        self.assertEqual(parent_binop.end_lineno, 2)

        self.assertEqual(child_binop.col_offset, 0)
        self.assertEqual(child_binop.lineno, 1)
        self.assertEqual(child_binop.end_col_offset, 2)
        self.assertEqual(child_binop.end_lineno, 2)

        self.assertEqual(grandchild_binop.col_offset, 0)
        self.assertEqual(grandchild_binop.lineno, 1)
        self.assertEqual(grandchild_binop.end_col_offset, 3)
        self.assertEqual(grandchild_binop.end_lineno, 1)

class ASTHelpers_Test(unittest.TestCase):
    maxDiff = None

    def test_parse(self):
        a = ast.parse('foo(1 + 1)')
        b = compile('foo(1 + 1)', '<unknown>', 'exec', ast.PyCF_ONLY_AST)
        self.assertEqual(ast.dump(a), ast.dump(b))

    def test_parse_in_error(self):
        try:
            1/0
        except Exception:
            with self.assertRaises(SyntaxError) as e:
                ast.literal_eval(r"'\U'")
            self.assertIsNotNone(e.exception.__context__)

    def test_dump(self):
        node = ast.parse('spam(eggs, "and cheese")')
        self.assertEqual(ast.dump(node),
            "Module(body=[Expr(value=Call(func=Name(id='spam', ctx=Load()), "
            "args=[Name(id='eggs', ctx=Load()), Constant(value='and cheese', kind=None)], "
            "keywords=[]))], type_ignores=[])"
        )
        self.assertEqual(ast.dump(node, annotate_fields=False),
            "Module([Expr(Call(Name('spam', Load()), [Name('eggs', Load()), "
            "Constant('and cheese', None)], []))], [])"
        )
        self.assertEqual(ast.dump(node, include_attributes=True),
            "Module(body=[Expr(value=Call(func=Name(id='spam', ctx=Load(), "
            "lineno=1, col_offset=0, end_lineno=1, end_col_offset=4), "
            "args=[Name(id='eggs', ctx=Load(), lineno=1, col_offset=5, "
            "end_lineno=1, end_col_offset=9), Constant(value='and cheese', kind=None, "
            "lineno=1, col_offset=11, end_lineno=1, end_col_offset=23)], keywords=[], "
            "lineno=1, col_offset=0, end_lineno=1, end_col_offset=24), "
            "lineno=1, col_offset=0, end_lineno=1, end_col_offset=24)], type_ignores=[])"
        )

    def test_dump_incomplete(self):
        node = ast.Raise(lineno=3, col_offset=4)
        self.assertEqual(ast.dump(node),
            "Raise()"
        )
        self.assertEqual(ast.dump(node, include_attributes=True),
            "Raise(lineno=3, col_offset=4)"
        )
        node = ast.Raise(exc=ast.Name(id='e', ctx=ast.Load()), lineno=3, col_offset=4)
        self.assertEqual(ast.dump(node),
            "Raise(exc=Name(id='e', ctx=Load()))"
        )
        self.assertEqual(ast.dump(node, annotate_fields=False),
            "Raise(Name('e', Load()))"
        )
        self.assertEqual(ast.dump(node, include_attributes=True),
            "Raise(exc=Name(id='e', ctx=Load()), lineno=3, col_offset=4)"
        )
        self.assertEqual(ast.dump(node, annotate_fields=False, include_attributes=True),
            "Raise(Name('e', Load()), lineno=3, col_offset=4)"
        )
        node = ast.Raise(cause=ast.Name(id='e', ctx=ast.Load()))
        self.assertEqual(ast.dump(node),
            "Raise(cause=Name(id='e', ctx=Load()))"
        )
        self.assertEqual(ast.dump(node, annotate_fields=False),
            "Raise(cause=Name('e', Load()))"
        )

    def test_copy_location(self):
        src = ast.parse('1 + 1', mode='eval')
        src.body.right = ast.copy_location(ast.Num(2), src.body.right)
        self.assertEqual(ast.dump(src, include_attributes=True),
            'Expression(body=BinOp(left=Constant(value=1, kind=None, lineno=1, col_offset=0, '
            'end_lineno=1, end_col_offset=1), op=Add(), right=Constant(value=2, '
            'lineno=1, col_offset=4, end_lineno=1, end_col_offset=5), lineno=1, '
            'col_offset=0, end_lineno=1, end_col_offset=5))'
        )

    def test_fix_missing_locations(self):
        src = ast.parse('write("spam")')
        src.body.append(ast.Expr(ast.Call(ast.Name('spam', ast.Load()),
                                          [ast.Str('eggs')], [])))
        self.assertEqual(src, ast.fix_missing_locations(src))
        self.maxDiff = None
        self.assertEqual(ast.dump(src, include_attributes=True),
            "Module(body=[Expr(value=Call(func=Name(id='write', ctx=Load(), "
            "lineno=1, col_offset=0, end_lineno=1, end_col_offset=5), "
            "args=[Constant(value='spam', kind=None, lineno=1, col_offset=6, end_lineno=1, "
            "end_col_offset=12)], keywords=[], lineno=1, col_offset=0, end_lineno=1, "
            "end_col_offset=13), lineno=1, col_offset=0, end_lineno=1, "
            "end_col_offset=13), Expr(value=Call(func=Name(id='spam', ctx=Load(), "
            "lineno=1, col_offset=0, end_lineno=1, end_col_offset=0), "
            "args=[Constant(value='eggs', lineno=1, col_offset=0, end_lineno=1, "
            "end_col_offset=0)], keywords=[], lineno=1, col_offset=0, end_lineno=1, "
            "end_col_offset=0), lineno=1, col_offset=0, end_lineno=1, end_col_offset=0)], "
            "type_ignores=[])"
        )

    def test_increment_lineno(self):
        src = ast.parse('1 + 1', mode='eval')
        self.assertEqual(ast.increment_lineno(src, n=3), src)
        self.assertEqual(ast.dump(src, include_attributes=True),
            'Expression(body=BinOp(left=Constant(value=1, kind=None, lineno=4, col_offset=0, '
            'end_lineno=4, end_col_offset=1), op=Add(), right=Constant(value=1, kind=None, '
            'lineno=4, col_offset=4, end_lineno=4, end_col_offset=5), lineno=4, '
            'col_offset=0, end_lineno=4, end_col_offset=5))'
        )
        # issue10869: do not increment lineno of root twice
        src = ast.parse('1 + 1', mode='eval')
        self.assertEqual(ast.increment_lineno(src.body, n=3), src.body)
        self.assertEqual(ast.dump(src, include_attributes=True),
            'Expression(body=BinOp(left=Constant(value=1, kind=None, lineno=4, col_offset=0, '
            'end_lineno=4, end_col_offset=1), op=Add(), right=Constant(value=1, kind=None, '
            'lineno=4, col_offset=4, end_lineno=4, end_col_offset=5), lineno=4, '
            'col_offset=0, end_lineno=4, end_col_offset=5))'
        )

    def test_iter_fields(self):
        node = ast.parse('foo()', mode='eval')
        d = dict(ast.iter_fields(node.body))
        self.assertEqual(d.pop('func').id, 'foo')
        self.assertEqual(d, {'keywords': [], 'args': []})

    def test_iter_child_nodes(self):
        node = ast.parse("spam(23, 42, eggs='leek')", mode='eval')
        self.assertEqual(len(list(ast.iter_child_nodes(node.body))), 4)
        iterator = ast.iter_child_nodes(node.body)
        self.assertEqual(next(iterator).id, 'spam')
        self.assertEqual(next(iterator).value, 23)
        self.assertEqual(next(iterator).value, 42)
        self.assertEqual(ast.dump(next(iterator)),
            "keyword(arg='eggs', value=Constant(value='leek', kind=None))"
        )

    def test_get_docstring(self):
        node = ast.parse('"""line one\n  line two"""')
        self.assertEqual(ast.get_docstring(node),
                         'line one\nline two')

        node = ast.parse('class foo:\n  """line one\n  line two"""')
        self.assertEqual(ast.get_docstring(node.body[0]),
                         'line one\nline two')

        node = ast.parse('def foo():\n  """line one\n  line two"""')
        self.assertEqual(ast.get_docstring(node.body[0]),
                         'line one\nline two')

        node = ast.parse('async def foo():\n  """spam\n  ham"""')
        self.assertEqual(ast.get_docstring(node.body[0]), 'spam\nham')

    def test_get_docstring_none(self):
        self.assertIsNone(ast.get_docstring(ast.parse('')))
        node = ast.parse('x = "not docstring"')
        self.assertIsNone(ast.get_docstring(node))
        node = ast.parse('def foo():\n  pass')
        self.assertIsNone(ast.get_docstring(node))

        node = ast.parse('class foo:\n  pass')
        self.assertIsNone(ast.get_docstring(node.body[0]))
        node = ast.parse('class foo:\n  x = "not docstring"')
        self.assertIsNone(ast.get_docstring(node.body[0]))
        node = ast.parse('class foo:\n  def bar(self): pass')
        self.assertIsNone(ast.get_docstring(node.body[0]))

        node = ast.parse('def foo():\n  pass')
        self.assertIsNone(ast.get_docstring(node.body[0]))
        node = ast.parse('def foo():\n  x = "not docstring"')
        self.assertIsNone(ast.get_docstring(node.body[0]))

        node = ast.parse('async def foo():\n  pass')
        self.assertIsNone(ast.get_docstring(node.body[0]))
        node = ast.parse('async def foo():\n  x = "not docstring"')
        self.assertIsNone(ast.get_docstring(node.body[0]))

    def test_multi_line_docstring_col_offset_and_lineno_issue16806(self):
        node = ast.parse(
            '"""line one\nline two"""\n\n'
            'def foo():\n  """line one\n  line two"""\n\n'
            '  def bar():\n    """line one\n    line two"""\n'
            '  """line one\n  line two"""\n'
            '"""line one\nline two"""\n\n'
        )
        self.assertEqual(node.body[0].col_offset, 0)
        self.assertEqual(node.body[0].lineno, 1)
        self.assertEqual(node.body[1].body[0].col_offset, 2)
        self.assertEqual(node.body[1].body[0].lineno, 5)
        self.assertEqual(node.body[1].body[1].body[0].col_offset, 4)
        self.assertEqual(node.body[1].body[1].body[0].lineno, 9)
        self.assertEqual(node.body[1].body[2].col_offset, 2)
        self.assertEqual(node.body[1].body[2].lineno, 11)
        self.assertEqual(node.body[2].col_offset, 0)
        self.assertEqual(node.body[2].lineno, 13)

    def test_elif_stmt_start_position(self):
        node = ast.parse('if a:\n    pass\nelif b:\n    pass\n')
        elif_stmt = node.body[0].orelse[0]
        self.assertEqual(elif_stmt.lineno, 3)
        self.assertEqual(elif_stmt.col_offset, 0)

    def test_elif_stmt_start_position_with_else(self):
        node = ast.parse('if a:\n    pass\nelif b:\n    pass\nelse:\n    pass\n')
        elif_stmt = node.body[0].orelse[0]
        self.assertEqual(elif_stmt.lineno, 3)
        self.assertEqual(elif_stmt.col_offset, 0)

    def test_starred_expr_end_position_within_call(self):
        node = ast.parse('f(*[0, 1])')
        starred_expr = node.body[0].value.args[0]
        self.assertEqual(starred_expr.end_lineno, 1)
        self.assertEqual(starred_expr.end_col_offset, 9)

    def test_literal_eval(self):
        self.assertEqual(ast.literal_eval('[1, 2, 3]'), [1, 2, 3])
        self.assertEqual(ast.literal_eval('{"foo": 42}'), {"foo": 42})
        self.assertEqual(ast.literal_eval('(True, False, None)'), (True, False, None))
        self.assertEqual(ast.literal_eval('{1, 2, 3}'), {1, 2, 3})
        self.assertEqual(ast.literal_eval('b"hi"'), b"hi")
        self.assertRaises(ValueError, ast.literal_eval, 'foo()')
        self.assertEqual(ast.literal_eval('6'), 6)
        self.assertEqual(ast.literal_eval('+6'), 6)
        self.assertEqual(ast.literal_eval('-6'), -6)
        self.assertEqual(ast.literal_eval('3.25'), 3.25)
        self.assertEqual(ast.literal_eval('+3.25'), 3.25)
        self.assertEqual(ast.literal_eval('-3.25'), -3.25)
        self.assertEqual(repr(ast.literal_eval('-0.0')), '-0.0')
        self.assertRaises(ValueError, ast.literal_eval, '++6')
        self.assertRaises(ValueError, ast.literal_eval, '+True')
        self.assertRaises(ValueError, ast.literal_eval, '2+3')

    def test_literal_eval_complex(self):
        # Issue #4907
        self.assertEqual(ast.literal_eval('6j'), 6j)
        self.assertEqual(ast.literal_eval('-6j'), -6j)
        self.assertEqual(ast.literal_eval('6.75j'), 6.75j)
        self.assertEqual(ast.literal_eval('-6.75j'), -6.75j)
        self.assertEqual(ast.literal_eval('3+6j'), 3+6j)
        self.assertEqual(ast.literal_eval('-3+6j'), -3+6j)
        self.assertEqual(ast.literal_eval('3-6j'), 3-6j)
        self.assertEqual(ast.literal_eval('-3-6j'), -3-6j)
        self.assertEqual(ast.literal_eval('3.25+6.75j'), 3.25+6.75j)
        self.assertEqual(ast.literal_eval('-3.25+6.75j'), -3.25+6.75j)
        self.assertEqual(ast.literal_eval('3.25-6.75j'), 3.25-6.75j)
        self.assertEqual(ast.literal_eval('-3.25-6.75j'), -3.25-6.75j)
        self.assertEqual(ast.literal_eval('(3+6j)'), 3+6j)
        self.assertRaises(ValueError, ast.literal_eval, '-6j+3')
        self.assertRaises(ValueError, ast.literal_eval, '-6j+3j')
        self.assertRaises(ValueError, ast.literal_eval, '3+-6j')
        self.assertRaises(ValueError, ast.literal_eval, '3+(0+6j)')
        self.assertRaises(ValueError, ast.literal_eval, '-(3+6j)')

    def test_bad_integer(self):
        # issue13436: Bad error message with invalid numeric values
        body = [ast.ImportFrom(module='time',
                               names=[ast.alias(name='sleep')],
                               level=None,
                               lineno=None, col_offset=None)]
        mod = ast.Module(body, [])
        with self.assertRaises(ValueError) as cm:
            compile(mod, 'test', 'exec')
        self.assertIn("invalid integer value: None", str(cm.exception))

    def test_level_as_none(self):
        body = [ast.ImportFrom(module='time',
                               names=[ast.alias(name='sleep')],
                               level=None,
                               lineno=0, col_offset=0)]
        mod = ast.Module(body, [])
        code = compile(mod, 'test', 'exec')
        ns = {}
        exec(code, ns)
        self.assertIn('sleep', ns)


class ASTValidatorTests(unittest.TestCase):

    def mod(self, mod, msg=None, mode="exec", *, exc=ValueError):
        mod.lineno = mod.col_offset = 0
        ast.fix_missing_locations(mod)
        if msg is None:
            compile(mod, "<test>", mode)
        else:
            with self.assertRaises(exc) as cm:
                compile(mod, "<test>", mode)
            self.assertIn(msg, str(cm.exception))

    def expr(self, node, msg=None, *, exc=ValueError):
        mod = ast.Module([ast.Expr(node)], [])
        self.mod(mod, msg, exc=exc)

    def stmt(self, stmt, msg=None):
        mod = ast.Module([stmt], [])
        self.mod(mod, msg)

    def test_module(self):
        m = ast.Interactive([ast.Expr(ast.Name("x", ast.Store()))])
        self.mod(m, "must have Load context", "single")
        m = ast.Expression(ast.Name("x", ast.Store()))
        self.mod(m, "must have Load context", "eval")

    def _check_arguments(self, fac, check):
        def arguments(args=None, posonlyargs=None, vararg=None,
                      kwonlyargs=None, kwarg=None,
                      defaults=None, kw_defaults=None):
            if args is None:
                args = []
            if posonlyargs is None:
                posonlyargs = []
            if kwonlyargs is None:
                kwonlyargs = []
            if defaults is None:
                defaults = []
            if kw_defaults is None:
                kw_defaults = []
            args = ast.arguments(args, posonlyargs, vararg, kwonlyargs,
                                 kw_defaults, kwarg, defaults)
            return fac(args)
        args = [ast.arg("x", ast.Name("x", ast.Store()))]
        check(arguments(args=args), "must have Load context")
        check(arguments(posonlyargs=args), "must have Load context")
        check(arguments(kwonlyargs=args), "must have Load context")
        check(arguments(defaults=[ast.Num(3)]),
                       "more positional defaults than args")
        check(arguments(kw_defaults=[ast.Num(4)]),
                       "length of kwonlyargs is not the same as kw_defaults")
        args = [ast.arg("x", ast.Name("x", ast.Load()))]
        check(arguments(args=args, defaults=[ast.Name("x", ast.Store())]),
                       "must have Load context")
        args = [ast.arg("a", ast.Name("x", ast.Load())),
                ast.arg("b", ast.Name("y", ast.Load()))]
        check(arguments(kwonlyargs=args,
                          kw_defaults=[None, ast.Name("x", ast.Store())]),
                          "must have Load context")

    def test_funcdef(self):
        a = ast.arguments([], [], None, [], [], None, [])
        f = ast.FunctionDef("x", a, [], [], None)
        self.stmt(f, "empty body on FunctionDef")
        f = ast.FunctionDef("x", a, [ast.Pass()], [ast.Name("x", ast.Store())],
                            None)
        self.stmt(f, "must have Load context")
        f = ast.FunctionDef("x", a, [ast.Pass()], [],
                            ast.Name("x", ast.Store()))
        self.stmt(f, "must have Load context")
        def fac(args):
            return ast.FunctionDef("x", args, [ast.Pass()], [], None)
        self._check_arguments(fac, self.stmt)

    def test_classdef(self):
        def cls(bases=None, keywords=None, body=None, decorator_list=None):
            if bases is None:
                bases = []
            if keywords is None:
                keywords = []
            if body is None:
                body = [ast.Pass()]
            if decorator_list is None:
                decorator_list = []
            return ast.ClassDef("myclass", bases, keywords,
                                body, decorator_list)
        self.stmt(cls(bases=[ast.Name("x", ast.Store())]),
                  "must have Load context")
        self.stmt(cls(keywords=[ast.keyword("x", ast.Name("x", ast.Store()))]),
                  "must have Load context")
        self.stmt(cls(body=[]), "empty body on ClassDef")
        self.stmt(cls(body=[None]), "None disallowed")
        self.stmt(cls(decorator_list=[ast.Name("x", ast.Store())]),
                  "must have Load context")

    def test_delete(self):
        self.stmt(ast.Delete([]), "empty targets on Delete")
        self.stmt(ast.Delete([None]), "None disallowed")
        self.stmt(ast.Delete([ast.Name("x", ast.Load())]),
                  "must have Del context")

    def test_assign(self):
        self.stmt(ast.Assign([], ast.Num(3)), "empty targets on Assign")
        self.stmt(ast.Assign([None], ast.Num(3)), "None disallowed")
        self.stmt(ast.Assign([ast.Name("x", ast.Load())], ast.Num(3)),
                  "must have Store context")
        self.stmt(ast.Assign([ast.Name("x", ast.Store())],
                                ast.Name("y", ast.Store())),
                  "must have Load context")

    def test_augassign(self):
        aug = ast.AugAssign(ast.Name("x", ast.Load()), ast.Add(),
                            ast.Name("y", ast.Load()))
        self.stmt(aug, "must have Store context")
        aug = ast.AugAssign(ast.Name("x", ast.Store()), ast.Add(),
                            ast.Name("y", ast.Store()))
        self.stmt(aug, "must have Load context")

    def test_for(self):
        x = ast.Name("x", ast.Store())
        y = ast.Name("y", ast.Load())
        p = ast.Pass()
        self.stmt(ast.For(x, y, [], []), "empty body on For")
        self.stmt(ast.For(ast.Name("x", ast.Load()), y, [p], []),
                  "must have Store context")
        self.stmt(ast.For(x, ast.Name("y", ast.Store()), [p], []),
                  "must have Load context")
        e = ast.Expr(ast.Name("x", ast.Store()))
        self.stmt(ast.For(x, y, [e], []), "must have Load context")
        self.stmt(ast.For(x, y, [p], [e]), "must have Load context")

    def test_while(self):
        self.stmt(ast.While(ast.Num(3), [], []), "empty body on While")
        self.stmt(ast.While(ast.Name("x", ast.Store()), [ast.Pass()], []),
                  "must have Load context")
        self.stmt(ast.While(ast.Num(3), [ast.Pass()],
                             [ast.Expr(ast.Name("x", ast.Store()))]),
                             "must have Load context")

    def test_if(self):
        self.stmt(ast.If(ast.Num(3), [], []), "empty body on If")
        i = ast.If(ast.Name("x", ast.Store()), [ast.Pass()], [])
        self.stmt(i, "must have Load context")
        i = ast.If(ast.Num(3), [ast.Expr(ast.Name("x", ast.Store()))], [])
        self.stmt(i, "must have Load context")
        i = ast.If(ast.Num(3), [ast.Pass()],
                   [ast.Expr(ast.Name("x", ast.Store()))])
        self.stmt(i, "must have Load context")

    def test_with(self):
        p = ast.Pass()
        self.stmt(ast.With([], [p]), "empty items on With")
        i = ast.withitem(ast.Num(3), None)
        self.stmt(ast.With([i], []), "empty body on With")
        i = ast.withitem(ast.Name("x", ast.Store()), None)
        self.stmt(ast.With([i], [p]), "must have Load context")
        i = ast.withitem(ast.Num(3), ast.Name("x", ast.Load()))
        self.stmt(ast.With([i], [p]), "must have Store context")

    def test_raise(self):
        r = ast.Raise(None, ast.Num(3))
        self.stmt(r, "Raise with cause but no exception")
        r = ast.Raise(ast.Name("x", ast.Store()), None)
        self.stmt(r, "must have Load context")
        r = ast.Raise(ast.Num(4), ast.Name("x", ast.Store()))
        self.stmt(r, "must have Load context")

    def test_try(self):
        p = ast.Pass()
        t = ast.Try([], [], [], [p])
        self.stmt(t, "empty body on Try")
        t = ast.Try([ast.Expr(ast.Name("x", ast.Store()))], [], [], [p])
        self.stmt(t, "must have Load context")
        t = ast.Try([p], [], [], [])
        self.stmt(t, "Try has neither except handlers nor finalbody")
        t = ast.Try([p], [], [p], [p])
        self.stmt(t, "Try has orelse but no except handlers")
        t = ast.Try([p], [ast.ExceptHandler(None, "x", [])], [], [])
        self.stmt(t, "empty body on ExceptHandler")
        e = [ast.ExceptHandler(ast.Name("x", ast.Store()), "y", [p])]
        self.stmt(ast.Try([p], e, [], []), "must have Load context")
        e = [ast.ExceptHandler(None, "x", [p])]
        t = ast.Try([p], e, [ast.Expr(ast.Name("x", ast.Store()))], [p])
        self.stmt(t, "must have Load context")
        t = ast.Try([p], e, [p], [ast.Expr(ast.Name("x", ast.Store()))])
        self.stmt(t, "must have Load context")

    def test_assert(self):
        self.stmt(ast.Assert(ast.Name("x", ast.Store()), None),
                  "must have Load context")
        assrt = ast.Assert(ast.Name("x", ast.Load()),
                           ast.Name("y", ast.Store()))
        self.stmt(assrt, "must have Load context")

    def test_import(self):
        self.stmt(ast.Import([]), "empty names on Import")

    def test_importfrom(self):
        imp = ast.ImportFrom(None, [ast.alias("x", None)], -42)
        self.stmt(imp, "Negative ImportFrom level")
        self.stmt(ast.ImportFrom(None, [], 0), "empty names on ImportFrom")

    def test_global(self):
        self.stmt(ast.Global([]), "empty names on Global")

    def test_nonlocal(self):
        self.stmt(ast.Nonlocal([]), "empty names on Nonlocal")

    def test_expr(self):
        e = ast.Expr(ast.Name("x", ast.Store()))
        self.stmt(e, "must have Load context")

    def test_boolop(self):
        b = ast.BoolOp(ast.And(), [])
        self.expr(b, "less than 2 values")
        b = ast.BoolOp(ast.And(), [ast.Num(3)])
        self.expr(b, "less than 2 values")
        b = ast.BoolOp(ast.And(), [ast.Num(4), None])
        self.expr(b, "None disallowed")
        b = ast.BoolOp(ast.And(), [ast.Num(4), ast.Name("x", ast.Store())])
        self.expr(b, "must have Load context")

    def test_unaryop(self):
        u = ast.UnaryOp(ast.Not(), ast.Name("x", ast.Store()))
        self.expr(u, "must have Load context")

    def test_lambda(self):
        a = ast.arguments([], [], None, [], [], None, [])
        self.expr(ast.Lambda(a, ast.Name("x", ast.Store())),
                  "must have Load context")
        def fac(args):
            return ast.Lambda(args, ast.Name("x", ast.Load()))
        self._check_arguments(fac, self.expr)

    def test_ifexp(self):
        l = ast.Name("x", ast.Load())
        s = ast.Name("y", ast.Store())
        for args in (s, l, l), (l, s, l), (l, l, s):
            self.expr(ast.IfExp(*args), "must have Load context")

    def test_dict(self):
        d = ast.Dict([], [ast.Name("x", ast.Load())])
        self.expr(d, "same number of keys as values")
        d = ast.Dict([ast.Name("x", ast.Load())], [None])
        self.expr(d, "None disallowed")

    def test_set(self):
        self.expr(ast.Set([None]), "None disallowed")
        s = ast.Set([ast.Name("x", ast.Store())])
        self.expr(s, "must have Load context")

    def _check_comprehension(self, fac):
        self.expr(fac([]), "comprehension with no generators")
        g = ast.comprehension(ast.Name("x", ast.Load()),
                              ast.Name("x", ast.Load()), [], 0)
        self.expr(fac([g]), "must have Store context")
        g = ast.comprehension(ast.Name("x", ast.Store()),
                              ast.Name("x", ast.Store()), [], 0)
        self.expr(fac([g]), "must have Load context")
        x = ast.Name("x", ast.Store())
        y = ast.Name("y", ast.Load())
        g = ast.comprehension(x, y, [None], 0)
        self.expr(fac([g]), "None disallowed")
        g = ast.comprehension(x, y, [ast.Name("x", ast.Store())], 0)
        self.expr(fac([g]), "must have Load context")

    def _simple_comp(self, fac):
        g = ast.comprehension(ast.Name("x", ast.Store()),
                              ast.Name("x", ast.Load()), [], 0)
        self.expr(fac(ast.Name("x", ast.Store()), [g]),
                  "must have Load context")
        def wrap(gens):
            return fac(ast.Name("x", ast.Store()), gens)
        self._check_comprehension(wrap)

    def test_listcomp(self):
        self._simple_comp(ast.ListComp)

    def test_setcomp(self):
        self._simple_comp(ast.SetComp)

    def test_generatorexp(self):
        self._simple_comp(ast.GeneratorExp)

    def test_dictcomp(self):
        g = ast.comprehension(ast.Name("y", ast.Store()),
                              ast.Name("p", ast.Load()), [], 0)
        c = ast.DictComp(ast.Name("x", ast.Store()),
                         ast.Name("y", ast.Load()), [g])
        self.expr(c, "must have Load context")
        c = ast.DictComp(ast.Name("x", ast.Load()),
                         ast.Name("y", ast.Store()), [g])
        self.expr(c, "must have Load context")
        def factory(comps):
            k = ast.Name("x", ast.Load())
            v = ast.Name("y", ast.Load())
            return ast.DictComp(k, v, comps)
        self._check_comprehension(factory)

    def test_yield(self):
        self.expr(ast.Yield(ast.Name("x", ast.Store())), "must have Load")
        self.expr(ast.YieldFrom(ast.Name("x", ast.Store())), "must have Load")

    def test_compare(self):
        left = ast.Name("x", ast.Load())
        comp = ast.Compare(left, [ast.In()], [])
        self.expr(comp, "no comparators")
        comp = ast.Compare(left, [ast.In()], [ast.Num(4), ast.Num(5)])
        self.expr(comp, "different number of comparators and operands")
        comp = ast.Compare(ast.Num("blah"), [ast.In()], [left])
        self.expr(comp)
        comp = ast.Compare(left, [ast.In()], [ast.Num("blah")])
        self.expr(comp)

    def test_call(self):
        func = ast.Name("x", ast.Load())
        args = [ast.Name("y", ast.Load())]
        keywords = [ast.keyword("w", ast.Name("z", ast.Load()))]
        call = ast.Call(ast.Name("x", ast.Store()), args, keywords)
        self.expr(call, "must have Load context")
        call = ast.Call(func, [None], keywords)
        self.expr(call, "None disallowed")
        bad_keywords = [ast.keyword("w", ast.Name("z", ast.Store()))]
        call = ast.Call(func, args, bad_keywords)
        self.expr(call, "must have Load context")

    def test_num(self):
        class subint(int):
            pass
        class subfloat(float):
            pass
        class subcomplex(complex):
            pass
        for obj in "0", "hello":
            self.expr(ast.Num(obj))
        for obj in subint(), subfloat(), subcomplex():
            self.expr(ast.Num(obj), "invalid type", exc=TypeError)

    def test_attribute(self):
        attr = ast.Attribute(ast.Name("x", ast.Store()), "y", ast.Load())
        self.expr(attr, "must have Load context")

    def test_subscript(self):
        sub = ast.Subscript(ast.Name("x", ast.Store()), ast.Index(ast.Num(3)),
                            ast.Load())
        self.expr(sub, "must have Load context")
        x = ast.Name("x", ast.Load())
        sub = ast.Subscript(x, ast.Index(ast.Name("y", ast.Store())),
                            ast.Load())
        self.expr(sub, "must have Load context")
        s = ast.Name("x", ast.Store())
        for args in (s, None, None), (None, s, None), (None, None, s):
            sl = ast.Slice(*args)
            self.expr(ast.Subscript(x, sl, ast.Load()),
                      "must have Load context")
        sl = ast.ExtSlice([])
        self.expr(ast.Subscript(x, sl, ast.Load()), "empty dims on ExtSlice")
        sl = ast.ExtSlice([ast.Index(s)])
        self.expr(ast.Subscript(x, sl, ast.Load()), "must have Load context")

    def test_starred(self):
        left = ast.List([ast.Starred(ast.Name("x", ast.Load()), ast.Store())],
                        ast.Store())
        assign = ast.Assign([left], ast.Num(4))
        self.stmt(assign, "must have Store context")

    def _sequence(self, fac):
        self.expr(fac([None], ast.Load()), "None disallowed")
        self.expr(fac([ast.Name("x", ast.Store())], ast.Load()),
                  "must have Load context")

    def test_list(self):
        self._sequence(ast.List)

    def test_tuple(self):
        self._sequence(ast.Tuple)

    def test_nameconstant(self):
        self.expr(ast.NameConstant(4))

    def test_stdlib_validates(self):
        stdlib = os.path.dirname(ast.__file__)
        tests = [fn for fn in os.listdir(stdlib) if fn.endswith(".py")]
        tests.extend(["test/test_grammar.py", "test/test_unpack_ex.py"])
        for module in tests:
            with self.subTest(module):
                fn = os.path.join(stdlib, module)
                with open(fn, "r", encoding="utf-8") as fp:
                    source = fp.read()
                mod = ast.parse(source, fn)
                compile(mod, fn, "exec")


class ConstantTests(unittest.TestCase):
    """Tests on the ast.Constant node type."""

    def compile_constant(self, value):
        tree = ast.parse("x = 123")

        node = tree.body[0].value
        new_node = ast.Constant(value=value)
        ast.copy_location(new_node, node)
        tree.body[0].value = new_node

        code = compile(tree, "<string>", "exec")

        ns = {}
        exec(code, ns)
        return ns['x']

    def test_validation(self):
        with self.assertRaises(TypeError) as cm:
            self.compile_constant([1, 2, 3])
        self.assertEqual(str(cm.exception),
                         "got an invalid type in Constant: list")

    def test_singletons(self):
        for const in (None, False, True, Ellipsis, b'', frozenset()):
            with self.subTest(const=const):
                value = self.compile_constant(const)
                self.assertIs(value, const)

    def test_values(self):
        nested_tuple = (1,)
        nested_frozenset = frozenset({1})
        for level in range(3):
            nested_tuple = (nested_tuple, 2)
            nested_frozenset = frozenset({nested_frozenset, 2})
        values = (123, 123.0, 123j,
                  "unicode", b'bytes',
                  tuple("tuple"), frozenset("frozenset"),
                  nested_tuple, nested_frozenset)
        for value in values:
            with self.subTest(value=value):
                result = self.compile_constant(value)
                self.assertEqual(result, value)

    def test_assign_to_constant(self):
        tree = ast.parse("x = 1")

        target = tree.body[0].targets[0]
        new_target = ast.Constant(value=1)
        ast.copy_location(new_target, target)
        tree.body[0].targets[0] = new_target

        with self.assertRaises(ValueError) as cm:
            compile(tree, "string", "exec")
        self.assertEqual(str(cm.exception),
                         "expression which can't be assigned "
                         "to in Store context")

    def test_get_docstring(self):
        tree = ast.parse("'docstring'\nx = 1")
        self.assertEqual(ast.get_docstring(tree), 'docstring')

    def get_load_const(self, tree):
        # Compile to bytecode, disassemble and get parameter of LOAD_CONST
        # instructions
        co = compile(tree, '<string>', 'exec')
        consts = []
        for instr in dis.get_instructions(co):
            if instr.opname == 'LOAD_CONST':
                consts.append(instr.argval)
        return consts

    @support.cpython_only
    def test_load_const(self):
        consts = [None,
                  True, False,
                  124,
                  2.0,
                  3j,
                  "unicode",
                  b'bytes',
                  (1, 2, 3)]

        code = '\n'.join(['x={!r}'.format(const) for const in consts])
        code += '\nx = ...'
        consts.extend((Ellipsis, None))

        tree = ast.parse(code)
        self.assertEqual(self.get_load_const(tree),
                         consts)

        # Replace expression nodes with constants
        for assign, const in zip(tree.body, consts):
            assert isinstance(assign, ast.Assign), ast.dump(assign)
            new_node = ast.Constant(value=const)
            ast.copy_location(new_node, assign.value)
            assign.value = new_node

        self.assertEqual(self.get_load_const(tree),
                         consts)

    def test_literal_eval(self):
        tree = ast.parse("1 + 2")
        binop = tree.body[0].value

        new_left = ast.Constant(value=10)
        ast.copy_location(new_left, binop.left)
        binop.left = new_left

        new_right = ast.Constant(value=20j)
        ast.copy_location(new_right, binop.right)
        binop.right = new_right

        self.assertEqual(ast.literal_eval(binop), 10+20j)

    def test_string_kind(self):
        c = ast.parse('"x"', mode='eval').body
        self.assertEqual(c.value, "x")
        self.assertEqual(c.kind, None)

        c = ast.parse('u"x"', mode='eval').body
        self.assertEqual(c.value, "x")
        self.assertEqual(c.kind, "u")

        c = ast.parse('r"x"', mode='eval').body
        self.assertEqual(c.value, "x")
        self.assertEqual(c.kind, None)

        c = ast.parse('b"x"', mode='eval').body
        self.assertEqual(c.value, b"x")
        self.assertEqual(c.kind, None)


class EndPositionTests(unittest.TestCase):
    """Tests for end position of AST nodes.

    Testing end positions of nodes requires a bit of extra care
    because of how LL parsers work.
    """
    def _check_end_pos(self, ast_node, end_lineno, end_col_offset):
        self.assertEqual(ast_node.end_lineno, end_lineno)
        self.assertEqual(ast_node.end_col_offset, end_col_offset)

    def _check_content(self, source, ast_node, content):
        self.assertEqual(ast.get_source_segment(source, ast_node), content)

    def _parse_value(self, s):
        # Use duck-typing to support both single expression
        # and a right hand side of an assignment statement.
        return ast.parse(s).body[0].value

    def test_lambda(self):
        s = 'lambda x, *y: None'
        lam = self._parse_value(s)
        self._check_content(s, lam.body, 'None')
        self._check_content(s, lam.args.args[0], 'x')
        self._check_content(s, lam.args.vararg, 'y')

    def test_func_def(self):
        s = dedent('''
            def func(x: int,
                     *args: str,
                     z: float = 0,
                     **kwargs: Any) -> bool:
                return True
            ''').strip()
        fdef = ast.parse(s).body[0]
        self._check_end_pos(fdef, 5, 15)
        self._check_content(s, fdef.body[0], 'return True')
        self._check_content(s, fdef.args.args[0], 'x: int')
        self._check_content(s, fdef.args.args[0].annotation, 'int')
        self._check_content(s, fdef.args.kwarg, 'kwargs: Any')
        self._check_content(s, fdef.args.kwarg.annotation, 'Any')

    def test_call(self):
        s = 'func(x, y=2, **kw)'
        call = self._parse_value(s)
        self._check_content(s, call.func, 'func')
        self._check_content(s, call.keywords[0].value, '2')
        self._check_content(s, call.keywords[1].value, 'kw')

    def test_call_noargs(self):
        s = 'x[0]()'
        call = self._parse_value(s)
        self._check_content(s, call.func, 'x[0]')
        self._check_end_pos(call, 1, 6)

    def test_class_def(self):
        s = dedent('''
            class C(A, B):
                x: int = 0
        ''').strip()
        cdef = ast.parse(s).body[0]
        self._check_end_pos(cdef, 2, 14)
        self._check_content(s, cdef.bases[1], 'B')
        self._check_content(s, cdef.body[0], 'x: int = 0')

    def test_class_kw(self):
        s = 'class S(metaclass=abc.ABCMeta): pass'
        cdef = ast.parse(s).body[0]
        self._check_content(s, cdef.keywords[0].value, 'abc.ABCMeta')

    def test_multi_line_str(self):
        s = dedent('''
            x = """Some multi-line text.

            It goes on starting from same indent."""
        ''').strip()
        assign = ast.parse(s).body[0]
        self._check_end_pos(assign, 3, 40)
        self._check_end_pos(assign.value, 3, 40)

    def test_continued_str(self):
        s = dedent('''
            x = "first part" \\
            "second part"
        ''').strip()
        assign = ast.parse(s).body[0]
        self._check_end_pos(assign, 2, 13)
        self._check_end_pos(assign.value, 2, 13)

    def test_suites(self):
        # We intentionally put these into the same string to check
        # that empty lines are not part of the suite.
        s = dedent('''
            while True:
                pass

            if one():
                x = None
            elif other():
                y = None
            else:
                z = None

            for x, y in stuff:
                assert True

            try:
                raise RuntimeError
            except TypeError as e:
                pass

            pass
        ''').strip()
        mod = ast.parse(s)
        while_loop = mod.body[0]
        if_stmt = mod.body[1]
        for_loop = mod.body[2]
        try_stmt = mod.body[3]
        pass_stmt = mod.body[4]

        self._check_end_pos(while_loop, 2, 8)
        self._check_end_pos(if_stmt, 9, 12)
        self._check_end_pos(for_loop, 12, 15)
        self._check_end_pos(try_stmt, 17, 8)
        self._check_end_pos(pass_stmt, 19, 4)

        self._check_content(s, while_loop.test, 'True')
        self._check_content(s, if_stmt.body[0], 'x = None')
        self._check_content(s, if_stmt.orelse[0].test, 'other()')
        self._check_content(s, for_loop.target, 'x, y')
        self._check_content(s, try_stmt.body[0], 'raise RuntimeError')
        self._check_content(s, try_stmt.handlers[0].type, 'TypeError')

    def test_fstring(self):
        s = 'x = f"abc {x + y} abc"'
        fstr = self._parse_value(s)
        binop = fstr.values[1].value
        self._check_content(s, binop, 'x + y')

    def test_fstring_multi_line(self):
        s = dedent('''
            f"""Some multi-line text.
            {
            arg_one
            +
            arg_two
            }
            It goes on..."""
        ''').strip()
        fstr = self._parse_value(s)
        binop = fstr.values[1].value
        self._check_end_pos(binop, 5, 7)
        self._check_content(s, binop.left, 'arg_one')
        self._check_content(s, binop.right, 'arg_two')

    def test_import_from_multi_line(self):
        s = dedent('''
            from x.y.z import (
                a, b, c as c
            )
        ''').strip()
        imp = ast.parse(s).body[0]
        self._check_end_pos(imp, 3, 1)

    def test_slices(self):
        s1 = 'f()[1, 2] [0]'
        s2 = 'x[ a.b: c.d]'
        sm = dedent('''
            x[ a.b: f () ,
               g () : c.d
              ]
        ''').strip()
        i1, i2, im = map(self._parse_value, (s1, s2, sm))
        self._check_content(s1, i1.value, 'f()[1, 2]')
        self._check_content(s1, i1.value.slice.value, '1, 2')
        self._check_content(s2, i2.slice.lower, 'a.b')
        self._check_content(s2, i2.slice.upper, 'c.d')
        self._check_content(sm, im.slice.dims[0].upper, 'f ()')
        self._check_content(sm, im.slice.dims[1].lower, 'g ()')
        self._check_end_pos(im, 3, 3)

    def test_binop(self):
        s = dedent('''
            (1 * 2 + (3 ) +
                 4
            )
        ''').strip()
        binop = self._parse_value(s)
        self._check_end_pos(binop, 2, 6)
        self._check_content(s, binop.right, '4')
        self._check_content(s, binop.left, '1 * 2 + (3 )')
        self._check_content(s, binop.left.right, '3')

    def test_boolop(self):
        s = dedent('''
            if (one_condition and
                    (other_condition or yet_another_one)):
                pass
        ''').strip()
        bop = ast.parse(s).body[0].test
        self._check_end_pos(bop, 2, 44)
        self._check_content(s, bop.values[1],
                            'other_condition or yet_another_one')

    def test_tuples(self):
        s1 = 'x = () ;'
        s2 = 'x = 1 , ;'
        s3 = 'x = (1 , 2 ) ;'
        sm = dedent('''
            x = (
                a, b,
            )
        ''').strip()
        t1, t2, t3, tm = map(self._parse_value, (s1, s2, s3, sm))
        self._check_content(s1, t1, '()')
        self._check_content(s2, t2, '1 ,')
        self._check_content(s3, t3, '(1 , 2 )')
        self._check_end_pos(tm, 3, 1)

    def test_attribute_spaces(self):
        s = 'func(x. y .z)'
        call = self._parse_value(s)
        self._check_content(s, call, s)
        self._check_content(s, call.args[0], 'x. y .z')

    def test_displays(self):
        s1 = '[{}, {1, }, {1, 2,} ]'
        s2 = '{a: b, f (): g () ,}'
        c1 = self._parse_value(s1)
        c2 = self._parse_value(s2)
        self._check_content(s1, c1.elts[0], '{}')
        self._check_content(s1, c1.elts[1], '{1, }')
        self._check_content(s1, c1.elts[2], '{1, 2,}')
        self._check_content(s2, c2.keys[1], 'f ()')
        self._check_content(s2, c2.values[1], 'g ()')

    def test_comprehensions(self):
        s = dedent('''
            x = [{x for x, y in stuff
                  if cond.x} for stuff in things]
        ''').strip()
        cmp = self._parse_value(s)
        self._check_end_pos(cmp, 2, 37)
        self._check_content(s, cmp.generators[0].iter, 'things')
        self._check_content(s, cmp.elt.generators[0].iter, 'stuff')
        self._check_content(s, cmp.elt.generators[0].ifs[0], 'cond.x')
        self._check_content(s, cmp.elt.generators[0].target, 'x, y')

    def test_yield_await(self):
        s = dedent('''
            async def f():
                yield x
                await y
        ''').strip()
        fdef = ast.parse(s).body[0]
        self._check_content(s, fdef.body[0].value, 'yield x')
        self._check_content(s, fdef.body[1].value, 'await y')

    def test_source_segment_multi(self):
        s_orig = dedent('''
            x = (
                a, b,
            ) + ()
        ''').strip()
        s_tuple = dedent('''
            (
                a, b,
            )
        ''').strip()
        binop = self._parse_value(s_orig)
        self.assertEqual(ast.get_source_segment(s_orig, binop.left), s_tuple)

    def test_source_segment_padded(self):
        s_orig = dedent('''
            class C:
                def fun(self) -> None:
                    "ЖЖЖЖЖ"
        ''').strip()
        s_method = '    def fun(self) -> None:\n' \
                   '        "ЖЖЖЖЖ"'
        cdef = ast.parse(s_orig).body[0]
        self.assertEqual(ast.get_source_segment(s_orig, cdef.body[0], padded=True),
                         s_method)

    def test_source_segment_endings(self):
        s = 'v = 1\r\nw = 1\nx = 1\n\ry = 1\rz = 1\r\n'
        v, w, x, y, z = ast.parse(s).body
        self._check_content(s, v, 'v = 1')
        self._check_content(s, w, 'w = 1')
        self._check_content(s, x, 'x = 1')
        self._check_content(s, y, 'y = 1')
        self._check_content(s, z, 'z = 1')

    def test_source_segment_tabs(self):
        s = dedent('''
            class C:
              \t\f  def fun(self) -> None:
              \t\f      pass
        ''').strip()
        s_method = '  \t\f  def fun(self) -> None:\n' \
                   '  \t\f      pass'

        cdef = ast.parse(s).body[0]
        self.assertEqual(ast.get_source_segment(s, cdef.body[0], padded=True), s_method)


class NodeVisitorTests(unittest.TestCase):
    def test_old_constant_nodes(self):
        class Visitor(ast.NodeVisitor):
            def visit_Num(self, node):
                log.append((node.lineno, 'Num', node.n))
            def visit_Str(self, node):
                log.append((node.lineno, 'Str', node.s))
            def visit_Bytes(self, node):
                log.append((node.lineno, 'Bytes', node.s))
            def visit_NameConstant(self, node):
                log.append((node.lineno, 'NameConstant', node.value))
            def visit_Ellipsis(self, node):
                log.append((node.lineno, 'Ellipsis', ...))
        mod = ast.parse(dedent('''\
            i = 42
            f = 4.25
            c = 4.25j
            s = 'string'
            b = b'bytes'
            t = True
            n = None
            e = ...
            '''))
        visitor = Visitor()
        log = []
        with warnings.catch_warnings(record=True) as wlog:
            warnings.filterwarnings('always', '', PendingDeprecationWarning)
            visitor.visit(mod)
        self.assertEqual(log, [
            (1, 'Num', 42),
            (2, 'Num', 4.25),
            (3, 'Num', 4.25j),
            (4, 'Str', 'string'),
            (5, 'Bytes', b'bytes'),
            (6, 'NameConstant', True),
            (7, 'NameConstant', None),
            (8, 'Ellipsis', ...),
        ])
        self.assertEqual([str(w.message) for w in wlog], [
            'visit_Num is deprecated; add visit_Constant',
            'visit_Num is deprecated; add visit_Constant',
            'visit_Num is deprecated; add visit_Constant',
            'visit_Str is deprecated; add visit_Constant',
            'visit_Bytes is deprecated; add visit_Constant',
            'visit_NameConstant is deprecated; add visit_Constant',
            'visit_NameConstant is deprecated; add visit_Constant',
            'visit_Ellipsis is deprecated; add visit_Constant',
        ])


def main():
    if __name__ != '__main__':
        return
    if sys.argv[1:] == ['-g']:
        for statements, kind in ((exec_tests, "exec"), (single_tests, "single"),
                                 (eval_tests, "eval")):
            print(kind+"_results = [")
            for statement in statements:
                tree = ast.parse(statement, "?", kind)
                print("%r," % (to_tuple(tree),))
            print("]")
        print("main()")
        raise SystemExit
    unittest.main()

#### EVERYTHING BELOW IS GENERATED BY python Lib/test/test_ast.py -g  #####
exec_results = [
('Module', [('Expr', (1, 0), ('Constant', (1, 0), None, None))], []),
('Module', [('Expr', (1, 0), ('Constant', (1, 0), 'module docstring', None))], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 9))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Expr', (1, 9), ('Constant', (1, 9), 'function docstring', None))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [('arg', (1, 6), 'a', None, None)], None, [], [], None, []), [('Pass', (1, 10))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [('arg', (1, 6), 'a', None, None)], None, [], [], None, [('Constant', (1, 8), 0, None)]), [('Pass', (1, 12))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [], ('arg', (1, 7), 'args', None, None), [], [], None, []), [('Pass', (1, 14))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], ('arg', (1, 8), 'kwargs', None, None), []), [('Pass', (1, 17))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [('arg', (1, 6), 'a', None, None), ('arg', (1, 9), 'b', None, None), ('arg', (1, 14), 'c', None, None), ('arg', (1, 22), 'd', None, None), ('arg', (1, 28), 'e', None, None)], ('arg', (1, 35), 'args', None, None), [('arg', (1, 41), 'f', None, None)], [('Constant', (1, 43), 42, None)], ('arg', (1, 49), 'kwargs', None, None), [('Constant', (1, 11), 1, None), ('Constant', (1, 16), None, None), ('List', (1, 24), [], ('Load',)), ('Dict', (1, 30), [], [])]), [('Expr', (1, 58), ('Constant', (1, 58), 'doc for f()', None))], [], None, None)], []),
('Module', [('ClassDef', (1, 0), 'C', [], [], [('Pass', (1, 8))], [])], []),
('Module', [('ClassDef', (1, 0), 'C', [], [], [('Expr', (1, 9), ('Constant', (1, 9), 'docstring for class C', None))], [])], []),
('Module', [('ClassDef', (1, 0), 'C', [('Name', (1, 8), 'object', ('Load',))], [], [('Pass', (1, 17))], [])], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Return', (1, 8), ('Constant', (1, 15), 1, None))], [], None, None)], []),
('Module', [('Delete', (1, 0), [('Name', (1, 4), 'v', ('Del',))])], []),
('Module', [('Assign', (1, 0), [('Name', (1, 0), 'v', ('Store',))], ('Constant', (1, 4), 1, None), None)], []),
('Module', [('Assign', (1, 0), [('Tuple', (1, 0), [('Name', (1, 0), 'a', ('Store',)), ('Name', (1, 2), 'b', ('Store',))], ('Store',))], ('Name', (1, 6), 'c', ('Load',)), None)], []),
('Module', [('Assign', (1, 0), [('Tuple', (1, 0), [('Name', (1, 1), 'a', ('Store',)), ('Name', (1, 3), 'b', ('Store',))], ('Store',))], ('Name', (1, 8), 'c', ('Load',)), None)], []),
('Module', [('Assign', (1, 0), [('List', (1, 0), [('Name', (1, 1), 'a', ('Store',)), ('Name', (1, 3), 'b', ('Store',))], ('Store',))], ('Name', (1, 8), 'c', ('Load',)), None)], []),
('Module', [('AugAssign', (1, 0), ('Name', (1, 0), 'v', ('Store',)), ('Add',), ('Constant', (1, 5), 1, None))], []),
('Module', [('For', (1, 0), ('Name', (1, 4), 'v', ('Store',)), ('Name', (1, 9), 'v', ('Load',)), [('Pass', (1, 11))], [], None)], []),
('Module', [('While', (1, 0), ('Name', (1, 6), 'v', ('Load',)), [('Pass', (1, 8))], [])], []),
('Module', [('If', (1, 0), ('Name', (1, 3), 'v', ('Load',)), [('Pass', (1, 5))], [])], []),
('Module', [('If', (1, 0), ('Name', (1, 3), 'a', ('Load',)), [('Pass', (2, 2))], [('If', (3, 0), ('Name', (3, 5), 'b', ('Load',)), [('Pass', (4, 2))], [])])], []),
('Module', [('If', (1, 0), ('Name', (1, 3), 'a', ('Load',)), [('Pass', (2, 2))], [('If', (3, 0), ('Name', (3, 5), 'b', ('Load',)), [('Pass', (4, 2))], [('Pass', (6, 2))])])], []),
('Module', [('With', (1, 0), [('withitem', ('Name', (1, 5), 'x', ('Load',)), ('Name', (1, 10), 'y', ('Store',)))], [('Pass', (1, 13))], None)], []),
('Module', [('With', (1, 0), [('withitem', ('Name', (1, 5), 'x', ('Load',)), ('Name', (1, 10), 'y', ('Store',))), ('withitem', ('Name', (1, 13), 'z', ('Load',)), ('Name', (1, 18), 'q', ('Store',)))], [('Pass', (1, 21))], None)], []),
('Module', [('Raise', (1, 0), ('Call', (1, 6), ('Name', (1, 6), 'Exception', ('Load',)), [('Constant', (1, 16), 'string', None)], []), None)], []),
('Module', [('Try', (1, 0), [('Pass', (2, 2))], [('ExceptHandler', (3, 0), ('Name', (3, 7), 'Exception', ('Load',)), None, [('Pass', (4, 2))])], [], [])], []),
('Module', [('Try', (1, 0), [('Pass', (2, 2))], [], [], [('Pass', (4, 2))])], []),
('Module', [('Assert', (1, 0), ('Name', (1, 7), 'v', ('Load',)), None)], []),
('Module', [('Import', (1, 0), [('alias', 'sys', None)])], []),
('Module', [('ImportFrom', (1, 0), 'sys', [('alias', 'v', None)], 0)], []),
('Module', [('Global', (1, 0), ['v'])], []),
('Module', [('Expr', (1, 0), ('Constant', (1, 0), 1, None))], []),
('Module', [('Pass', (1, 0))], []),
('Module', [('For', (1, 0), ('Name', (1, 4), 'v', ('Store',)), ('Name', (1, 9), 'v', ('Load',)), [('Break', (1, 11))], [], None)], []),
('Module', [('For', (1, 0), ('Name', (1, 4), 'v', ('Store',)), ('Name', (1, 9), 'v', ('Load',)), [('Continue', (1, 11))], [], None)], []),
('Module', [('For', (1, 0), ('Tuple', (1, 4), [('Name', (1, 4), 'a', ('Store',)), ('Name', (1, 6), 'b', ('Store',))], ('Store',)), ('Name', (1, 11), 'c', ('Load',)), [('Pass', (1, 14))], [], None)], []),
('Module', [('For', (1, 0), ('Tuple', (1, 4), [('Name', (1, 5), 'a', ('Store',)), ('Name', (1, 7), 'b', ('Store',))], ('Store',)), ('Name', (1, 13), 'c', ('Load',)), [('Pass', (1, 16))], [], None)], []),
('Module', [('For', (1, 0), ('List', (1, 4), [('Name', (1, 5), 'a', ('Store',)), ('Name', (1, 7), 'b', ('Store',))], ('Store',)), ('Name', (1, 13), 'c', ('Load',)), [('Pass', (1, 16))], [], None)], []),
('Module', [('Expr', (1, 0), ('GeneratorExp', (1, 0), ('Tuple', (2, 4), [('Name', (3, 4), 'Aa', ('Load',)), ('Name', (5, 7), 'Bb', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (8, 4), [('Name', (8, 4), 'Aa', ('Store',)), ('Name', (10, 4), 'Bb', ('Store',))], ('Store',)), ('Name', (10, 10), 'Cc', ('Load',)), [], 0)]))], []),
('Module', [('Expr', (1, 0), ('DictComp', (1, 0), ('Name', (1, 1), 'a', ('Load',)), ('Name', (1, 5), 'b', ('Load',)), [('comprehension', ('Name', (1, 11), 'w', ('Store',)), ('Name', (1, 16), 'x', ('Load',)), [], 0), ('comprehension', ('Name', (1, 22), 'm', ('Store',)), ('Name', (1, 27), 'p', ('Load',)), [('Name', (1, 32), 'g', ('Load',))], 0)]))], []),
('Module', [('Expr', (1, 0), ('DictComp', (1, 0), ('Name', (1, 1), 'a', ('Load',)), ('Name', (1, 5), 'b', ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 11), 'v', ('Store',)), ('Name', (1, 13), 'w', ('Store',))], ('Store',)), ('Name', (1, 18), 'x', ('Load',)), [], 0)]))], []),
('Module', [('Expr', (1, 0), ('SetComp', (1, 0), ('Name', (1, 1), 'r', ('Load',)), [('comprehension', ('Name', (1, 7), 'l', ('Store',)), ('Name', (1, 12), 'x', ('Load',)), [('Name', (1, 17), 'g', ('Load',))], 0)]))], []),
('Module', [('Expr', (1, 0), ('SetComp', (1, 0), ('Name', (1, 1), 'r', ('Load',)), [('comprehension', ('Tuple', (1, 7), [('Name', (1, 7), 'l', ('Store',)), ('Name', (1, 9), 'm', ('Store',))], ('Store',)), ('Name', (1, 14), 'x', ('Load',)), [], 0)]))], []),
('Module', [('AsyncFunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Expr', (2, 1), ('Constant', (2, 1), 'async function', None)), ('Expr', (3, 1), ('Await', (3, 1), ('Call', (3, 7), ('Name', (3, 7), 'something', ('Load',)), [], [])))], [], None, None)], []),
('Module', [('AsyncFunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('AsyncFor', (2, 1), ('Name', (2, 11), 'e', ('Store',)), ('Name', (2, 16), 'i', ('Load',)), [('Expr', (2, 19), ('Constant', (2, 19), 1, None))], [('Expr', (3, 7), ('Constant', (3, 7), 2, None))], None)], [], None, None)], []),
('Module', [('AsyncFunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('AsyncWith', (2, 1), [('withitem', ('Name', (2, 12), 'a', ('Load',)), ('Name', (2, 17), 'b', ('Store',)))], [('Expr', (2, 20), ('Constant', (2, 20), 1, None))], None)], [], None, None)], []),
('Module', [('Expr', (1, 0), ('Dict', (1, 0), [None, ('Constant', (1, 10), 2, None)], [('Dict', (1, 3), [('Constant', (1, 4), 1, None)], [('Constant', (1, 6), 2, None)]), ('Constant', (1, 12), 3, None)]))], []),
('Module', [('Expr', (1, 0), ('Set', (1, 0), [('Starred', (1, 1), ('Set', (1, 2), [('Constant', (1, 3), 1, None), ('Constant', (1, 6), 2, None)]), ('Load',)), ('Constant', (1, 10), 3, None)]))], []),
('Module', [('AsyncFunctionDef', (1, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Expr', (2, 1), ('ListComp', (2, 1), ('Name', (2, 2), 'i', ('Load',)), [('comprehension', ('Name', (2, 14), 'b', ('Store',)), ('Name', (2, 19), 'c', ('Load',)), [], 1)]))], [], None, None)], []),
('Module', [('FunctionDef', (4, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (4, 9))], [('Name', (1, 1), 'deco1', ('Load',)), ('Call', (2, 1), ('Name', (2, 1), 'deco2', ('Load',)), [], []), ('Call', (3, 1), ('Name', (3, 1), 'deco3', ('Load',)), [('Constant', (3, 7), 1, None)], [])], None, None)], []),
('Module', [('AsyncFunctionDef', (4, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (4, 15))], [('Name', (1, 1), 'deco1', ('Load',)), ('Call', (2, 1), ('Name', (2, 1), 'deco2', ('Load',)), [], []), ('Call', (3, 1), ('Name', (3, 1), 'deco3', ('Load',)), [('Constant', (3, 7), 1, None)], [])], None, None)], []),
('Module', [('ClassDef', (4, 0), 'C', [], [], [('Pass', (4, 9))], [('Name', (1, 1), 'deco1', ('Load',)), ('Call', (2, 1), ('Name', (2, 1), 'deco2', ('Load',)), [], []), ('Call', (3, 1), ('Name', (3, 1), 'deco3', ('Load',)), [('Constant', (3, 7), 1, None)], [])])], []),
('Module', [('FunctionDef', (2, 0), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (2, 9))], [('Call', (1, 1), ('Name', (1, 1), 'deco', ('Load',)), [('GeneratorExp', (1, 5), ('Name', (1, 6), 'a', ('Load',)), [('comprehension', ('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 17), 'b', ('Load',)), [], 0)])], [])], None, None)], []),
('Module', [('Expr', (1, 0), ('NamedExpr', (1, 1), ('Name', (1, 1), 'a', ('Store',)), ('Constant', (1, 6), 1, None)))], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [], None, [], [], None, []), [('Pass', (1, 14))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 12), 'c', None, None), ('arg', (1, 15), 'd', None, None), ('arg', (1, 18), 'e', None, None)], None, [], [], None, []), [('Pass', (1, 22))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 12), 'c', None, None)], None, [('arg', (1, 18), 'd', None, None), ('arg', (1, 21), 'e', None, None)], [None, None], None, []), [('Pass', (1, 25))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 12), 'c', None, None)], None, [('arg', (1, 18), 'd', None, None), ('arg', (1, 21), 'e', None, None)], [None, None], ('arg', (1, 26), 'kwargs', None, None), []), [('Pass', (1, 35))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [], None, [], [], None, [('Constant', (1, 8), 1, None)]), [('Pass', (1, 16))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 14), 'b', None, None), ('arg', (1, 19), 'c', None, None)], None, [], [], None, [('Constant', (1, 8), 1, None), ('Constant', (1, 16), 2, None), ('Constant', (1, 21), 4, None)]), [('Pass', (1, 25))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 14), 'b', None, None)], None, [('arg', (1, 22), 'c', None, None)], [('Constant', (1, 24), 4, None)], None, [('Constant', (1, 8), 1, None), ('Constant', (1, 16), 2, None)]), [('Pass', (1, 28))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 14), 'b', None, None)], None, [('arg', (1, 22), 'c', None, None)], [None], None, [('Constant', (1, 8), 1, None), ('Constant', (1, 16), 2, None)]), [('Pass', (1, 26))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 14), 'b', None, None)], None, [('arg', (1, 22), 'c', None, None)], [('Constant', (1, 24), 4, None)], ('arg', (1, 29), 'kwargs', None, None), [('Constant', (1, 8), 1, None), ('Constant', (1, 16), 2, None)]), [('Pass', (1, 38))], [], None, None)], []),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [('arg', (1, 6), 'a', None, None)], [('arg', (1, 14), 'b', None, None)], None, [('arg', (1, 22), 'c', None, None)], [None], ('arg', (1, 27), 'kwargs', None, None), [('Constant', (1, 8), 1, None), ('Constant', (1, 16), 2, None)]), [('Pass', (1, 36))], [], None, None)], []),
]
single_results = [
('Interactive', [('Expr', (1, 0), ('BinOp', (1, 0), ('Constant', (1, 0), 1, None), ('Add',), ('Constant', (1, 2), 2, None)))]),
]
eval_results = [
('Expression', ('Constant', (1, 0), None, None)),
('Expression', ('BoolOp', (1, 0), ('And',), [('Name', (1, 0), 'a', ('Load',)), ('Name', (1, 6), 'b', ('Load',))])),
('Expression', ('BinOp', (1, 0), ('Name', (1, 0), 'a', ('Load',)), ('Add',), ('Name', (1, 4), 'b', ('Load',)))),
('Expression', ('UnaryOp', (1, 0), ('Not',), ('Name', (1, 4), 'v', ('Load',)))),
('Expression', ('Lambda', (1, 0), ('arguments', [], [], None, [], [], None, []), ('Constant', (1, 7), None, None))),
('Expression', ('Dict', (1, 0), [('Constant', (1, 2), 1, None)], [('Constant', (1, 4), 2, None)])),
('Expression', ('Dict', (1, 0), [], [])),
('Expression', ('Set', (1, 0), [('Constant', (1, 1), None, None)])),
('Expression', ('Dict', (1, 0), [('Constant', (2, 6), 1, None)], [('Constant', (4, 10), 2, None)])),
('Expression', ('ListComp', (1, 0), ('Name', (1, 1), 'a', ('Load',)), [('comprehension', ('Name', (1, 7), 'b', ('Store',)), ('Name', (1, 12), 'c', ('Load',)), [('Name', (1, 17), 'd', ('Load',))], 0)])),
('Expression', ('GeneratorExp', (1, 0), ('Name', (1, 1), 'a', ('Load',)), [('comprehension', ('Name', (1, 7), 'b', ('Store',)), ('Name', (1, 12), 'c', ('Load',)), [('Name', (1, 17), 'd', ('Load',))], 0)])),
('Expression', ('ListComp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 11), 'a', ('Store',)), ('Name', (1, 13), 'b', ('Store',))], ('Store',)), ('Name', (1, 18), 'c', ('Load',)), [], 0)])),
('Expression', ('ListComp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 20), 'c', ('Load',)), [], 0)])),
('Expression', ('ListComp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('List', (1, 11), [('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 20), 'c', ('Load',)), [], 0)])),
('Expression', ('SetComp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 11), 'a', ('Store',)), ('Name', (1, 13), 'b', ('Store',))], ('Store',)), ('Name', (1, 18), 'c', ('Load',)), [], 0)])),
('Expression', ('SetComp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 20), 'c', ('Load',)), [], 0)])),
('Expression', ('SetComp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('List', (1, 11), [('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 20), 'c', ('Load',)), [], 0)])),
('Expression', ('GeneratorExp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 11), 'a', ('Store',)), ('Name', (1, 13), 'b', ('Store',))], ('Store',)), ('Name', (1, 18), 'c', ('Load',)), [], 0)])),
('Expression', ('GeneratorExp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11), [('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 20), 'c', ('Load',)), [], 0)])),
('Expression', ('GeneratorExp', (1, 0), ('Tuple', (1, 1), [('Name', (1, 2), 'a', ('Load',)), ('Name', (1, 4), 'b', ('Load',))], ('Load',)), [('comprehension', ('List', (1, 11), [('Name', (1, 12), 'a', ('Store',)), ('Name', (1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 20), 'c', ('Load',)), [], 0)])),
('Expression', ('Compare', (1, 0), ('Constant', (1, 0), 1, None), [('Lt',), ('Lt',)], [('Constant', (1, 4), 2, None), ('Constant', (1, 8), 3, None)])),
('Expression', ('Call', (1, 0), ('Name', (1, 0), 'f', ('Load',)), [('Constant', (1, 2), 1, None), ('Constant', (1, 4), 2, None), ('Starred', (1, 10), ('Name', (1, 11), 'd', ('Load',)), ('Load',))], [('keyword', 'c', ('Constant', (1, 8), 3, None)), ('keyword', None, ('Name', (1, 15), 'e', ('Load',)))])),
('Expression', ('Call', (1, 0), ('Name', (1, 0), 'f', ('Load',)), [('Starred', (1, 2), ('List', (1, 3), [('Constant', (1, 4), 0, None), ('Constant', (1, 7), 1, None)], ('Load',)), ('Load',))], [])),
('Expression', ('Call', (1, 0), ('Name', (1, 0), 'f', ('Load',)), [('GeneratorExp', (1, 1), ('Name', (1, 2), 'a', ('Load',)), [('comprehension', ('Name', (1, 8), 'a', ('Store',)), ('Name', (1, 13), 'b', ('Load',)), [], 0)])], [])),
('Expression', ('Constant', (1, 0), 10, None)),
('Expression', ('Constant', (1, 0), 'string', None)),
('Expression', ('Attribute', (1, 0), ('Name', (1, 0), 'a', ('Load',)), 'b', ('Load',))),
('Expression', ('Subscript', (1, 0), ('Name', (1, 0), 'a', ('Load',)), ('Slice', ('Name', (1, 2), 'b', ('Load',)), ('Name', (1, 4), 'c', ('Load',)), None), ('Load',))),
('Expression', ('Name', (1, 0), 'v', ('Load',))),
('Expression', ('List', (1, 0), [('Constant', (1, 1), 1, None), ('Constant', (1, 3), 2, None), ('Constant', (1, 5), 3, None)], ('Load',))),
('Expression', ('List', (1, 0), [], ('Load',))),
('Expression', ('Tuple', (1, 0), [('Constant', (1, 0), 1, None), ('Constant', (1, 2), 2, None), ('Constant', (1, 4), 3, None)], ('Load',))),
('Expression', ('Tuple', (1, 0), [('Constant', (1, 1), 1, None), ('Constant', (1, 3), 2, None), ('Constant', (1, 5), 3, None)], ('Load',))),
('Expression', ('Tuple', (1, 0), [], ('Load',))),
('Expression', ('Call', (1, 0), ('Attribute', (1, 0), ('Attribute', (1, 0), ('Attribute', (1, 0), ('Name', (1, 0), 'a', ('Load',)), 'b', ('Load',)), 'c', ('Load',)), 'd', ('Load',)), [('Subscript', (1, 8), ('Attribute', (1, 8), ('Name', (1, 8), 'a', ('Load',)), 'b', ('Load',)), ('Slice', ('Constant', (1, 12), 1, None), ('Constant', (1, 14), 2, None), None), ('Load',))], [])),
]
main()
