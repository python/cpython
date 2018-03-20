#!/usr/bin/env python3
# Alernative compile tool for .py files, like compile_all.py.
#
# Neil Schemenauer <nas@python.ca>, March 2018
#
# Examine top-level definitions (functions, classes, imports).  Don't load
# them into memory unless they are actually access.  Access is detected via
# LOAD_NAME/LOAD_GLOBAL or by the module getattr method.  This relies on a
# modified Python that looks for a __lazy_code__ dict inside the module
# globals.  The keys of the dict are the variable names.  The values are
# bytestrings which are marshalled code objects that create the variable
# values.
#
# This could potentially save memory and make startup time faster.  I would
# guess that for most modules, only a fraction of their global names are ever
# accessed.  Credit to Larry Hastings for this idea (PHP did something similar
# for a big gain).
#
# Currently the marshalled code is loaded into RAM when the module is imported.
# Instead, we could keep the marshalled code on disk and only load it on
# getattr.
#
# We can use lazy analyzer AST walking tool to look for things that are
# safe to keep as marshal data.  I.e. anything that potentially has global
# side-effects will get loaded as per normal.

import sys
import imp
import ast
import marshal
import importlib.util
import py_compile
from importlib.machinery import SourceFileLoader
import lazy_analyze

PY_EXT = ".py"

class FileLoader(SourceFileLoader):
    @staticmethod
    def source_to_code(data, path, *, _optimize=-1):
        node = parse(data, path)
        return compile(node, path, 'exec', dont_inherit=True,
                       optimize=_optimize)


class Transformer(ast.NodeTransformer):
    def __init__(self, fn, *args, **kwargs):
        ast.NodeTransformer.__init__(self, *args, **kwargs)
        self.fn = fn
        self.lazy_defs = set()
        self.is_lazy = False

    def _is_lazy_assign(self, node):
        return (len(node.targets) == 1 and
                isinstance(node.targets[0], ast.Name))


    def _make_init_dict(self):
        name = ast.Name(id='__lazy_code__', ctx=ast.Store(),
                        lineno=1)
        assign = ast.Assign(targets=[name], value=ast.Dict(keys=[],
                                                           values=[]),
                            lineno=1)
        ast.fix_missing_locations(assign)
        return assign

    def visit_Module(self, node):
        for stmt in node.body:
            stmt_name = stmt.__class__.__name__
            if stmt_name == 'Assign':
                target = stmt.targets[0]
                if getattr(target, 'id', None) == '__lazy_module__':
                    self.is_lazy = True
            if stmt_name == 'ClassDef':
                self.lazy_defs.add(stmt)
            elif stmt_name in {'FunctionDef', 'Assign'}:
                if stmt_name == 'Assign' and not self._is_lazy_assign(stmt):
                    continue
                if lazy_analyze.is_lazy_safe(stmt):
                    self.lazy_defs.add(stmt)
        self.is_lazy = self.is_lazy and self.lazy_defs
        if not self.is_lazy:
            # nothing to do, skip __class__ and other stuff
            return node
        # skip __future__ statements
        future_stmt = None
        body = []
        for stmt in node.body:
            if isinstance(stmt, ast.ImportFrom) and stmt.module == '__future__':
                future_stmt = stmt
                body.append(stmt)
            elif isinstance(stmt, ast.Import):
                # translate import statements, one import node can become
                # many
                nodes = self._compile_import(stmt)
                body.extend(nodes)
            elif isinstance(stmt, ast.ImportFrom):
                nodes = self._compile_import_from(stmt)
                body.extend(nodes)
            else:
                body.append(stmt)
        # insert our new code into the start of module body
        if future_stmt is not None:
            idx = body.index(future_stmt)
        else:
            idx = 0
        body[idx:idx] = [self._make_init_dict()]
        node.body[:] = body
        return self.generic_visit(node)

    def _compile_stmt(self, node):
        code = compile(ast.Module(body=[node]), self.fn, 'exec')
        return marshal.dumps(code)

    def _store_code(self, code_name, code):
        # store marshal data in the dict
        name = ast.Name(id='__lazy_code__', ctx=ast.Load())
        index = ast.Index(ast.Str(code_name))
        target = ast.Subscript(value=name, slice=index, ctx=ast.Store())
        assign = ast.Assign(targets=[target], value=ast.Bytes(code))
        ast.fix_missing_locations(assign)
        return assign

    def visit_FunctionDef(self, node):
        if node not in self.lazy_defs:
            return self.generic_visit(node) # compile as normal
        code = self._compile_stmt(node)
        return self._store_code(node.name, code)

    def visit_Assign(self, node):
        if node not in self.lazy_defs:
            return self.generic_visit(node)
        else:
            code = self._compile_stmt(node)
            return self._store_code(node.targets[0].id, code)

    def visit_ClassDef(self, node):
        if node not in self.lazy_defs:
            return self.generic_visit(node)
        else:
            code = self._compile_stmt(node)
            return self._store_code(node.name, code)

    def _compile_import(self, node):
        nodes = []
        for alias in node.names:
            if alias.name == '*':
                return [self.generic_visit(node)] # don't translate
            # make import node for each name
            import_node = ast.Import([alias])
            ast.copy_location(import_node, node)
            code = self._compile_stmt(import_node)
            name = alias.asname or alias.name
            if '.' in name:
                name = name.split('.')[0]
            n = self._store_code(name, code)
            nodes.append(n)
        return nodes

    def _compile_import_from(self, node):
        nodes = []
        code = self._compile_stmt(node)
        for alias in node.names:
            if alias.name == '*':
                return [self.generic_visit(node)] # don't translate
            import_node = ast.ImportFrom(node.module, [alias], node.level)
            ast.copy_location(import_node, node)
            code = self._compile_stmt(import_node)
            n = self._store_code(alias.asname or alias.name, code)
            nodes.append(n)
        return nodes


def parse(buf, filename='<string>'):
    if isinstance(buf, bytes):
        buf = importlib.util.decode_source(buf)
    try:
        node = ast.parse(buf, filename)
    except SyntaxError as e:
        # set the filename attribute
        raise SyntaxError(str(e), (filename, e.lineno, e.offset, e.text))
    t = Transformer(filename)
    return t.visit(node)


py_compile_get_loader = py_compile.get_loader


def get_lazy_loader(file):
    return FileLoader('<lazy_compile>', file)

# monkey-patch
py_compile.get_loader = get_lazy_loader

if __name__ == '__main__':
    import compileall
    exit_status = int(not compileall.main())
    sys.exit(exit_status)
