#
# Example input file for modulator if you don't have tk.
#
# You may also have to strip some imports out of modulator to make
# it work.

import genmodule

#
# Generate code for a simple object with a method called sample

o = genmodule.object()
o.name = 'simple object'
o.abbrev = 'simp'
o.methodlist = ['sample']
o.funclist = ['new']

#
# Generate code for an object that looks numberish
#
o2 = genmodule.object()
o2.name = 'number-like object'
o2.abbrev = 'nl'
o2.typelist = ['tp_as_number']
o2.funclist = ['new', 'tp_repr', 'tp_compare']

#
# Generate code for a method with a full complement of functions,
# some methods, accessible as sequence and allowing structmember.c type
# structure access as well.
#
o3 = genmodule.object()
o3.name = 'over-the-top object'
o3.abbrev = 'ot'
o3.methodlist = ['method1', 'method2']
o3.funclist = ['new', 'tp_dealloc', 'tp_print', 'tp_getattr', 'tp_setattr',
            'tp_compare', 'tp_repr', 'tp_hash']
o3.typelist = ['tp_as_sequence', 'structure']

#
# Now generate code for a module that incorporates these object types.
# Also add the boilerplates for functions to create instances of each
# type.
#
m = genmodule.module()
m.name = 'sample'
m.abbrev = 'sample'
m.methodlist = ['newsimple', 'newnumberish', 'newott']
m.objects = [o, o2, o3]

fp = open('EXAMPLEmodule.c', 'w')
genmodule.write(fp, m)
fp.close()
