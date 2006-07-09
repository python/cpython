import sys, itertools
import _ast

def to_tuple(t):
    if t is None or isinstance(t, (basestring, int, long, complex)):
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
# There should be atleast one test per statement
exec_tests = [
    # FunctionDef
    "def f(): pass",
    # ClassDef
    "class C:pass",
    # Return
    "def f():return 1",
    # Delete
    "del v",
    # Assign
    "v = 1",
    # AugAssign
    "v += 1",
    # Print
    "print >>f, 1, ",
    # For
    "for v in v:pass",
    # While
    "while v:pass",
    # If
    "if v:pass",
    # Raise
    "raise Exception, 'string'",
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
    # Exec
    "exec 'v'",
    # Global
    "global v",
    # Expr
    "1",
    # Pass,
    "pass",
    # Break
    "break",
    # Continue
    "continue",
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
  # ListComp
  "[a for b in c if d]",
  # GeneratorExp
  "(a for b in c if d)",
  # Yield - yield expressions can't work outside a function
  #
  # Compare
  "1 < 2 < 3",
  # Call
  "f(1,2,c=3,*d,**e)",
  # Repr
  "`v`",
  # Num
  "10L",
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
  # Tuple
  "1,2,3",
  # Combination
  "a.b.c.d(a.b[1:2])",

]

# TODO: expr_context, slice, boolop, operator, unaryop, cmpop, comprehension
# excepthandler, arguments, keywords, alias

if __name__=='__main__' and sys.argv[1:] == ['-g']:
    for statements, kind in ((exec_tests, "exec"), (single_tests, "single"),
                             (eval_tests, "eval")):
        print kind+"_results = ["
        for s in statements:
            print repr(to_tuple(compile(s, "?", kind, 0x400)))+","
        print "]"
    print "run_tests()"
    raise SystemExit

def test_order(ast_node, parent_pos):

    if not isinstance(ast_node, _ast.AST) or ast_node._fields == None:
        return
    if isinstance(ast_node, (_ast.expr, _ast.stmt, _ast.excepthandler)):
        node_pos = (ast_node.lineno, ast_node.col_offset)
        assert node_pos >= parent_pos, (node_pos, parent_pos)
        parent_pos = (ast_node.lineno, ast_node.col_offset)
    for name in ast_node._fields:
        value = getattr(ast_node, name)
        if isinstance(value, list):
            for child in value:
                test_order(child, parent_pos)
        elif value != None:
            test_order(value, parent_pos)

def run_tests():
    for input, output, kind in ((exec_tests, exec_results, "exec"),
                                (single_tests, single_results, "single"),
                                (eval_tests, eval_results, "eval")):
        for i, o in itertools.izip(input, output):
            ast_tree = compile(i, "?", kind, 0x400)
            assert to_tuple(ast_tree) == o
            test_order(ast_tree, (0, 0))

