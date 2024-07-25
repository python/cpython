import ast

class ASTTestMixin:
    """Test mixing to have basic assertions for AST nodes."""

    def assertASTEqual(self, ast1, ast2):
        # Ensure the comparisons start at an AST node
        self.assertIsInstance(ast1, ast.AST)
        self.assertIsInstance(ast2, ast.AST)

        # An AST comparison routine modeled after ast.dump(), but
        # instead of string building, it traverses the two trees
        # in lock-step.
        def traverse_compare(a, b, missing=object()):
            if type(a) is not type(b):
                self.fail(f"{type(a)!r} is not {type(b)!r}")
            if isinstance(a, ast.AST):
                for field in a._fields:
                    value1 = getattr(a, field, missing)
                    value2 = getattr(b, field, missing)
                    # Singletons are equal by definition, so further
                    # testing can be skipped.
                    if value1 is not value2:
                        traverse_compare(value1, value2)
            elif isinstance(a, list):
                try:
                    for node1, node2 in zip(a, b, strict=True):
                        traverse_compare(node1, node2)
                except ValueError:
                    # Attempt a "pretty" error ala assertSequenceEqual()
                    len1 = len(a)
                    len2 = len(b)
                    if len1 > len2:
                        what = "First"
                        diff = len1 - len2
                    else:
                        what = "Second"
                        diff = len2 - len1
                    msg = f"{what} list contains {diff} additional elements."
                    raise self.failureException(msg) from None
            elif a != b:
                self.fail(f"{a!r} != {b!r}")
        traverse_compare(ast1, ast2)

def to_tuple(t):
    if t is None or isinstance(t, (str, int, complex, float, bytes)) or t is Ellipsis:
        return t
    elif isinstance(t, list):
        return [to_tuple(e) for e in t]
    result = [t.__class__.__name__]
    if hasattr(t, 'lineno') and hasattr(t, 'col_offset'):
        result.append((t.lineno, t.col_offset))
        if hasattr(t, 'end_lineno') and hasattr(t, 'end_col_offset'):
            result[-1] += (t.end_lineno, t.end_col_offset)
    if t._fields is None:
        return tuple(result)
    for f in t._fields:
        result.append(to_tuple(getattr(t, f)))
    return tuple(result)
