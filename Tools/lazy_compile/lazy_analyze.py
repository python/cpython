import ast
import importlib.util
import sys
import tokenize

class OPTIONS:
    verbose = 0
    allow_bases = True


class Analyzer(ast.NodeVisitor):

    # see https://docs.python.org/3/library/ast.html#abstract-grammar
    SAFE = {
        'Module',
        'Interactive',
        'Expression',
        'Return',
        'Global',
        'Nonlocal',
        'Expr',
        'Pass',
        'Break',
        'Continue',
        'IfExp',
        'Dict',
        'Set',
        'ListComp',
        'SetComp',
        'DictComp',
        'GeneratorExp',
        'Num',
        'Str',
        'Bytes',
        'NameConstant',
        'Ellipsis',
        'Constant',
        'Name',
        'List',
        'Tuple',
        'Store',
        'Load',
        }

    UNSAFE = {
        'Delete',
        'Raise',
        'Try',
        'Assert',
        'BoolOp',
        'BinOp',
        'UnaryOp',
        'Lambda', # FIXE, could be safe
        'AugAssign',
        'AnnAssign',
        # maybe safe
        'For',
        'AsyncWith',
        'While',
        'If',
        'With',
        'AsyncWith',
        'Await',
        'Yield',
        'YieldFrom',
        'Compare',
        'Call',
        'FormattedValue',
        'JoinedStr',
        'Attribute',
        'Subscript',
        'Starred',
        'Slice',
        'ExtSlice',
        'Index',
        'And',
        'Or',
        'Add',
        'Sub',
        'Mult',
        'MatMult',
        'Div',
        'Mod',
        'Pow',
        'LShift',
        'RShift',
        'BitOr',
        'BitXor',
        'BitAnd',
        'FloorDiv',
        'Invert',
        'Not',
        'UAdd',
        'USub',
        'Eq',
        'NotEq',
        'Lt',
        'LtE',
        'Gt',
        'GtE',
        'Is',
        'IsNot',
        'In',
        'NotIn',
        'comprehension',
        }

    def __init__(self, fn, *args, **kwargs):
        ast.NodeVisitor.__init__(self, *args, **kwargs)
        self.fn = fn
        self.imports = []
        self.safe = True

    def analyze(self, node):
        self.visit(node)

    def visit(self, node):
        if node is None:
            return # FIXME: how?
        name = node.__class__.__name__
        func = getattr(self, 'visit_' + name, None)
        if func is not None:
            func(node)
        elif name in self.SAFE:
            ast.NodeVisitor.visit(self, node)
        elif name in self.UNSAFE:
            self.note_unsafe('unsafe', node)
        else:
            raise RuntimeError(f'unknown node {name}')

    def note_unsafe(self, msg, node):
        if not self.safe and OPTIONS.verbose <= 1:
            return # only print first warning
        self.safe = False
        if hasattr(node, 'lineno'):
            lineno = node.lineno
        else:
            lineno = '???'
        if OPTIONS.verbose:
            print(f'{self.fn}:{lineno}: {msg} {node.__class__.__name__}')

    def visit_Assign(self, node):
        # Assignments are safe if the RHS doesn't have side effects
        for target in node.targets:
            self.visit(target)
        self.visit(node.value)

    def visit_Import(self, node):
        for alias in node.names:
            self.imports.append(alias.name)

    def visit_ImportFrom(self, node):
        # This is unfortunately not safe as it is like a getattr.
        # E.g. could raise error, cause getattr hook to run.
        self.imports.append(node.module)

    def visit_ClassDef(self, node):
        # bases, body, decorator_list, keywords (empty)
        if node.bases:
            if not OPTIONS.allow_bases:
                self.note_unsafe('base classes', node)
                return
        for stmt in node.body:
            self.visit(stmt)

    def visit_FunctionDef(self, node):
        # args, body, decorator_list, returns
        if node.decorator_list:
            self.note_unsafe('decorator', node)
        for v in node.args.kw_defaults:
            self.visit(v)
        # FIXME: type annotations?

    def visit_AsyncFunctionDef(self, node):
        self.visit_FunctionDef(node)


def parse(buf, filename='<string>'):
    if isinstance(buf, bytes):
        buf = importlib.util.decode_source(buf)
    try:
        node = ast.parse(buf, filename)
    except SyntaxError as e:
        # set the filename attribute
        raise SyntaxError(str(e), (filename, e.lineno, e.offset, e.text))
    return node


def analyze(node, fn):
    t = Analyzer(fn)
    t.analyze(node)
    return t

def is_lazy_safe(node):
    fn = None
    t = Analyzer(fn)
    t.analyze(node)
    return t.safe


USAGE = "Usage: %prog [-v] file [...]"

def main():
    global OPTIONS
    import optparse
    parser = optparse.OptionParser(USAGE)
    parser.add_option('-v', '--verbose',
                      action='count', dest='verbose', default=0,
                      help="enable extra status output")
    parser.add_option('-b', '--allow-bases', action='store_true',
                      help="allow base classes")
    OPTIONS, args = parser.parse_args()
    lazy = set()
    eager = set()
    for fn in args:
        try:
            fp = tokenize.open(fn)
        except SyntaxError:
            continue
        try:
            buf = fp.read()
        except UnicodeDecodeError:
            continue
        finally:
            fp.close()
        try:
            node = parse(buf)
        except SyntaxError:
            continue
        a = analyze(node, fn)
        if not a.safe:
            eager.add(fn)
        else:
            lazy.add(fn)
    total = len(lazy) + len(eager)
    if not total:
        print('warning: no Python modules parsed.')
        return
    print('Eager modules:')
    for fn in sorted(eager):
        print(f'    {fn}')
    print('Lazy modules:')
    for fn in sorted(lazy):
        print(f'    {fn}')
    print(f'{len(lazy) / total * 100:.1f}% - total: {total}')


if __name__ == '__main__':
    main()
