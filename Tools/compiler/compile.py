import sys
import getopt

from compiler import compile, visitor

def main():
    VERBOSE = 0
    opts, args = getopt.getopt(sys.argv[1:], 'vq')
    for k, v in opts:
        if k == '-v':
            VERBOSE = 1
            visitor.ASTVisitor.VERBOSE = visitor.ASTVisitor.VERBOSE + 1
        if k == '-q':
            f = open('/dev/null', 'wb')
            sys.stdout = f
    if not args:
        print "no files to compile"
    else:
        for filename in args:
            if VERBOSE:
                print filename
            compile(filename)

if __name__ == "__main__":
    main()
