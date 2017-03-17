Issue #26146: Add a new kind of AST node: ``ast.Constant``. It can be used
by external AST optimizers, but the compiler does not emit directly such
node.