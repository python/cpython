import marshal
import bkfile


# Write a file containing frozen code for the modules in the dictionary.

header = """
#include "Python.h"

static struct _frozen _PyImport_FrozenModules[] = {
"""
trailer = """\
    {0, 0, 0} /* sentinel */
};
"""

# if __debug__ == 0 (i.e. -O option given), set Py_OptimizeFlag in frozen app.
default_entry_point = """
int
main(int argc, char **argv)
{
        extern int Py_FrozenMain(int, char **);
""" + ((not __debug__ and """
        Py_OptimizeFlag++;
""") or "")  + """
        PyImport_FrozenModules = _PyImport_FrozenModules;
        return Py_FrozenMain(argc, argv);
}

"""

def makefreeze(base, dict, debug=0, entry_point=None, fail_import=()):
    if entry_point is None: entry_point = default_entry_point
    done = []
    files = []
    mods = sorted(dict.keys())
    for mod in mods:
        m = dict[mod]
        mangled = "__".join(mod.split("."))
        if m.__code__:
            file = 'M_' + mangled + '.c'
            with bkfile.open(base + file, 'w') as outfp:
                files.append(file)
                if debug:
                    print("freezing", mod, "...")
                str = marshal.dumps(m.__code__)
                size = len(str)
                is_package = '0'
                if m.__path__:
                    is_package = '1'
                done.append((mod, mangled, size, is_package))
                writecode(outfp, mangled, str)
    if debug:
        print("generating table of frozen modules")
    with bkfile.open(base + 'frozen.c', 'w') as outfp:
        for mod, mangled, size, _ in done:
            outfp.write('extern unsigned char M_%s[];\n' % mangled)
        outfp.write(header)
        for mod, mangled, size, is_package in done:
            outfp.write('\t{"%s", M_%s, %d, %s},\n' % (mod, mangled, size, is_package))
        outfp.write('\n')
        # The following modules have a NULL code pointer, indicating
        # that the frozen program should not search for them on the host
        # system. Importing them will *always* raise an ImportError.
        # The zero value size is never used.
        for mod in fail_import:
            outfp.write('\t{"%s", NULL, 0},\n' % (mod,))
        outfp.write(trailer)
        outfp.write(entry_point)
    return files



# Write a C initializer for a module containing the frozen python code.
# The array is called M_<mod>.

def writecode(fp, mod, data):
    print('unsigned char M_%s[] = {' % mod, file=fp)
    indent = ' ' * 4
    for i in range(0, len(data), 16):
        print(indent, file=fp, end='')
        for c in bytes(data[i:i+16]):
            print('%d,' % c, file=fp, end='')
        print('', file=fp)
    print('};', file=fp)
