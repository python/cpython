from distutils.core import Extension, setup
from distutils import sysconfig
import os

SRCDIR="../.."

def find_file(filename, std_dirs, paths):
    """Searches for the directory where a given file is located,
    and returns a possibly-empty list of additional directories, or None
    if the file couldn't be found at all.

    'filename' is the name of a file, such as readline.h or libcrypto.a.
    'std_dirs' is the list of standard system directories; if the
        file is found in one of them, no additional directives are needed.
    'paths' is a list of additional locations to check; if the file is
        found in one of them, the resulting list will contain the directory.
    """

    # Check the standard locations
    for dir in std_dirs:
        f = os.path.join(dir, filename)
        if os.path.exists(f): return []

    # Check the additional directories
    for dir in paths:
        f = os.path.join(dir, filename)
        if os.path.exists(f):
            return [dir]

    # Not found anywhere
    return None

def find_library_file(compiler, libname, std_dirs, paths):
    filename = compiler.library_filename(libname, lib_type='shared')
    result = find_file(filename, std_dirs, paths)
    if result is not None: return result

    filename = compiler.library_filename(libname, lib_type='static')
    result = find_file(filename, std_dirs, paths)
    return result
    
def waste_Extension():
    waste_incs = find_file("WASTE.h", [], 
            ['../'*n + 'waste/C_C++ Headers' for n in (0,1,2,3,4)])
    if waste_incs != None:
        waste_libs = [os.path.join(os.path.split(waste_incs[0])[0], "Static Libraries")]
        srcdir = SRCDIR
        return [ Extension('waste',
                       [os.path.join(srcdir, d) for d in 
                        'Mac/Modules/waste/wastemodule.c',
                        'Mac/Wastemods/WEObjectHandlers.c',
                        'Mac/Wastemods/WETabHooks.c',
                        'Mac/Wastemods/WETabs.c'
                       ],
                       include_dirs = waste_incs + [
                        os.path.join(srcdir, 'Mac/Include'),
                        os.path.join(srcdir, 'Mac/Wastemods')
                       ],
                       library_dirs = waste_libs,
                       libraries = ['WASTE'],
                       extra_link_args = ['-framework', 'Carbon'],
        ) ]
    return []

setup(name="MacPython for Jaguar extensions", version="2.2",
    ext_modules=[
        Extension("OverrideFrom23._AE",
            [SRCDIR + "/Mac/Modules/ae/_AEmodule.c"],
            include_dirs=[SRCDIR+"/Mac/Include"],
            extra_link_args=['-framework', 'Carbon']),
        Extension("OverrideFrom23._Res",
            [SRCDIR + "/Mac/Modules/res/_Resmodule.c"],
            include_dirs=[SRCDIR+"/Mac/Include"],
            extra_link_args=['-framework', 'Carbon']),
        Extension('_Help', 
            [SRCDIR + '/Mac/Modules/help/_Helpmodule.c'],
            include_dirs=[SRCDIR+"/Mac/Include"],
            extra_link_args=['-framework', 'Carbon']),
        Extension('_Scrap', 
            [SRCDIR + '/Mac/Modules/scrap/_Scrapmodule.c'],
            include_dirs=[SRCDIR+"/Mac/Include"],
            extra_link_args=['-framework', 'Carbon']),
        ] + 
        waste_Extension()
    )