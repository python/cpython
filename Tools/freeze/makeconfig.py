import re


# Write the config.c file

never = ['marshal', '__main__', 'builtins', 'sys', 'exceptions']

def makeconfig(infp, outfp, modules, with_ifdef=0):
    m1 = re.compile('-- ADDMODULE MARKER 1 --')
    m2 = re.compile('-- ADDMODULE MARKER 2 --')
    while 1:
        line = infp.readline()
        if not line: break
        outfp.write(line)
        if m1 and m1.search(line):
            m1 = None
            for mod in modules:
                if mod in never:
                    continue
                if with_ifdef:
                    outfp.write("#ifndef init%s\n"%mod)
                outfp.write('extern void init%s(void);\n' % mod)
                if with_ifdef:
                    outfp.write("#endif\n")
        elif m2 and m2.search(line):
            m2 = None
            for mod in modules:
                if mod in never:
                    continue
                outfp.write('\t{"%s", init%s},\n' %
                            (mod, mod))
    if m1:
        sys.stderr.write('MARKER 1 never found\n')
    elif m2:
        sys.stderr.write('MARKER 2 never found\n')


# Test program.

def test():
    import sys
    if not sys.argv[3:]:
        print('usage: python makeconfig.py config.c.in outputfile', end=' ')
        print('modulename ...')
        sys.exit(2)
    if sys.argv[1] == '-':
        infp = sys.stdin
    else:
        infp = open(sys.argv[1])
    if sys.argv[2] == '-':
        outfp = sys.stdout
    else:
        outfp = open(sys.argv[2], 'w')
    makeconfig(infp, outfp, sys.argv[3:])
    if outfp != sys.stdout:
        outfp.close()
    if infp != sys.stdin:
        infp.close()

if __name__ == '__main__':
    test()
