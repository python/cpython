#! /usr/bin/env python

"""Print names of all methods defined in module

This script demonstrates use of the visitor interface of the compiler
package.
"""

import compiler

class MethodFinder:
    """Print the names of all the methods

    Each visit method takes two arguments, the node and its current
    scope.  The scope is the name of the current class or None.
    """

    def visitClass(self, node, scope=None):
        self.visit(node.code, node.name)

    def visitFunction(self, node, scope=None):
        if scope is not None:
            print "%s.%s" % (scope, node.name)
        self.visit(node.code, None)

def main(files):
    mf = MethodFinder()
    for file in files:
        f = open(file)
        buf = f.read()
        f.close()
        ast = compiler.parse(buf)
        compiler.walk(ast, mf)

if __name__ == "__main__":
    import sys

    main(sys.argv[1:])
