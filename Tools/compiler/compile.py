import sys
import getopt

from compiler import compile, visitor

import profile

def main():
    VERBOSE = 0
    DISPLAY = 0
    PROFILE = 0
    CONTINUE = 0
    opts, args = getopt.getopt(sys.argv[1:], 'vqdcp')
    for k, v in opts:
        if k == '-v':
            VERBOSE = 1
            visitor.ASTVisitor.VERBOSE = visitor.ASTVisitor.VERBOSE + 1
        if k == '-q':
            if sys.platform[:3]=="win":
                f = open('nul', 'wb') # /dev/null fails on Windows...
            else:
                f = open('/dev/null', 'wb')
            sys.stdout = f
        if k == '-d':
            DISPLAY = 1
        if k == '-c':
            CONTINUE = 1
        if k == '-p':
            PROFILE = 1
    if not args:
        print "no files to compile"
    else:
        for filename in args:
            if VERBOSE:
                print filename
            try:
                if PROFILE:
                    profile.run('compile(%s, %s)' % (`filename`, `DISPLAY`),
                                filename + ".prof")
                else:
                    compile(filename, DISPLAY)
                    
            except SyntaxError, err:
                print err
                print err.lineno
                if not CONTINUE:
                    sys.exit(-1)

if __name__ == "__main__":
    main()
