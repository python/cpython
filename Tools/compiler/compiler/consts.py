from new import * # import all the CO_xxx flags
del classobj, code, function, instance, instancemethod, module

# operation flags
OP_ASSIGN = 'OP_ASSIGN'
OP_DELETE = 'OP_DELETE'
OP_APPLY = 'OP_APPLY'

SC_LOCAL = 1
SC_GLOBAL = 2
SC_FREE = 3
SC_CELL = 4
SC_UNKNOWN = 5
