#! /usr/bin/env python

"""Freeze a Python script into a binary.

usage: freeze [options...] script [module]...

Options:

-p prefix:    This is the prefix used when you ran ``make install''
              in the Python build directory.
              (If you never ran this, freeze won't work.)
              The default is whatever sys.prefix evaluates to.
              It can also be the top directory of the Python source
              tree; then -P must point to the build tree.

-P exec_prefix: Like -p but this is the 'exec_prefix', used to
                install objects etc.  The default is whatever sys.exec_prefix
                evaluates to, or the -p argument if given.
                If -p points to the Python source tree, -P must point
                to the build tree, if different.

-e extension: A directory containing additional .o files that
              may be used to resolve modules.  This directory
              should also have a Setup file describing the .o files.
              More than one -e option may be given.

-o dir:       Directory where the output files are created; default '.'.

-m:           Additional arguments are module names instead of filenames.

-d:           Debugging mode for the module finder.

-q:           Make the module finder totally quiet.

-h:           Print this help message.

-w:           Toggle Windows (NT or 95) behavior.
              (For debugging only -- on a win32 platform, win32 behaviour
              is automatic.)

-s subsystem: Specify the subsystem; 'windows' or 'console' (default).
              (For Windows only.)

Arguments:

script:       The Python script to be executed by the resulting binary.

module ...:   Additional Python modules (referenced by pathname)
              that will be included in the resulting binary.  These
              may be .py or .pyc files.  If -m is specified, these are
              module names that are search in the path instead.

NOTES:

In order to use freeze successfully, you must have built Python and
installed it ("make install").

The script should not use modules provided only as shared libraries;
if it does, the resulting binary is not self-contained.
"""


# Import standard modules

import cmp
import getopt
import os
import string
import sys
import addpack


# Import the freeze-private modules

import checkextensions
import modulefinder
import makeconfig
import makefreeze
import makemakefile
import parsesetup


# Main program