#### EVERYTHING BELOW IS GENERATED #####
exec_results = [
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], None, None, []), [('Pass', (1, 9))], [])]),
('Module', [('ClassDef', (1, 0), 'C', [], [('Pass', (1, 8))])]),
('Module', [('FunctionDef', (1, 0), 'f', ('arguments', [], None, None, []), [('Return', (1, 8), ('Num', (1, 15), 1))], [])]),
('Module', [('Delete', (1, 0), [('Name', (1, 4), 'v', ('Del',))])]),
('Module', [('Assign', (1, 0), [('Name', (1, 0), 'v', ('Store',))], ('Num', (1, 4), 1))]),
('Module', [('AugAssign', (1, 0), ('Name', (1, 0), 'v', ('Store',)), ('Add',), ('Num', (1, 5), 1))]),
('Module', [('Print', (1, 0), ('Name', (1, 8), 'f', ('Load',)), [('Num', (1, 11), 1)], False)]),
('Module', [('For', (1, 0), ('Name', (1, 4), 'v', ('Store',)), ('Name', (1, 9), 'v', ('Load',)), [('Pass', (1, 11))], [])]),
('Module', [('While', (1, 0), ('Name', (1, 6), 'v', ('Load',)), [('Pass', (1, 8))], [])]),
('Module', [('If', (1, 0), ('Name', (1, 3), 'v', ('Load',)), [('Pass', (1, 5))], [])]),
('Module', [('Raise', (1, 0), ('Name', (1, 6), 'Exception', ('Load',)), ('Str', (1, 17), 'string'), None)]),
('Module', [('TryExcept', (1, 0), [('Pass', (2, 2))], [('excepthandler', (3, 0), ('Name', (3, 7), 'Exception', ('Load',)), None, [('Pass', (4, 2))], 3, 0)], [])]),
('Module', [('TryFinally', (1, 0), [('Pass', (2, 2))], [('Pass', (4, 2))])]),
('Module', [('Assert', (1, 0), ('Name', (1, 7), 'v', ('Load',)), None)]),
('Module', [('Import', (1, 0), [('alias', 'sys', None)])]),
('Module', [('ImportFrom', (1, 0), 'sys', [('alias', 'v', None)], 0)]),
('Module', [('Exec', (1, 0), ('Str', (1, 5), 'v'), None, None)]),
('Module', [('Global', (1, 0), ['v'])]),
('Module', [('Expr', (1, 0), ('Num', (1, 0), 1))]),
('Module', [('Pass', (1, 0))]),
('Module', [('Break', (1, 0))]),
('Module', [('Continue', (1, 0))]),
]
single_results = [
('Interactive', [('Expr', (1, 0), ('BinOp', (1, 0), ('Num', (1, 0), 1), ('Add',), ('Num', (1, 2), 2)))]),
]
eval_results = [
('Expression', ('BoolOp', (1, 0), ('And',), [('Name', (1, 0), 'a', ('Load',)), ('Name', (1, 6), 'b', ('Load',))])),
('Expression', ('BinOp', (1, 0), ('Name', (1, 0), 'a', ('Load',)), ('Add',), ('Name', (1, 4), 'b', ('Load',)))),
('Expression', ('UnaryOp', (1, 0), ('Not',), ('Name', (1, 4), 'v', ('Load',)))),
('Expression', ('Lambda', (1, 0), ('arguments', [], None, None, []), ('Name', (1, 7), 'None', ('Load',)))),
('Expression', ('Dict', (1, 0), [('Num', (1, 2), 1)], [('Num', (1, 4), 2)])),
('Expression', ('ListComp', (1, 1), ('Name', (1, 1), 'a', ('Load',)), [('comprehension', ('Name', (1, 7), 'b', ('Store',)), ('Name', (1, 12), 'c', ('Load',)), [('Name', (1, 17), 'd', ('Load',))])])),
('Expression', ('GeneratorExp', (1, 1), ('Name', (1, 1), 'a', ('Load',)), [('comprehension', ('Name', (1, 7), 'b', ('Store',)), ('Name', (1, 12), 'c', ('Load',)), [('Name', (1, 17), 'd', ('Load',))])])),
('Expression', ('Compare', (1, 0), ('Num', (1, 0), 1), [('Lt',), ('Lt',)], [('Num', (1, 4), 2), ('Num', (1, 8), 3)])),
('Expression', ('Call', (1, 0), ('Name', (1, 0), 'f', ('Load',)), [('Num', (1, 2), 1), ('Num', (1, 4), 2)], [('keyword', 'c', ('Num', (1, 8), 3))], ('Name', (1, 11), 'd', ('Load',)), ('Name', (1, 15), 'e', ('Load',)))),
('Expression', ('Repr', (1, 0), ('Name', (1, 1), 'v', ('Load',)))),
('Expression', ('Num', (1, 0), 10L)),
('Expression', ('Str', (1, 0), 'string')),
('Expression', ('Attribute', (1, 0), ('Name', (1, 0), 'a', ('Load',)), 'b', ('Load',))),
('Expression', ('Subscript', (1, 0), ('Name', (1, 0), 'a', ('Load',)), ('Slice', ('Name', (1, 2), 'b', ('Load',)), ('Name', (1, 4), 'c', ('Load',)), None), ('Load',))),
('Expression', ('Name', (1, 0), 'v', ('Load',))),
('Expression', ('List', (1, 0), [('Num', (1, 1), 1), ('Num', (1, 3), 2), ('Num', (1, 5), 3)], ('Load',))),
('Expression', ('Tuple', (1, 0), [('Num', (1, 0), 1), ('Num', (1, 2), 2), ('Num', (1, 4), 3)], ('Load',))),
('Expression', ('Call', (1, 0), ('Attribute', (1, 0), ('Attribute', (1, 0), ('Attribute', (1, 0), ('Name', (1, 0), 'a', ('Load',)), 'b', ('Load',)), 'c', ('Load',)), 'd', ('Load',)), [('Subscript', (1, 8), ('Attribute', (1, 8), ('Name', (1, 8), 'a', ('Load',)), 'b', ('Load',)), ('Slice', ('Num', (1, 12), 1), ('Num', (1, 14), 2), None), ('Load',))], [], None, None)),
]
run_tests()
