import compiler
import dis
import types

def extract_code_objects(co):
    l = [co]
    for const in co.co_consts:
        if type(const) == types.CodeType:
            l.append(const)
    return l

def compare(a, b):
    if not (a.co_name == "?" or a.co_name.startswith('<lambda')):
        assert a.co_name == b.co_name, (a, b)
    if a.co_stacksize != b.co_stacksize:
        print "stack mismatch %s: %d vs. %d" % (a.co_name,
                                                a.co_stacksize,
                                                b.co_stacksize)
        if a.co_stacksize > b.co_stacksize:
            print "good code"
            dis.dis(a)
            print "bad code"
            dis.dis(b)
            assert 0

def main(files):
    for file in files:
        print file
        buf = open(file).read()
        try:
            co1 = compile(buf, file, "exec")
        except SyntaxError:
            print "skipped"
            continue
        co2 = compiler.compile(buf, file, "exec")
        co1l = extract_code_objects(co1)
        co2l = extract_code_objects(co2)
        for a, b in zip(co1l, co2l):
            compare(a, b)

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