def main():
    # overridable context
    prefix = None                       # settable with -p option
    exec_prefix = None                  # settable with -P option
    extensions = []
    path = sys.path[:]
    modargs = 0
    debug = 1
    odir = ''
    win = sys.platform[:3] == 'win'

    # modules that are imported by the Python runtime
    implicits = ["site", "exceptions"]

    # output files
    frozen_c = 'frozen.c'
    config_c = 'config.c'
    target = 'a.out'                    # normally derived from script name
    makefile = 'Makefile'
    subsystem = 'console'

    # parse command line
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'de:hmo:p:P:qs:w')
    except getopt.error, msg:
        usage('getopt error: ' + str(msg))

    # proces option arguments
    for o, a in opts:
        if o == '-h':
            print __doc__
            return
        if o == '-d':
            debug = debug + 1
        if o == '-e':
            extensions.append(a)
        if o == '-m':
            modargs = 1
        if o == '-o':
            odir = a
        if o == '-p':
            prefix = a
        if o == '-P':
            exec_prefix = a
        if o == '-q':
            debug = 0
        if o == '-w':
            win = not win
        if o == '-s':
            if not win:
                usage("-s subsystem option only on Windows")
            subsystem = a

    # default prefix and exec_prefix
    if not exec_prefix:
        if prefix:
            exec_prefix = prefix
        else:
            exec_prefix = sys.exec_prefix
    if not prefix:
        prefix = sys.prefix

    # determine whether -p points to the Python source tree
    ishome = os.path.exists(os.path.join(prefix, 'Python', 'ceval.c'))

    # locations derived from options
    version = sys.version[:3]
    if ishome:
        print "(Using Python source directory)"
        binlib = exec_prefix
        incldir = os.path.join(prefix, 'Include')
        config_c_in = os.path.join(prefix, 'Modules', 'config.c.in')
        frozenmain_c = os.path.join(prefix, 'Python', 'frozenmain.c')
        makefile_in = os.path.join(exec_prefix, 'Modules', 'Makefile')
    else:
        binlib = os.path.join(exec_prefix,
                              'lib', 'python%s' % version, 'config')
        incldir = os.path.join(prefix, 'include', 'python%s' % version)
        config_c_in = os.path.join(binlib, 'config.c.in')
        frozenmain_c = os.path.join(binlib, 'frozenmain.c')
        makefile_in = os.path.join(binlib, 'Makefile')
    supp_sources = []
    defines = []
    includes = ['-I' + incldir, '-I' + binlib]

    # sanity check of directories and files
    for dir in [prefix, exec_prefix, binlib, incldir] + extensions:
        if not os.path.exists(dir):
            usage('needed directory %s not found' % dir)
        if not os.path.isdir(dir):
            usage('%s: not a directory' % dir)
    if win:
        files = supp_sources
    else:
        files = [config_c_in, makefile_in] + supp_sources
    for file in supp_sources:
        if not os.path.exists(file):
            usage('needed file %s not found' % file)
        if not os.path.isfile(file):
            usage('%s: not a plain file' % file)
    if not win:
        for dir in extensions:
            setup = os.path.join(dir, 'Setup')
            if not os.path.exists(setup):
                usage('needed file %s not found' % setup)
            if not os.path.isfile(setup):
                usage('%s: not a plain file' % setup)

    # check that enough arguments are passed
    if not args:
        usage('at least one filename argument required')

    # check that file arguments exist
    for arg in args:
        if arg == '-m':
            break
        if not os.path.exists(arg):
            usage('argument %s not found' % arg)
        if not os.path.isfile(arg):
            usage('%s: not a plain file' % arg)
        if modargs:
            break

    # process non-option arguments
    scriptfile = args[0]
    modules = args[1:]

    # derive target name from script name
    base = os.path.basename(scriptfile)
    base, ext = os.path.splitext(base)
    if base:
        if base != scriptfile:
            target = base
        else:
            target = base + '.bin'

    # handle -o option
    base_frozen_c = frozen_c
    base_config_c = config_c
    base_target = target
    if odir and not os.path.isdir(odir):
        try:
            os.mkdir(odir)
            print "Created output directory", odir
        except os.error, msg:
            usage('%s: mkdir failed (%s)' % (odir, str(msg)))
    if odir:
        frozen_c = os.path.join(odir, frozen_c)
        config_c = os.path.join(odir, config_c)
        target = os.path.join(odir, target)
        makefile = os.path.join(odir, makefile)

    # Actual work starts here...

    # collect all modules of the program
    dir = os.path.dirname(scriptfile)
    path[0] = dir
    mf = modulefinder.ModuleFinder(path, debug)
    for mod in implicits:
        mf.import_hook(mod)
    for mod in modules:
        if mod == '-m':
            modargs = 1
            continue
        if modargs:
            if mod[-2:] == '.*':
                mf.import_hook(mod[:-2], None, ["*"])
            else:
                mf.import_hook(mod)
        else:
            mf.load_file(mod)
    mf.run_script(scriptfile)
    if debug > 0:
        mf.report()
        print
    dict = mf.modules

    # generate output for frozen modules
    backup = frozen_c + '~'
    try:
        os.rename(frozen_c, backup)
    except os.error:
        backup = None
    outfp = open(frozen_c, 'w')
    try:
        makefreeze.makefreeze(outfp, dict, debug)
        if win and subsystem == 'windows':
            import winmakemakefile
            outfp.write(winmakemakefile.WINMAINTEMPLATE)
    finally:
        outfp.close()
    if backup:
        if cmp.cmp(backup, frozen_c):
            sys.stderr.write('%s not changed, not written\n' % frozen_c)
            os.unlink(frozen_c)
            os.rename(backup, frozen_c)

    # windows gets different treatment
    if win:
        # Taking a shortcut here...
        import winmakemakefile
        outfp = open(makefile, 'w')
        try:
            winmakemakefile.makemakefile(outfp,
                                         locals(),
                                         [frozenmain_c,
                                          os.path.basename(frozen_c)],
                                         os.path.basename(target))
        finally:
            outfp.close()
        return

    # generate config.c and Makefile
    builtins = []
    unknown = []
    mods = dict.keys()
    mods.sort()
    for mod in mods:
        if dict[mod].__code__:
            continue
        if not dict[mod].__file__:
            builtins.append(mod)
        else:
            unknown.append(mod)

    addfiles = []
    if unknown:
        addfiles, addmods = \
                  checkextensions.checkextensions(unknown, extensions)
        for mod in addmods:
            unknown.remove(mod)
        builtins = builtins + addmods
    if unknown:
        sys.stderr.write('Warning: unknown modules remain: %s\n' %
                         string.join(unknown))

    builtins.sort()
    infp = open(config_c_in)
    backup = config_c + '~'
    try:
        os.rename(config_c, backup)
    except os.error:
        backup = None
    outfp = open(config_c, 'w')
    try:
        makeconfig.makeconfig(infp, outfp, builtins)
    finally:
        outfp.close()
    infp.close()
    if backup:
        if cmp.cmp(backup, config_c):
            sys.stderr.write('%s not changed, not written\n' % config_c)
            os.unlink(config_c)
            os.rename(backup, config_c)

    cflags = defines + includes + ['$(OPT)']
    libs = [os.path.join(binlib, 'libpython$(VERSION).a')]

    somevars = {}
    if os.path.exists(makefile_in):
        makevars = parsesetup.getmakevars(makefile_in)
    for key in makevars.keys():
        somevars[key] = makevars[key]

    somevars['CFLAGS'] = string.join(cflags) # override
    files = ['$(OPT)', '$(LDFLAGS)', base_config_c, base_frozen_c] + \
            supp_sources +  addfiles + libs + \
            ['$(MODLIBS)', '$(LIBS)', '$(SYSLIBS)']

    backup = makefile + '~'
    if os.path.exists(makefile):
        try:
            os.unlink(backup)
        except os.error:
            pass
    try:
        os.rename(makefile, backup)
    except os.error:
        backup = None
    outfp = open(makefile, 'w')
    try:
        makemakefile.makemakefile(outfp, somevars, files, base_target)
    finally:
        outfp.close()
    if backup:
        if not cmp.cmp(backup, makefile):
            print 'previous Makefile saved as', backup
        else:
            sys.stderr.write('%s not changed, not written\n' % makefile)
            os.unlink(makefile)
            os.rename(backup, makefile)

    # Done!

    if odir:
        print 'Now run "make" in', odir,
        print 'to build the target:', base_target
    else:
        print 'Now run "make" to build the target:', base_target


# Print usage message and exit

def usage(msg):
    sys.stdout = sys.stderr
    print "Error:", msg
    print "Use ``%s -h'' for help" % sys.argv[0]
    sys.exit(2)


main()

# Local Variables:
# indent-tabs-mode: nil
# End:
