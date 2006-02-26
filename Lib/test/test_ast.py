import sys, itertools

def to_tuple(t):
    if t is None or isinstance(t, (str, int, long)):
        return t
    elif isinstance(t, list):
        return [to_tuple(e) for e in t]
    result = [t.__class__.__name__]
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
  # Yield
  #"def f():yield 3",
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
  "1,2,3"
]

# TODO: expr_context, slice, boolop, operator, unaryop, cmpop, comprehension
# excepthandler, arguments, keywords, alias

if __name__=='__main__' and sys.argv[1:] == ['-g']:
    for statements, kind in ((exec_tests, "exec"), (single_tests, "single"), (eval_tests, "eval")):
        print kind+"_results = ["
        for s in statements:
            print repr(to_tuple(compile(s, "?", kind, 0x400)))+","
        print "]"
    print "run_tests()"
    raise SystemExit

def run_tests():
    for input, output, kind in ((exec_tests, exec_results, "exec"), 
                                        (single_tests, single_results, "single"), 
                                        (eval_tests, eval_results, "eval")):
        for i, o in itertools.izip(input, output):
            assert to_tuple(compile(i, "?", kind, 0x400)) == o

#### EVERYTHING BELOW IS GENERATED ##### 
exec_results = [
('Module', [('FunctionDef', 'f', ('arguments', [], None, None, []), [('Pass',)], [])]),
('Module', [('ClassDef', 'C', [], [('Pass',)])]),
('Module', [('FunctionDef', 'f', ('arguments', [], None, None, []), [('Return', ('Num', 1))], [])]),
('Module', [('Delete', [('Name', 'v', ('Del',))])]),
('Module', [('Assign', [('Name', 'v', ('Store',))], ('Num', 1))]),
('Module', [('AugAssign', ('Name', 'v', ('Load',)), ('Add',), ('Num', 1))]),
('Module', [('Print', ('Name', 'f', ('Load',)), [('Num', 1)], False)]),
('Module', [('For', ('Name', 'v', ('Store',)), ('Name', 'v', ('Load',)), [('Pass',)], [])]),
('Module', [('While', ('Name', 'v', ('Load',)), [('Pass',)], [])]),
('Module', [('If', ('Name', 'v', ('Load',)), [('Pass',)], [])]),
('Module', [('Raise', ('Name', 'Exception', ('Load',)), ('Str', 'string'), None)]),
('Module', [('TryExcept', [('Pass',)], [('excepthandler', ('Name', 'Exception', ('Load',)), None, [('Pass',)])], [])]),
('Module', [('TryFinally', [('Pass',)], [('Pass',)])]),
('Module', [('Assert', ('Name', 'v', ('Load',)), None)]),
('Module', [('Import', [('alias', 'sys', None)])]),
('Module', [('ImportFrom', 'sys', [('alias', 'v', None)])]),
('Module', [('Exec', ('Str', 'v'), None, None)]),
('Module', [('Global', ['v'])]),
('Module', [('Expr', ('Num', 1))]),
('Module', [('Pass',)]),
('Module', [('Break',)]),
('Module', [('Continue',)]),
]
single_results = [
('Interactive', [('Expr', ('BinOp', ('Num', 1), ('Add',), ('Num', 2)))]),
]
eval_results = [
('Expression', ('BoolOp', ('And',), [('Name', 'a', ('Load',)), ('Name', 'b', ('Load',))])),
('Expression', ('BinOp', ('Name', 'a', ('Load',)), ('Add',), ('Name', 'b', ('Load',)))),
('Expression', ('UnaryOp', ('Not',), ('Name', 'v', ('Load',)))),
('Expression', ('Lambda', ('arguments', [], None, None, []), ('Name', 'None', ('Load',)))),
('Expression', ('Dict', [('Num', 1)], [('Num', 2)])),
('Expression', ('ListComp', ('Name', 'a', ('Load',)), [('comprehension', ('Name', 'b', ('Store',)), ('Name', 'c', ('Load',)), [('Name', 'd', ('Load',))])])),
('Expression', ('GeneratorExp', ('Name', 'a', ('Load',)), [('comprehension', ('Name', 'b', ('Store',)), ('Name', 'c', ('Load',)), [('Name', 'd', ('Load',))])])),
('Expression', ('Compare', ('Num', 1), [('Lt',), ('Lt',)], [('Num', 2), ('Num', 3)])),
('Expression', ('Call', ('Name', 'f', ('Load',)), [('Num', 1), ('Num', 2)], [('keyword', 'c', ('Num', 3))], ('Name', 'd', ('Load',)), ('Name', 'e', ('Load',)))),
('Expression', ('Repr', ('Name', 'v', ('Load',)))),
('Expression', ('Num', 10L)),
('Expression', ('Str', 'string')),
('Expression', ('Attribute', ('Name', 'a', ('Load',)), 'b', ('Load',))),
('Expression', ('Subscript', ('Name', 'a', ('Load',)), ('Slice', ('Name', 'b', ('Load',)), ('Name', 'c', ('Load',)), None), ('Load',))),
('Expression', ('Name', 'v', ('Load',))),
('Expression', ('List', [('Num', 1), ('Num', 2), ('Num', 3)], ('Load',))),
('Expression', ('Tuple', [('Num', 1), ('Num', 2), ('Num', 3)], ('Load',))),
]
run_tests()

