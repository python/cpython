"""macgen_info - Generate CodeWarrior project, config source, resource file"""
import EasyDialogs
import os
import sys
import macfs
import MacOS
import macostools
import macgen_rsrc
# Note: this depends on being frozen, or on sys.path already being
# modified by macmodulefinder.
import makeconfig

TEMPLATEDIR=os.path.join(sys.prefix, ':Mac:mwerks:projects:build.macfreeze')
PROJECT_TEMPLATE=os.path.join(TEMPLATEDIR, ':frozen.prj')
CONFIG_TEMPLATE=os.path.join(TEMPLATEDIR, ':templatefrozenconfig.c')
BUNDLE_TEMPLATE=os.path.join(TEMPLATEDIR, ':frozenbundle.rsrc')

def generate(output, module_dict, debug=0, with_ifdef=0):
    problems = 0
    output_created=0
    if not os.path.exists(output):
        print 'Creating project folder', output
        os.mkdir(output)
        output_created = 1
    # Resolve aliases, if needed
    try:
        fss, dummy1, dummy2 = macfs.ResolveAliasFile(output)
    except macfs.error:
        pass
    else:
        newname = fss.as_pathname()
        if newname != output:
            if debug:
                print 'Alias', output
                print 'Resolved to', newname
            output = newname
    # Construct the filenames
    dummy, outfile = os.path.split(output)
    build, ext = os.path.splitext(outfile)
    if build == 'build' and ext[0] == '.':
        # This is probably a good name for the project
        projname = ext[1:]
    else:
        projname = 'frozenapplet.prj'
    config_name = os.path.join(output, ':macfrozenconfig.c')
    project_name = os.path.join(output, ':' + projname + '.prj')
    resource_name = os.path.join(output, ':frozenmodules.rsrc')
    bundle_name = os.path.join(output, ':frozenbundle.rsrc')

    # Fill the output folder, if needed.
    if output_created:
        # Create the project, if needed
        if not os.path.exists(project_name):
            print 'Creating project', project_name
            if not os.path.exists(PROJECT_TEMPLATE):
                print '** No template CodeWarrior project found at', PROJECT_TEMPLATE
                print '   To generate standalone Python applications from source you need'
                print '   a full source distribution. Check http://www.cwi.nl/~jack/macpython.html'
                print '   for details.'
                problems = 1
            else:
                macostools.copy(PROJECT_TEMPLATE, project_name)
                print 'A template CodeWarrior project has been copied to', project_name
                print 'It is up to you to make the following changes:'
                print '- Change the output file name'
                print '- Change the search path, unless the folder is in the python home'
                print '- Add sourcefiles/libraries for any extension modules used'
                print '- Remove unused sources, to speed up the build process'
                print '- Remove unused resource files (like tcl/tk) for a smaller binary'
                problems = 1
                macostools.copy(BUNDLE_TEMPLATE, bundle_name)
                print 'A template bundle file has also been copied to', bundle_name
                print 'You may want to adapt signature, size resource, etc'


    # Create the resource file
    macgen_rsrc.generate(resource_name, module_dict, debug=debug)

    # Create the config.c file
    if not os.path.exists(CONFIG_TEMPLATE):
        print '** No template config.c found at', PROJECT_TEMPLATE
        print '   To generate standalone Python applications from source you need'
        print '   a full source distribution. Check http://www.cwi.nl/~jack/macpython.html'
        print '   for details.'
        problems = 1
    else:
        # Find elegible modules (builtins and dynamically loaded modules)
        c_modules = []
        for module in module_dict.keys():
            if module_dict[module].gettype() in ('builtin', 'dynamic'):
                # if the module is in a package we have no choice but
                # to put it at the toplevel in the frozen application.
                if '.' in module:
                    module = module.split('.')[-1]
                c_modules.append(module)
        ifp = open(CONFIG_TEMPLATE)
        ofp = open(config_name, 'w')
        makeconfig.makeconfig(ifp, ofp, c_modules, with_ifdef)
        ifp.close()
        ofp.close()
        MacOS.SetCreatorAndType(config_name, 'CWIE', 'TEXT')

    if warnings(module_dict):
        problems = 1
    return problems

def warnings(module_dict):
    problems = 0
    for name, module in module_dict.items():
        if module.gettype() not in ('builtin', 'module', 'dynamic', 'package'):
            problems = problems + 1
            print 'Warning: %s not included: %s %s'%(name, module.gettype(), module)
    return problems
