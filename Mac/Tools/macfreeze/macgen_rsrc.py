"""macgen_info - Generate PYC resource file only"""
import EasyDialogs
import py_resource
from Carbon import Res
import sys

def generate(output, module_dict, debug=0, preload=1):
    fsid = py_resource.create(output)

    for name, module in module_dict.items():
        mtype = module.gettype()
        if mtype not in ['module', 'package']:
            continue
        location = module.__file__

        if location[-4:] == '.pyc':
            # Attempt corresponding .py
            location = location[:-1]
        if location[-3:] != '.py':
            print '*** skipping', location
            continue

        id, name = py_resource.frompyfile(location, name, preload=preload,
                        ispackage=mtype=='package')
        if debug > 0:
            print 'PYC resource %5d\t%s\t%s'%(id, name, location)

    Res.CloseResFile(fsid)

def warnings(module_dict):
    problems = 0
    for name, module in module_dict.items():
        if module.gettype() not in ('builtin', 'module', 'package'):
            problems = problems + 1
            print 'Warning: %s not included: %s %s'%(name, module.gettype(), module)
    return problems
