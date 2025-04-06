"""
Copy-parse of ast.dump, removing the `isinstance` checks. This is needed,
because testing pegen requires generating a C extension module, which contains
a copy of the symbols defined in Python-ast.c. Thus, the isinstance check would
always fail. We rely on string comparison of the base classes instead.
TODO: Remove the above-described hack.
"""

from typing import Any, Optional, Tuple


def ast_dump(
    node: Any,
    annotate_fields: bool = True,
    include_attributes: bool = False,
    *,
    indent: Optional[str] = None,
) -> str:
    def _format(node: Any, level: int = 0) -> Tuple[str, bool]:
        if indent is not None:
            level += 1
            prefix = "\n" + indent * level
            sep = ",\n" + indent * level
        else:
            prefix = ""
            sep = ", "
        if any(cls.__name__ == "AST" for cls in node.__class__.__mro__):
            cls = type(node)
            args = []
            allsimple = True
            keywords = annotate_fields
            for name in node._fields:
                try:
                    value = getattr(node, name)
                except AttributeError:
                    keywords = True
                    continue
                if value is None and getattr(cls, name, ...) is None:
                    keywords = True
                    continue
                value, simple = _format(value, level)
                allsimple = allsimple and simple
                if keywords:
                    args.append("%s=%s" % (name, value))
                else:
                    args.append(value)
            if include_attributes and node._attributes:
                for name in node._attributes:
                    try:
                        value = getattr(node, name)
                    except AttributeError:
                        continue
                    if value is None and getattr(cls, name, ...) is None:
                        continue
                    value, simple = _format(value, level)
                    allsimple = allsimple and simple
                    args.append("%s=%s" % (name, value))
            if allsimple and len(args) <= 3:
                return "%s(%s)" % (node.__class__.__name__, ", ".join(args)), not args
            return "%s(%s%s)" % (node.__class__.__name__, prefix, sep.join(args)), False
        elif isinstance(node, list):
            if not node:
                return "[]", True
            return "[%s%s]" % (prefix, sep.join(_format(x, level)[0] for x in node)), False
        return repr(node), True

    if all(cls.__name__ != "AST" for cls in node.__class__.__mro__):
        raise TypeError("expected AST, got %r" % node.__class__.__name__)
    return _format(node)[0]
