# This is an example of how the compiler could generate code for a lazy module.
# It would be enabled by a compiler directive like __lazy_module__ = 1.  This
# requires Python to support the __lazy_code__ module global.

import sys
import marshal

mod_name = 'demo_mod'
mod = type(__builtins__)(mod_name)
sys.modules[mod_name] = mod

source = '''\
a = 1
del a
a = 2
'''
c = compile(source, 'none', 'exec')
# create lazy version of 'a', could also be function def, class def, import
# use AST analysis to determine which are safe for laziness
mod.__lazy_code__ = {'a': marshal.dumps(c)}

print('mod dict', mod.__dict__)
print('mod dir()', dir(mod))
print('mod.a is', mod.a)

# now make some code that uses 'a', will fail in standard Python because
# LOAD_NAME does not look in __lazy_code__
source = '''\
print(f'a is {a}')
'''
c = compile(source, 'none', 'exec')
exec(c, mod.__dict__)

# another example of using global defs.
source = '''\
c = [a]
print(f'c is {c}')
'''
c = compile(source, 'none', 'exec')
exec(c, mod.__dict__)
