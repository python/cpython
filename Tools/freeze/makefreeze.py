import marshal
import string


# Write a file containing frozen code for the modules in the dictionary.

header = """
#include "Python.h"

static struct _frozen _PyImport_FrozenModules[] = {
"""
trailer = """\
    {0, 0, 0} /* sentinel */
};
"""

default_entry_point = """
int
main(argc, argv)
    int argc;
    char **argv;
{
        PyImport_FrozenModules = _PyImport_FrozenModules;
        return Py_FrozenMain(argc, argv);
}

"""

def makefreeze(outfp, dict, debug=0, entry_point = None):
    if entry_point is None: entry_point = default_entry_point
    done = []
    mods = dict.keys()
    mods.sort()
    for mod in mods:
        m = dict[mod]
        mangled = string.join(string.split(mod, "."), "__")
        if m.__code__:
            if debug:
                print "freezing", mod, "..."
            str = marshal.dumps(m.__code__)
            size = len(str)
            if m.__path__:
                # Indicate package by negative size
                size = -size
            done.append((mod, mangled, size))
            writecode(outfp, mangled, str)
    if debug:
        print "generating table of frozen modules"
    outfp.write(header)
    for mod, mangled, size in done:
        outfp.write('\t{"%s", M_%s, %d},\n' % (mod, mangled, size))
    outfp.write(trailer)
    outfp.write(entry_point)



# Write a C initializer for a module containing the frozen python code.
# The array is called M_<mod>.

def writecode(outfp, mod, str):
    outfp.write('static unsigned char M_%s[] = {' % mod)
    for i in range(0, len(str), 16):
        outfp.write('\n\t')
        for c in str[i:i+16]:
            outfp.write('%d,' % ord(c))
    outfp.write('\n};\n')

# Local Variables:
# indent-tabs-mode: nil
# End:
