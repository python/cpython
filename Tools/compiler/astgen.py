"""Generate ast module from specification

This script generates the ast module from a simple specification,
which makes it easy to accomodate changes in the grammar.  This
approach would be quite reasonable if the grammar changed often.
Instead, it is rather complex to generate the appropriate code.  And
the Node interface has changed more often than the grammar.
"""

import fileinput
import getopt
import re
import sys
from StringIO import StringIO

SPEC = "ast.txt"
COMMA = ", "

def load_boilerplate(file):
    f = open(file)
    buf = f.read()
    f.close()
    i = buf.find('### ''PROLOGUE')
    j = buf.find('### ''EPILOGUE')
    pro = buf[i+12:j].strip()
    epi = buf[j+12:].strip()
    return pro, epi

def strip_default(arg):
    """Return the argname from an 'arg = default' string"""
    i = arg.find('=')
    if i == -1:
        return arg
    t = arg[:i].strip()
    return t

P_NODE = 1
P_OTHER = 2
P_NESTED = 3
P_NONE = 4

class NodeInfo:
    """Each instance describes a specific AST node"""
    def __init__(self, name, args):
        self.name = name
        self.args = args.strip()
        self.argnames = self.get_argnames()
        self.argprops = self.get_argprops()
        self.nargs = len(self.argnames)
        self.init = []

    def get_argnames(self):
        if '(' in self.args:
            i = self.args.find('(')
            j = self.args.rfind(')')
            args = self.args[i+1:j]
        else:
            args = self.args
        return [strip_default(arg.strip())
                for arg in args.split(',') if arg]

    def get_argprops(self):
        """Each argument can have a property like '*' or '!'

        XXX This method modifies the argnames in place!
        """
        d = {}
        hardest_arg = P_NODE
        for i in range(len(self.argnames)):
            arg = self.argnames[i]
            if arg.endswith('*'):
                arg = self.argnames[i] = arg[:-1]
                d[arg] = P_OTHER
                hardest_arg = max(hardest_arg, P_OTHER)
            elif arg.endswith('!'):
                arg = self.argnames[i] = arg[:-1]
                d[arg] = P_NESTED
                hardest_arg = max(hardest_arg, P_NESTED)
            elif arg.endswith('&'):
                arg = self.argnames[i] = arg[:-1]
                d[arg] = P_NONE
                hardest_arg = max(hardest_arg, P_NONE)
            else:
                d[arg] = P_NODE
        self.hardest_arg = hardest_arg

        if hardest_arg > P_NODE:
            self.args = self.args.replace('*', '')
            self.args = self.args.replace('!', '')
            self.args = self.args.replace('&', '')

        return d

    def gen_source(self):
        buf = StringIO()
        print >> buf, "class %s(Node):" % self.name
        self._gen_init(buf)
        print >> buf
        self._gen_getChildren(buf)
        print >> buf
        self._gen_getChildNodes(buf)
        print >> buf
        self._gen_repr(buf)
        buf.seek(0, 0)
        return buf.read()

    def _gen_init(self, buf):
        if self.args:
            print >> buf, "    def __init__(self, %s, lineno=None):" % self.args
        else:
            print >> buf, "    def __init__(self, lineno=None):"
        if self.argnames:
            for name in self.argnames:
                print >> buf, "        self.%s = %s" % (name, name)
        print >> buf, "        self.lineno = lineno"
        if self.init:
            print >> buf, "".join(["    " + line for line in self.init])

    def _gen_getChildren(self, buf):
        print >> buf, "    def getChildren(self):"
        if len(self.argnames) == 0:
            print >> buf, "        return ()"
        else:
            if self.hardest_arg < P_NESTED:
                clist = COMMA.join(["self.%s" % c
                                    for c in self.argnames])
                if self.nargs == 1:
                    print >> buf, "        return %s," % clist
                else:
                    print >> buf, "        return %s" % clist
            else:
                if len(self.argnames) == 1:
                    print >> buf, "        return tuple(flatten(self.%s))" % self.argnames[0]
                else:
                    print >> buf, "        children = []"
                    template = "        children.%s(%sself.%s%s)"
                    for name in self.argnames:
                        if self.argprops[name] == P_NESTED:
                            print >> buf, template % ("extend", "flatten(",
                                                      name, ")")
                        else:
                            print >> buf, template % ("append", "", name, "")
                    print >> buf, "        return tuple(children)"

    def _gen_getChildNodes(self, buf):
        print >> buf, "    def getChildNodes(self):"
        if len(self.argnames) == 0:
            print >> buf, "        return ()"
        else:
            if self.hardest_arg < P_NESTED:
                clist = ["self.%s" % c
                         for c in self.argnames
                         if self.argprops[c] == P_NODE]
                if len(clist) == 0:
                    print >> buf, "        return ()"
                elif len(clist) == 1:
                    print >> buf, "        return %s," % clist[0]
                else:
                    print >> buf, "        return %s" % COMMA.join(clist)
            else:
                print >> buf, "        nodelist = []"
                template = "        nodelist.%s(%sself.%s%s)"
                for name in self.argnames:
                    if self.argprops[name] == P_NONE:
                        tmp = ("        if self.%s is not None:\n"
                               "            nodelist.append(self.%s)")
                        print >> buf, tmp % (name, name)
                    elif self.argprops[name] == P_NESTED:
                        print >> buf, template % ("extend", "flatten_nodes(",
                                                  name, ")")
                    elif self.argprops[name] == P_NODE:
                        print >> buf, template % ("append", "", name, "")
                print >> buf, "        return tuple(nodelist)"

    def _gen_repr(self, buf):
        print >> buf, "    def __repr__(self):"
        if self.argnames:
            fmt = COMMA.join(["%s"] * self.nargs)
            if '(' in self.args:
                fmt = '(%s)' % fmt
            vals = ["repr(self.%s)" % name for name in self.argnames]
            vals = COMMA.join(vals)
            if self.nargs == 1:
                vals = vals + ","
            print >> buf, '        return "%s(%s)" %% (%s)' % \
                  (self.name, fmt, vals)
        else:
            print >> buf, '        return "%s()"' % self.name

