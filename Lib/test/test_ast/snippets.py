import ast
import sys

from test.test_ast.utils import to_tuple


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
    # FunctionDef with varargs as TypeVarTuple
    "def f(*args: *Ts): pass",
    # FunctionDef with varargs as unpacked Tuple
    "def f(*args: *tuple[int, ...]): pass",
    # FunctionDef with varargs as unpacked Tuple *and* TypeVarTuple
    "def f(*args: *tuple[int, *Ts]): pass",
    # FunctionDef with kwargs
    "def f(**kwargs): pass",
    # FunctionDef with all kind of args and docstring
    "def f(a, b=1, c=None, d=[], e={}, *args, f=42, **kwargs): 'doc for f()'",
    # FunctionDef with type annotation on return involving unpacking
    "def f() -> tuple[*Ts]: pass",
    "def f() -> tuple[int, *Ts]: pass",
    "def f() -> tuple[int, *tuple[int, ...]]: pass",
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
    # AnnAssign with unpacked types
    "x: tuple[*Ts]",
    "x: tuple[int, *Ts]",
    "x: tuple[int, *tuple[str, ...]]",
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
    # TryStarExcept
    "try:\n  pass\nexcept* Exception:\n  pass",
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
    # Decorator with attribute
    "@a.b.c\ndef f(): pass",
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
    # Type aliases
    "type X = int",
    "type X[T] = int",
    "type X[T, *Ts, **P] = (T, Ts, P)",
    "type X[T: int, *Ts, **P] = (T, Ts, P)",
    "type X[T: (int, str), *Ts, **P] = (T, Ts, P)",
    # Generic classes
    "class X[T]: pass",
    "class X[T, *Ts, **P]: pass",
    "class X[T: int, *Ts, **P]: pass",
    "class X[T: (int, str), *Ts, **P]: pass",
    # Generic functions
    "def f[T](): pass",
    "def f[T, *Ts, **P](): pass",
    "def f[T: int, *Ts, **P](): pass",
    "def f[T: (int, str), *Ts, **P](): pass",
]

# These are compiled through "single"
# because of overlap with "eval", it just tests what
# can't be tested with "eval"
single_tests = ["1+2"]

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