rx_init = re.compile('init\((.*)\):')

def parse_spec(file):
    classes = {}
    cur = None
    for line in fileinput.input(file):
        if line.strip().startswith('#'):
            continue
        mo = rx_init.search(line)
        if mo is None:
            if cur is None:
                # a normal entry
                try:
                    name, args = line.split(':')
                except ValueError:
                    continue
                classes[name] = NodeInfo(name, args)
                cur = None
            else:
                # some code for the __init__ method
                cur.init.append(line)
        else:
            # some extra code for a Node's __init__ method
            name = mo.group(1)
            cur = classes[name]
    return sorted(classes.values(), key=lambda n: n.name)

def main():
    prologue, epilogue = load_boilerplate(sys.argv[-1])
    print prologue
    print
    classes = parse_spec(SPEC)
    for info in classes:
        print info.gen_source()
    print epilogue

if __name__ == "__main__":
    main()
    sys.exit(0)

### PROLOGUE
"""Python abstract syntax node definitions

This file is automatically generated by Tools/compiler/astgen.py
"""
from consts import CO_VARARGS, CO_VARKEYWORDS

def flatten(list):
    l = []
    for elt in list:
        t = type(elt)
        if t is tuple or t is list:
            for elt2 in flatten(elt):
                l.append(elt2)
        else:
            l.append(elt)
    return l

def flatten_nodes(list):
    return [n for n in flatten(list) if isinstance(n, Node)]

nodes = {}

class Node:
    """Abstract base class for ast nodes."""
    def getChildren(self):
        pass # implemented by subclasses
    def __iter__(self):
        for n in self.getChildren():
            yield n
    def asList(self): # for backwards compatibility
        return self.getChildren()
    def getChildNodes(self):
        pass # implemented by subclasses

class EmptyNode(Node):
    pass

class Expression(Node):
    # Expression is an artificial node class to support "eval"
    nodes["expression"] = "Expression"
    def __init__(self, node):
        self.node = node

    def getChildren(self):
        return self.node,

    def getChildNodes(self):
        return self.node,

    def __repr__(self):
        return "Expression(%s)" % (repr(self.node))

### EPILOGUE
for name, obj in globals().items():
    if isinstance(obj, type) and issubclass(obj, Node):
        nodes[name.lower()] = obj