#### EVERYTHING BELOW IS GENERATED BY python Lib/test/test_ast/snippets.py -g  #####
exec_results = [
('Module', [('Expr', (1, 0, 1, 4), ('Constant', (1, 0, 1, 4), None, None))], []),
('Module', [('Expr', (1, 0, 1, 18), ('Constant', (1, 0, 1, 18), 'module docstring', None))], []),
('Module', [('FunctionDef', (1, 0, 1, 13), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 9, 1, 13))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 29), 'f', ('arguments', [], [], None, [], [], None, []), [('Expr', (1, 9, 1, 29), ('Constant', (1, 9, 1, 29), 'function docstring', None))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 14), 'f', ('arguments', [], [('arg', (1, 6, 1, 7), 'a', None, None)], None, [], [], None, []), [('Pass', (1, 10, 1, 14))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 16), 'f', ('arguments', [], [('arg', (1, 6, 1, 7), 'a', None, None)], None, [], [], None, [('Constant', (1, 8, 1, 9), 0, None)]), [('Pass', (1, 12, 1, 16))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 18), 'f', ('arguments', [], [], ('arg', (1, 7, 1, 11), 'args', None, None), [], [], None, []), [('Pass', (1, 14, 1, 18))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 23), 'f', ('arguments', [], [], ('arg', (1, 7, 1, 16), 'args', ('Starred', (1, 13, 1, 16), ('Name', (1, 14, 1, 16), 'Ts', ('Load',)), ('Load',)), None), [], [], None, []), [('Pass', (1, 19, 1, 23))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 36), 'f', ('arguments', [], [], ('arg', (1, 7, 1, 29), 'args', ('Starred', (1, 13, 1, 29), ('Subscript', (1, 14, 1, 29), ('Name', (1, 14, 1, 19), 'tuple', ('Load',)), ('Tuple', (1, 20, 1, 28), [('Name', (1, 20, 1, 23), 'int', ('Load',)), ('Constant', (1, 25, 1, 28), Ellipsis, None)], ('Load',)), ('Load',)), ('Load',)), None), [], [], None, []), [('Pass', (1, 32, 1, 36))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 36), 'f', ('arguments', [], [], ('arg', (1, 7, 1, 29), 'args', ('Starred', (1, 13, 1, 29), ('Subscript', (1, 14, 1, 29), ('Name', (1, 14, 1, 19), 'tuple', ('Load',)), ('Tuple', (1, 20, 1, 28), [('Name', (1, 20, 1, 23), 'int', ('Load',)), ('Starred', (1, 25, 1, 28), ('Name', (1, 26, 1, 28), 'Ts', ('Load',)), ('Load',))], ('Load',)), ('Load',)), ('Load',)), None), [], [], None, []), [('Pass', (1, 32, 1, 36))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 21), 'f', ('arguments', [], [], None, [], [], ('arg', (1, 8, 1, 14), 'kwargs', None, None), []), [('Pass', (1, 17, 1, 21))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 71), 'f', ('arguments', [], [('arg', (1, 6, 1, 7), 'a', None, None), ('arg', (1, 9, 1, 10), 'b', None, None), ('arg', (1, 14, 1, 15), 'c', None, None), ('arg', (1, 22, 1, 23), 'd', None, None), ('arg', (1, 28, 1, 29), 'e', None, None)], ('arg', (1, 35, 1, 39), 'args', None, None), [('arg', (1, 41, 1, 42), 'f', None, None)], [('Constant', (1, 43, 1, 45), 42, None)], ('arg', (1, 49, 1, 55), 'kwargs', None, None), [('Constant', (1, 11, 1, 12), 1, None), ('Constant', (1, 16, 1, 20), None, None), ('List', (1, 24, 1, 26), [], ('Load',)), ('Dict', (1, 30, 1, 32), [], [])]), [('Expr', (1, 58, 1, 71), ('Constant', (1, 58, 1, 71), 'doc for f()', None))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 27), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 23, 1, 27))], [], ('Subscript', (1, 11, 1, 21), ('Name', (1, 11, 1, 16), 'tuple', ('Load',)), ('Tuple', (1, 17, 1, 20), [('Starred', (1, 17, 1, 20), ('Name', (1, 18, 1, 20), 'Ts', ('Load',)), ('Load',))], ('Load',)), ('Load',)), None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 32), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 28, 1, 32))], [], ('Subscript', (1, 11, 1, 26), ('Name', (1, 11, 1, 16), 'tuple', ('Load',)), ('Tuple', (1, 17, 1, 25), [('Name', (1, 17, 1, 20), 'int', ('Load',)), ('Starred', (1, 22, 1, 25), ('Name', (1, 23, 1, 25), 'Ts', ('Load',)), ('Load',))], ('Load',)), ('Load',)), None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 45), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 41, 1, 45))], [], ('Subscript', (1, 11, 1, 39), ('Name', (1, 11, 1, 16), 'tuple', ('Load',)), ('Tuple', (1, 17, 1, 38), [('Name', (1, 17, 1, 20), 'int', ('Load',)), ('Starred', (1, 22, 1, 38), ('Subscript', (1, 23, 1, 38), ('Name', (1, 23, 1, 28), 'tuple', ('Load',)), ('Tuple', (1, 29, 1, 37), [('Name', (1, 29, 1, 32), 'int', ('Load',)), ('Constant', (1, 34, 1, 37), Ellipsis, None)], ('Load',)), ('Load',)), ('Load',))], ('Load',)), ('Load',)), None, [])], []),
('Module', [('ClassDef', (1, 0, 1, 12), 'C', [], [], [('Pass', (1, 8, 1, 12))], [], [])], []),
('Module', [('ClassDef', (1, 0, 1, 32), 'C', [], [], [('Expr', (1, 9, 1, 32), ('Constant', (1, 9, 1, 32), 'docstring for class C', None))], [], [])], []),
('Module', [('ClassDef', (1, 0, 1, 21), 'C', [('Name', (1, 8, 1, 14), 'object', ('Load',))], [], [('Pass', (1, 17, 1, 21))], [], [])], []),
('Module', [('FunctionDef', (1, 0, 1, 16), 'f', ('arguments', [], [], None, [], [], None, []), [('Return', (1, 8, 1, 16), ('Constant', (1, 15, 1, 16), 1, None))], [], None, None, [])], []),
('Module', [('Delete', (1, 0, 1, 5), [('Name', (1, 4, 1, 5), 'v', ('Del',))])], []),
('Module', [('Assign', (1, 0, 1, 5), [('Name', (1, 0, 1, 1), 'v', ('Store',))], ('Constant', (1, 4, 1, 5), 1, None), None)], []),
('Module', [('Assign', (1, 0, 1, 7), [('Tuple', (1, 0, 1, 3), [('Name', (1, 0, 1, 1), 'a', ('Store',)), ('Name', (1, 2, 1, 3), 'b', ('Store',))], ('Store',))], ('Name', (1, 6, 1, 7), 'c', ('Load',)), None)], []),
('Module', [('Assign', (1, 0, 1, 9), [('Tuple', (1, 0, 1, 5), [('Name', (1, 1, 1, 2), 'a', ('Store',)), ('Name', (1, 3, 1, 4), 'b', ('Store',))], ('Store',))], ('Name', (1, 8, 1, 9), 'c', ('Load',)), None)], []),
('Module', [('Assign', (1, 0, 1, 9), [('List', (1, 0, 1, 5), [('Name', (1, 1, 1, 2), 'a', ('Store',)), ('Name', (1, 3, 1, 4), 'b', ('Store',))], ('Store',))], ('Name', (1, 8, 1, 9), 'c', ('Load',)), None)], []),
('Module', [('AnnAssign', (1, 0, 1, 13), ('Name', (1, 0, 1, 1), 'x', ('Store',)), ('Subscript', (1, 3, 1, 13), ('Name', (1, 3, 1, 8), 'tuple', ('Load',)), ('Tuple', (1, 9, 1, 12), [('Starred', (1, 9, 1, 12), ('Name', (1, 10, 1, 12), 'Ts', ('Load',)), ('Load',))], ('Load',)), ('Load',)), None, 1)], []),
('Module', [('AnnAssign', (1, 0, 1, 18), ('Name', (1, 0, 1, 1), 'x', ('Store',)), ('Subscript', (1, 3, 1, 18), ('Name', (1, 3, 1, 8), 'tuple', ('Load',)), ('Tuple', (1, 9, 1, 17), [('Name', (1, 9, 1, 12), 'int', ('Load',)), ('Starred', (1, 14, 1, 17), ('Name', (1, 15, 1, 17), 'Ts', ('Load',)), ('Load',))], ('Load',)), ('Load',)), None, 1)], []),
('Module', [('AnnAssign', (1, 0, 1, 31), ('Name', (1, 0, 1, 1), 'x', ('Store',)), ('Subscript', (1, 3, 1, 31), ('Name', (1, 3, 1, 8), 'tuple', ('Load',)), ('Tuple', (1, 9, 1, 30), [('Name', (1, 9, 1, 12), 'int', ('Load',)), ('Starred', (1, 14, 1, 30), ('Subscript', (1, 15, 1, 30), ('Name', (1, 15, 1, 20), 'tuple', ('Load',)), ('Tuple', (1, 21, 1, 29), [('Name', (1, 21, 1, 24), 'str', ('Load',)), ('Constant', (1, 26, 1, 29), Ellipsis, None)], ('Load',)), ('Load',)), ('Load',))], ('Load',)), ('Load',)), None, 1)], []),
('Module', [('AugAssign', (1, 0, 1, 6), ('Name', (1, 0, 1, 1), 'v', ('Store',)), ('Add',), ('Constant', (1, 5, 1, 6), 1, None))], []),
('Module', [('For', (1, 0, 1, 15), ('Name', (1, 4, 1, 5), 'v', ('Store',)), ('Name', (1, 9, 1, 10), 'v', ('Load',)), [('Pass', (1, 11, 1, 15))], [], None)], []),
('Module', [('While', (1, 0, 1, 12), ('Name', (1, 6, 1, 7), 'v', ('Load',)), [('Pass', (1, 8, 1, 12))], [])], []),
('Module', [('If', (1, 0, 1, 9), ('Name', (1, 3, 1, 4), 'v', ('Load',)), [('Pass', (1, 5, 1, 9))], [])], []),
('Module', [('If', (1, 0, 4, 6), ('Name', (1, 3, 1, 4), 'a', ('Load',)), [('Pass', (2, 2, 2, 6))], [('If', (3, 0, 4, 6), ('Name', (3, 5, 3, 6), 'b', ('Load',)), [('Pass', (4, 2, 4, 6))], [])])], []),
('Module', [('If', (1, 0, 6, 6), ('Name', (1, 3, 1, 4), 'a', ('Load',)), [('Pass', (2, 2, 2, 6))], [('If', (3, 0, 6, 6), ('Name', (3, 5, 3, 6), 'b', ('Load',)), [('Pass', (4, 2, 4, 6))], [('Pass', (6, 2, 6, 6))])])], []),
('Module', [('With', (1, 0, 1, 17), [('withitem', ('Name', (1, 5, 1, 6), 'x', ('Load',)), ('Name', (1, 10, 1, 11), 'y', ('Store',)))], [('Pass', (1, 13, 1, 17))], None)], []),
('Module', [('With', (1, 0, 1, 25), [('withitem', ('Name', (1, 5, 1, 6), 'x', ('Load',)), ('Name', (1, 10, 1, 11), 'y', ('Store',))), ('withitem', ('Name', (1, 13, 1, 14), 'z', ('Load',)), ('Name', (1, 18, 1, 19), 'q', ('Store',)))], [('Pass', (1, 21, 1, 25))], None)], []),
('Module', [('Raise', (1, 0, 1, 25), ('Call', (1, 6, 1, 25), ('Name', (1, 6, 1, 15), 'Exception', ('Load',)), [('Constant', (1, 16, 1, 24), 'string', None)], []), None)], []),
('Module', [('Try', (1, 0, 4, 6), [('Pass', (2, 2, 2, 6))], [('ExceptHandler', (3, 0, 4, 6), ('Name', (3, 7, 3, 16), 'Exception', ('Load',)), None, [('Pass', (4, 2, 4, 6))])], [], [])], []),
('Module', [('Try', (1, 0, 4, 6), [('Pass', (2, 2, 2, 6))], [], [], [('Pass', (4, 2, 4, 6))])], []),
('Module', [('TryStar', (1, 0, 4, 6), [('Pass', (2, 2, 2, 6))], [('ExceptHandler', (3, 0, 4, 6), ('Name', (3, 8, 3, 17), 'Exception', ('Load',)), None, [('Pass', (4, 2, 4, 6))])], [], [])], []),
('Module', [('Assert', (1, 0, 1, 8), ('Name', (1, 7, 1, 8), 'v', ('Load',)), None)], []),
('Module', [('Import', (1, 0, 1, 10), [('alias', (1, 7, 1, 10), 'sys', None)])], []),
('Module', [('ImportFrom', (1, 0, 1, 17), 'sys', [('alias', (1, 16, 1, 17), 'v', None)], 0)], []),
('Module', [('Global', (1, 0, 1, 8), ['v'])], []),
('Module', [('Expr', (1, 0, 1, 1), ('Constant', (1, 0, 1, 1), 1, None))], []),
('Module', [('Pass', (1, 0, 1, 4))], []),
('Module', [('For', (1, 0, 1, 16), ('Name', (1, 4, 1, 5), 'v', ('Store',)), ('Name', (1, 9, 1, 10), 'v', ('Load',)), [('Break', (1, 11, 1, 16))], [], None)], []),
('Module', [('For', (1, 0, 1, 19), ('Name', (1, 4, 1, 5), 'v', ('Store',)), ('Name', (1, 9, 1, 10), 'v', ('Load',)), [('Continue', (1, 11, 1, 19))], [], None)], []),
('Module', [('For', (1, 0, 1, 18), ('Tuple', (1, 4, 1, 7), [('Name', (1, 4, 1, 5), 'a', ('Store',)), ('Name', (1, 6, 1, 7), 'b', ('Store',))], ('Store',)), ('Name', (1, 11, 1, 12), 'c', ('Load',)), [('Pass', (1, 14, 1, 18))], [], None)], []),
('Module', [('For', (1, 0, 1, 20), ('Tuple', (1, 4, 1, 9), [('Name', (1, 5, 1, 6), 'a', ('Store',)), ('Name', (1, 7, 1, 8), 'b', ('Store',))], ('Store',)), ('Name', (1, 13, 1, 14), 'c', ('Load',)), [('Pass', (1, 16, 1, 20))], [], None)], []),
('Module', [('For', (1, 0, 1, 20), ('List', (1, 4, 1, 9), [('Name', (1, 5, 1, 6), 'a', ('Store',)), ('Name', (1, 7, 1, 8), 'b', ('Store',))], ('Store',)), ('Name', (1, 13, 1, 14), 'c', ('Load',)), [('Pass', (1, 16, 1, 20))], [], None)], []),
('Module', [('Expr', (1, 0, 11, 5), ('GeneratorExp', (1, 0, 11, 5), ('Tuple', (2, 4, 6, 5), [('Name', (3, 4, 3, 6), 'Aa', ('Load',)), ('Name', (5, 7, 5, 9), 'Bb', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (8, 4, 10, 6), [('Name', (8, 4, 8, 6), 'Aa', ('Store',)), ('Name', (10, 4, 10, 6), 'Bb', ('Store',))], ('Store',)), ('Name', (10, 10, 10, 12), 'Cc', ('Load',)), [], 0)]))], []),
('Module', [('Expr', (1, 0, 1, 34), ('DictComp', (1, 0, 1, 34), ('Name', (1, 1, 1, 2), 'a', ('Load',)), ('Name', (1, 5, 1, 6), 'b', ('Load',)), [('comprehension', ('Name', (1, 11, 1, 12), 'w', ('Store',)), ('Name', (1, 16, 1, 17), 'x', ('Load',)), [], 0), ('comprehension', ('Name', (1, 22, 1, 23), 'm', ('Store',)), ('Name', (1, 27, 1, 28), 'p', ('Load',)), [('Name', (1, 32, 1, 33), 'g', ('Load',))], 0)]))], []),
('Module', [('Expr', (1, 0, 1, 20), ('DictComp', (1, 0, 1, 20), ('Name', (1, 1, 1, 2), 'a', ('Load',)), ('Name', (1, 5, 1, 6), 'b', ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 14), [('Name', (1, 11, 1, 12), 'v', ('Store',)), ('Name', (1, 13, 1, 14), 'w', ('Store',))], ('Store',)), ('Name', (1, 18, 1, 19), 'x', ('Load',)), [], 0)]))], []),
('Module', [('Expr', (1, 0, 1, 19), ('SetComp', (1, 0, 1, 19), ('Name', (1, 1, 1, 2), 'r', ('Load',)), [('comprehension', ('Name', (1, 7, 1, 8), 'l', ('Store',)), ('Name', (1, 12, 1, 13), 'x', ('Load',)), [('Name', (1, 17, 1, 18), 'g', ('Load',))], 0)]))], []),
('Module', [('Expr', (1, 0, 1, 16), ('SetComp', (1, 0, 1, 16), ('Name', (1, 1, 1, 2), 'r', ('Load',)), [('comprehension', ('Tuple', (1, 7, 1, 10), [('Name', (1, 7, 1, 8), 'l', ('Store',)), ('Name', (1, 9, 1, 10), 'm', ('Store',))], ('Store',)), ('Name', (1, 14, 1, 15), 'x', ('Load',)), [], 0)]))], []),
('Module', [('AsyncFunctionDef', (1, 0, 3, 18), 'f', ('arguments', [], [], None, [], [], None, []), [('Expr', (2, 1, 2, 17), ('Constant', (2, 1, 2, 17), 'async function', None)), ('Expr', (3, 1, 3, 18), ('Await', (3, 1, 3, 18), ('Call', (3, 7, 3, 18), ('Name', (3, 7, 3, 16), 'something', ('Load',)), [], [])))], [], None, None, [])], []),
('Module', [('AsyncFunctionDef', (1, 0, 3, 8), 'f', ('arguments', [], [], None, [], [], None, []), [('AsyncFor', (2, 1, 3, 8), ('Name', (2, 11, 2, 12), 'e', ('Store',)), ('Name', (2, 16, 2, 17), 'i', ('Load',)), [('Expr', (2, 19, 2, 20), ('Constant', (2, 19, 2, 20), 1, None))], [('Expr', (3, 7, 3, 8), ('Constant', (3, 7, 3, 8), 2, None))], None)], [], None, None, [])], []),
('Module', [('AsyncFunctionDef', (1, 0, 2, 21), 'f', ('arguments', [], [], None, [], [], None, []), [('AsyncWith', (2, 1, 2, 21), [('withitem', ('Name', (2, 12, 2, 13), 'a', ('Load',)), ('Name', (2, 17, 2, 18), 'b', ('Store',)))], [('Expr', (2, 20, 2, 21), ('Constant', (2, 20, 2, 21), 1, None))], None)], [], None, None, [])], []),
('Module', [('Expr', (1, 0, 1, 14), ('Dict', (1, 0, 1, 14), [None, ('Constant', (1, 10, 1, 11), 2, None)], [('Dict', (1, 3, 1, 8), [('Constant', (1, 4, 1, 5), 1, None)], [('Constant', (1, 6, 1, 7), 2, None)]), ('Constant', (1, 12, 1, 13), 3, None)]))], []),
('Module', [('Expr', (1, 0, 1, 12), ('Set', (1, 0, 1, 12), [('Starred', (1, 1, 1, 8), ('Set', (1, 2, 1, 8), [('Constant', (1, 3, 1, 4), 1, None), ('Constant', (1, 6, 1, 7), 2, None)]), ('Load',)), ('Constant', (1, 10, 1, 11), 3, None)]))], []),
('Module', [('AsyncFunctionDef', (1, 0, 2, 21), 'f', ('arguments', [], [], None, [], [], None, []), [('Expr', (2, 1, 2, 21), ('ListComp', (2, 1, 2, 21), ('Name', (2, 2, 2, 3), 'i', ('Load',)), [('comprehension', ('Name', (2, 14, 2, 15), 'b', ('Store',)), ('Name', (2, 19, 2, 20), 'c', ('Load',)), [], 1)]))], [], None, None, [])], []),
('Module', [('FunctionDef', (4, 0, 4, 13), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (4, 9, 4, 13))], [('Name', (1, 1, 1, 6), 'deco1', ('Load',)), ('Call', (2, 1, 2, 8), ('Name', (2, 1, 2, 6), 'deco2', ('Load',)), [], []), ('Call', (3, 1, 3, 9), ('Name', (3, 1, 3, 6), 'deco3', ('Load',)), [('Constant', (3, 7, 3, 8), 1, None)], [])], None, None, [])], []),
('Module', [('AsyncFunctionDef', (4, 0, 4, 19), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (4, 15, 4, 19))], [('Name', (1, 1, 1, 6), 'deco1', ('Load',)), ('Call', (2, 1, 2, 8), ('Name', (2, 1, 2, 6), 'deco2', ('Load',)), [], []), ('Call', (3, 1, 3, 9), ('Name', (3, 1, 3, 6), 'deco3', ('Load',)), [('Constant', (3, 7, 3, 8), 1, None)], [])], None, None, [])], []),
('Module', [('ClassDef', (4, 0, 4, 13), 'C', [], [], [('Pass', (4, 9, 4, 13))], [('Name', (1, 1, 1, 6), 'deco1', ('Load',)), ('Call', (2, 1, 2, 8), ('Name', (2, 1, 2, 6), 'deco2', ('Load',)), [], []), ('Call', (3, 1, 3, 9), ('Name', (3, 1, 3, 6), 'deco3', ('Load',)), [('Constant', (3, 7, 3, 8), 1, None)], [])], [])], []),
('Module', [('FunctionDef', (2, 0, 2, 13), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (2, 9, 2, 13))], [('Call', (1, 1, 1, 19), ('Name', (1, 1, 1, 5), 'deco', ('Load',)), [('GeneratorExp', (1, 5, 1, 19), ('Name', (1, 6, 1, 7), 'a', ('Load',)), [('comprehension', ('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 17, 1, 18), 'b', ('Load',)), [], 0)])], [])], None, None, [])], []),
('Module', [('FunctionDef', (2, 0, 2, 13), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (2, 9, 2, 13))], [('Attribute', (1, 1, 1, 6), ('Attribute', (1, 1, 1, 4), ('Name', (1, 1, 1, 2), 'a', ('Load',)), 'b', ('Load',)), 'c', ('Load',))], None, None, [])], []),
('Module', [('Expr', (1, 0, 1, 8), ('NamedExpr', (1, 1, 1, 7), ('Name', (1, 1, 1, 2), 'a', ('Store',)), ('Constant', (1, 6, 1, 7), 1, None)))], []),
('Module', [('FunctionDef', (1, 0, 1, 18), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [], None, [], [], None, []), [('Pass', (1, 14, 1, 18))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 26), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 12, 1, 13), 'c', None, None), ('arg', (1, 15, 1, 16), 'd', None, None), ('arg', (1, 18, 1, 19), 'e', None, None)], None, [], [], None, []), [('Pass', (1, 22, 1, 26))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 29), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 12, 1, 13), 'c', None, None)], None, [('arg', (1, 18, 1, 19), 'd', None, None), ('arg', (1, 21, 1, 22), 'e', None, None)], [None, None], None, []), [('Pass', (1, 25, 1, 29))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 39), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 12, 1, 13), 'c', None, None)], None, [('arg', (1, 18, 1, 19), 'd', None, None), ('arg', (1, 21, 1, 22), 'e', None, None)], [None, None], ('arg', (1, 26, 1, 32), 'kwargs', None, None), []), [('Pass', (1, 35, 1, 39))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 20), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [], None, [], [], None, [('Constant', (1, 8, 1, 9), 1, None)]), [('Pass', (1, 16, 1, 20))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 29), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 14, 1, 15), 'b', None, None), ('arg', (1, 19, 1, 20), 'c', None, None)], None, [], [], None, [('Constant', (1, 8, 1, 9), 1, None), ('Constant', (1, 16, 1, 17), 2, None), ('Constant', (1, 21, 1, 22), 4, None)]), [('Pass', (1, 25, 1, 29))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 32), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 14, 1, 15), 'b', None, None)], None, [('arg', (1, 22, 1, 23), 'c', None, None)], [('Constant', (1, 24, 1, 25), 4, None)], None, [('Constant', (1, 8, 1, 9), 1, None), ('Constant', (1, 16, 1, 17), 2, None)]), [('Pass', (1, 28, 1, 32))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 30), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 14, 1, 15), 'b', None, None)], None, [('arg', (1, 22, 1, 23), 'c', None, None)], [None], None, [('Constant', (1, 8, 1, 9), 1, None), ('Constant', (1, 16, 1, 17), 2, None)]), [('Pass', (1, 26, 1, 30))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 42), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 14, 1, 15), 'b', None, None)], None, [('arg', (1, 22, 1, 23), 'c', None, None)], [('Constant', (1, 24, 1, 25), 4, None)], ('arg', (1, 29, 1, 35), 'kwargs', None, None), [('Constant', (1, 8, 1, 9), 1, None), ('Constant', (1, 16, 1, 17), 2, None)]), [('Pass', (1, 38, 1, 42))], [], None, None, [])], []),
('Module', [('FunctionDef', (1, 0, 1, 40), 'f', ('arguments', [('arg', (1, 6, 1, 7), 'a', None, None)], [('arg', (1, 14, 1, 15), 'b', None, None)], None, [('arg', (1, 22, 1, 23), 'c', None, None)], [None], ('arg', (1, 27, 1, 33), 'kwargs', None, None), [('Constant', (1, 8, 1, 9), 1, None), ('Constant', (1, 16, 1, 17), 2, None)]), [('Pass', (1, 36, 1, 40))], [], None, None, [])], []),
('Module', [('TypeAlias', (1, 0, 1, 12), ('Name', (1, 5, 1, 6), 'X', ('Store',)), [], ('Name', (1, 9, 1, 12), 'int', ('Load',)))], []),
('Module', [('TypeAlias', (1, 0, 1, 15), ('Name', (1, 5, 1, 6), 'X', ('Store',)), [('TypeVar', (1, 7, 1, 8), 'T', None)], ('Name', (1, 12, 1, 15), 'int', ('Load',)))], []),
('Module', [('TypeAlias', (1, 0, 1, 32), ('Name', (1, 5, 1, 6), 'X', ('Store',)), [('TypeVar', (1, 7, 1, 8), 'T', None), ('TypeVarTuple', (1, 10, 1, 13), 'Ts'), ('ParamSpec', (1, 15, 1, 18), 'P')], ('Tuple', (1, 22, 1, 32), [('Name', (1, 23, 1, 24), 'T', ('Load',)), ('Name', (1, 26, 1, 28), 'Ts', ('Load',)), ('Name', (1, 30, 1, 31), 'P', ('Load',))], ('Load',)))], []),
('Module', [('TypeAlias', (1, 0, 1, 37), ('Name', (1, 5, 1, 6), 'X', ('Store',)), [('TypeVar', (1, 7, 1, 13), 'T', ('Name', (1, 10, 1, 13), 'int', ('Load',))), ('TypeVarTuple', (1, 15, 1, 18), 'Ts'), ('ParamSpec', (1, 20, 1, 23), 'P')], ('Tuple', (1, 27, 1, 37), [('Name', (1, 28, 1, 29), 'T', ('Load',)), ('Name', (1, 31, 1, 33), 'Ts', ('Load',)), ('Name', (1, 35, 1, 36), 'P', ('Load',))], ('Load',)))], []),
('Module', [('TypeAlias', (1, 0, 1, 44), ('Name', (1, 5, 1, 6), 'X', ('Store',)), [('TypeVar', (1, 7, 1, 20), 'T', ('Tuple', (1, 10, 1, 20), [('Name', (1, 11, 1, 14), 'int', ('Load',)), ('Name', (1, 16, 1, 19), 'str', ('Load',))], ('Load',))), ('TypeVarTuple', (1, 22, 1, 25), 'Ts'), ('ParamSpec', (1, 27, 1, 30), 'P')], ('Tuple', (1, 34, 1, 44), [('Name', (1, 35, 1, 36), 'T', ('Load',)), ('Name', (1, 38, 1, 40), 'Ts', ('Load',)), ('Name', (1, 42, 1, 43), 'P', ('Load',))], ('Load',)))], []),
('Module', [('ClassDef', (1, 0, 1, 16), 'X', [], [], [('Pass', (1, 12, 1, 16))], [], [('TypeVar', (1, 8, 1, 9), 'T', None)])], []),
('Module', [('ClassDef', (1, 0, 1, 26), 'X', [], [], [('Pass', (1, 22, 1, 26))], [], [('TypeVar', (1, 8, 1, 9), 'T', None), ('TypeVarTuple', (1, 11, 1, 14), 'Ts'), ('ParamSpec', (1, 16, 1, 19), 'P')])], []),
('Module', [('ClassDef', (1, 0, 1, 31), 'X', [], [], [('Pass', (1, 27, 1, 31))], [], [('TypeVar', (1, 8, 1, 14), 'T', ('Name', (1, 11, 1, 14), 'int', ('Load',))), ('TypeVarTuple', (1, 16, 1, 19), 'Ts'), ('ParamSpec', (1, 21, 1, 24), 'P')])], []),
('Module', [('ClassDef', (1, 0, 1, 38), 'X', [], [], [('Pass', (1, 34, 1, 38))], [], [('TypeVar', (1, 8, 1, 21), 'T', ('Tuple', (1, 11, 1, 21), [('Name', (1, 12, 1, 15), 'int', ('Load',)), ('Name', (1, 17, 1, 20), 'str', ('Load',))], ('Load',))), ('TypeVarTuple', (1, 23, 1, 26), 'Ts'), ('ParamSpec', (1, 28, 1, 31), 'P')])], []),
('Module', [('FunctionDef', (1, 0, 1, 16), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 12, 1, 16))], [], None, None, [('TypeVar', (1, 6, 1, 7), 'T', None)])], []),
('Module', [('FunctionDef', (1, 0, 1, 26), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 22, 1, 26))], [], None, None, [('TypeVar', (1, 6, 1, 7), 'T', None), ('TypeVarTuple', (1, 9, 1, 12), 'Ts'), ('ParamSpec', (1, 14, 1, 17), 'P')])], []),
('Module', [('FunctionDef', (1, 0, 1, 31), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 27, 1, 31))], [], None, None, [('TypeVar', (1, 6, 1, 12), 'T', ('Name', (1, 9, 1, 12), 'int', ('Load',))), ('TypeVarTuple', (1, 14, 1, 17), 'Ts'), ('ParamSpec', (1, 19, 1, 22), 'P')])], []),
('Module', [('FunctionDef', (1, 0, 1, 38), 'f', ('arguments', [], [], None, [], [], None, []), [('Pass', (1, 34, 1, 38))], [], None, None, [('TypeVar', (1, 6, 1, 19), 'T', ('Tuple', (1, 9, 1, 19), [('Name', (1, 10, 1, 13), 'int', ('Load',)), ('Name', (1, 15, 1, 18), 'str', ('Load',))], ('Load',))), ('TypeVarTuple', (1, 21, 1, 24), 'Ts'), ('ParamSpec', (1, 26, 1, 29), 'P')])], []),
]
single_results = [
('Interactive', [('Expr', (1, 0, 1, 3), ('BinOp', (1, 0, 1, 3), ('Constant', (1, 0, 1, 1), 1, None), ('Add',), ('Constant', (1, 2, 1, 3), 2, None)))]),
]
eval_results = [
('Expression', ('Constant', (1, 0, 1, 4), None, None)),
('Expression', ('BoolOp', (1, 0, 1, 7), ('And',), [('Name', (1, 0, 1, 1), 'a', ('Load',)), ('Name', (1, 6, 1, 7), 'b', ('Load',))])),
('Expression', ('BinOp', (1, 0, 1, 5), ('Name', (1, 0, 1, 1), 'a', ('Load',)), ('Add',), ('Name', (1, 4, 1, 5), 'b', ('Load',)))),
('Expression', ('UnaryOp', (1, 0, 1, 5), ('Not',), ('Name', (1, 4, 1, 5), 'v', ('Load',)))),
('Expression', ('Lambda', (1, 0, 1, 11), ('arguments', [], [], None, [], [], None, []), ('Constant', (1, 7, 1, 11), None, None))),
('Expression', ('Dict', (1, 0, 1, 7), [('Constant', (1, 2, 1, 3), 1, None)], [('Constant', (1, 4, 1, 5), 2, None)])),
('Expression', ('Dict', (1, 0, 1, 2), [], [])),
('Expression', ('Set', (1, 0, 1, 7), [('Constant', (1, 1, 1, 5), None, None)])),
('Expression', ('Dict', (1, 0, 5, 6), [('Constant', (2, 6, 2, 7), 1, None)], [('Constant', (4, 10, 4, 11), 2, None)])),
('Expression', ('ListComp', (1, 0, 1, 19), ('Name', (1, 1, 1, 2), 'a', ('Load',)), [('comprehension', ('Name', (1, 7, 1, 8), 'b', ('Store',)), ('Name', (1, 12, 1, 13), 'c', ('Load',)), [('Name', (1, 17, 1, 18), 'd', ('Load',))], 0)])),
('Expression', ('GeneratorExp', (1, 0, 1, 19), ('Name', (1, 1, 1, 2), 'a', ('Load',)), [('comprehension', ('Name', (1, 7, 1, 8), 'b', ('Store',)), ('Name', (1, 12, 1, 13), 'c', ('Load',)), [('Name', (1, 17, 1, 18), 'd', ('Load',))], 0)])),
('Expression', ('ListComp', (1, 0, 1, 20), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 14), [('Name', (1, 11, 1, 12), 'a', ('Store',)), ('Name', (1, 13, 1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 18, 1, 19), 'c', ('Load',)), [], 0)])),
('Expression', ('ListComp', (1, 0, 1, 22), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 16), [('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 14, 1, 15), 'b', ('Store',))], ('Store',)), ('Name', (1, 20, 1, 21), 'c', ('Load',)), [], 0)])),
('Expression', ('ListComp', (1, 0, 1, 22), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('List', (1, 11, 1, 16), [('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 14, 1, 15), 'b', ('Store',))], ('Store',)), ('Name', (1, 20, 1, 21), 'c', ('Load',)), [], 0)])),
('Expression', ('SetComp', (1, 0, 1, 20), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 14), [('Name', (1, 11, 1, 12), 'a', ('Store',)), ('Name', (1, 13, 1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 18, 1, 19), 'c', ('Load',)), [], 0)])),
('Expression', ('SetComp', (1, 0, 1, 22), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 16), [('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 14, 1, 15), 'b', ('Store',))], ('Store',)), ('Name', (1, 20, 1, 21), 'c', ('Load',)), [], 0)])),
('Expression', ('SetComp', (1, 0, 1, 22), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('List', (1, 11, 1, 16), [('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 14, 1, 15), 'b', ('Store',))], ('Store',)), ('Name', (1, 20, 1, 21), 'c', ('Load',)), [], 0)])),
('Expression', ('GeneratorExp', (1, 0, 1, 20), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 14), [('Name', (1, 11, 1, 12), 'a', ('Store',)), ('Name', (1, 13, 1, 14), 'b', ('Store',))], ('Store',)), ('Name', (1, 18, 1, 19), 'c', ('Load',)), [], 0)])),
('Expression', ('GeneratorExp', (1, 0, 1, 22), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('Tuple', (1, 11, 1, 16), [('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 14, 1, 15), 'b', ('Store',))], ('Store',)), ('Name', (1, 20, 1, 21), 'c', ('Load',)), [], 0)])),
('Expression', ('GeneratorExp', (1, 0, 1, 22), ('Tuple', (1, 1, 1, 6), [('Name', (1, 2, 1, 3), 'a', ('Load',)), ('Name', (1, 4, 1, 5), 'b', ('Load',))], ('Load',)), [('comprehension', ('List', (1, 11, 1, 16), [('Name', (1, 12, 1, 13), 'a', ('Store',)), ('Name', (1, 14, 1, 15), 'b', ('Store',))], ('Store',)), ('Name', (1, 20, 1, 21), 'c', ('Load',)), [], 0)])),
('Expression', ('Compare', (1, 0, 1, 9), ('Constant', (1, 0, 1, 1), 1, None), [('Lt',), ('Lt',)], [('Constant', (1, 4, 1, 5), 2, None), ('Constant', (1, 8, 1, 9), 3, None)])),
('Expression', ('Call', (1, 0, 1, 17), ('Name', (1, 0, 1, 1), 'f', ('Load',)), [('Constant', (1, 2, 1, 3), 1, None), ('Constant', (1, 4, 1, 5), 2, None), ('Starred', (1, 10, 1, 12), ('Name', (1, 11, 1, 12), 'd', ('Load',)), ('Load',))], [('keyword', (1, 6, 1, 9), 'c', ('Constant', (1, 8, 1, 9), 3, None)), ('keyword', (1, 13, 1, 16), None, ('Name', (1, 15, 1, 16), 'e', ('Load',)))])),
('Expression', ('Call', (1, 0, 1, 10), ('Name', (1, 0, 1, 1), 'f', ('Load',)), [('Starred', (1, 2, 1, 9), ('List', (1, 3, 1, 9), [('Constant', (1, 4, 1, 5), 0, None), ('Constant', (1, 7, 1, 8), 1, None)], ('Load',)), ('Load',))], [])),
('Expression', ('Call', (1, 0, 1, 15), ('Name', (1, 0, 1, 1), 'f', ('Load',)), [('GeneratorExp', (1, 1, 1, 15), ('Name', (1, 2, 1, 3), 'a', ('Load',)), [('comprehension', ('Name', (1, 8, 1, 9), 'a', ('Store',)), ('Name', (1, 13, 1, 14), 'b', ('Load',)), [], 0)])], [])),
('Expression', ('Constant', (1, 0, 1, 2), 10, None)),
('Expression', ('Constant', (1, 0, 1, 8), 'string', None)),
('Expression', ('Attribute', (1, 0, 1, 3), ('Name', (1, 0, 1, 1), 'a', ('Load',)), 'b', ('Load',))),
('Expression', ('Subscript', (1, 0, 1, 6), ('Name', (1, 0, 1, 1), 'a', ('Load',)), ('Slice', (1, 2, 1, 5), ('Name', (1, 2, 1, 3), 'b', ('Load',)), ('Name', (1, 4, 1, 5), 'c', ('Load',)), None), ('Load',))),
('Expression', ('Name', (1, 0, 1, 1), 'v', ('Load',))),
('Expression', ('List', (1, 0, 1, 7), [('Constant', (1, 1, 1, 2), 1, None), ('Constant', (1, 3, 1, 4), 2, None), ('Constant', (1, 5, 1, 6), 3, None)], ('Load',))),
('Expression', ('List', (1, 0, 1, 2), [], ('Load',))),
('Expression', ('Tuple', (1, 0, 1, 5), [('Constant', (1, 0, 1, 1), 1, None), ('Constant', (1, 2, 1, 3), 2, None), ('Constant', (1, 4, 1, 5), 3, None)], ('Load',))),
('Expression', ('Tuple', (1, 0, 1, 7), [('Constant', (1, 1, 1, 2), 1, None), ('Constant', (1, 3, 1, 4), 2, None), ('Constant', (1, 5, 1, 6), 3, None)], ('Load',))),
('Expression', ('Tuple', (1, 0, 1, 2), [], ('Load',))),
('Expression', ('Call', (1, 0, 1, 17), ('Attribute', (1, 0, 1, 7), ('Attribute', (1, 0, 1, 5), ('Attribute', (1, 0, 1, 3), ('Name', (1, 0, 1, 1), 'a', ('Load',)), 'b', ('Load',)), 'c', ('Load',)), 'd', ('Load',)), [('Subscript', (1, 8, 1, 16), ('Attribute', (1, 8, 1, 11), ('Name', (1, 8, 1, 9), 'a', ('Load',)), 'b', ('Load',)), ('Slice', (1, 12, 1, 15), ('Constant', (1, 12, 1, 13), 1, None), ('Constant', (1, 14, 1, 15), 2, None), None), ('Load',))], [])),
]
main()
