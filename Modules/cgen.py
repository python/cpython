########################################################################
# Copyright (c) 2000, BeOpen.com.
# Copyright (c) 1995-2000, Corporation for National Research Initiatives.
# Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
# All rights reserved.
#
# See the file "Misc/COPYRIGHT" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
########################################################################

# Python script to parse cstubs file for gl and generate C stubs.
# usage: python cgen.py <cstubs >glmodule.c
#
# NOTE: You  must first make a python binary without the "GL" option
#       before you can run this, when building Python for the first time.
#       See comments in the Makefile.
#
# XXX BUG return arrays generate wrong code
# XXX need to change error returns into gotos to free mallocked arrays


import string
import sys


# Function to print to stderr
#
def err(*args):
    savestdout = sys.stdout
    try:
        sys.stdout = sys.stderr
        for i in args:
            print i,
        print
    finally:
        sys.stdout = savestdout


# The set of digits that form a number
#
digits = '0123456789'


# Function to extract a string of digits from the front of the string.
# Returns the leading string of digits and the remaining string.
# If no number is found, returns '' and the original string.
#
def getnum(s):
    n = ''
    while s and s[0] in digits:
        n = n + s[0]
        s = s[1:]
    return n, s


# Function to check if a string is a number
#
def isnum(s):
    if not s: return False
    for c in s:
        if not c in digits: return False
    return True


# Allowed function return types
#
return_types = ['void', 'short', 'long']


# Allowed function argument types
#
arg_types = ['char', 'string', 'short', 'u_short', 'float', 'long', 'double']


# Need to classify arguments as follows
#       simple input variable
#       simple output variable
#       input array
#       output array
#       input giving size of some array
#
# Array dimensions can be specified as follows
#       constant
#       argN
#       constant * argN
#       retval
#       constant * retval
#
# The dimensions given as constants * something are really
# arrays of points where points are 2- 3- or 4-tuples
#
# We have to consider three lists:
#       python input arguments
#       C stub arguments (in & out)
#       python output arguments (really return values)
#
# There is a mapping from python input arguments to the input arguments
# of the C stub, and a further mapping from C stub arguments to the
# python return values


# Exception raised by checkarg() and generate()
#
arg_error = 'bad arg'


# Function to check one argument.
# Arguments: the type and the arg "name" (really mode plus subscript).
# Raises arg_error if something's wrong.
# Return type, mode, factor, rest of subscript; factor and rest may be empty.
#
def checkarg(type, arg):
    #
    # Turn "char *x" into "string x".
    #
    if type == 'char' and arg[0] == '*':
        type = 'string'
        arg = arg[1:]
    #
    # Check that the type is supported.
    #
    if type not in arg_types:
        raise arg_error, ('bad type', type)
    if type[:2] == 'u_':
        type = 'unsigned ' + type[2:]
    #
    # Split it in the mode (first character) and the rest.
    #
    mode, rest = arg[:1], arg[1:]
    #
    # The mode must be 's' for send (= input) or 'r' for return argument.
    #
    if mode not in ('r', 's'):
        raise arg_error, ('bad arg mode', mode)
    #
    # Is it a simple argument: if so, we are done.
    #
    if not rest:
        return type, mode, '', ''
    #
    # Not a simple argument; must be an array.
    # The 'rest' must be a subscript enclosed in [ and ].
    # The subscript must be one of the following forms,
    # otherwise we don't handle it (where N is a number):
    #       N
    #       argN
    #       retval
    #       N*argN
    #       N*retval
    #
    if rest[:1] <> '[' or rest[-1:] <> ']':
        raise arg_error, ('subscript expected', rest)
    sub = rest[1:-1]
    #
    # Is there a leading number?
    #
    num, sub = getnum(sub)
    if num:
        # There is a leading number
        if not sub:
            # The subscript is just a number
            return type, mode, num, ''
        if sub[:1] == '*':
            # There is a factor prefix
            sub = sub[1:]
        else:
            raise arg_error, ('\'*\' expected', sub)
    if sub == 'retval':
        # size is retval -- must be a reply argument
        if mode <> 'r':
            raise arg_error, ('non-r mode with [retval]', mode)
    elif not isnum(sub) and (sub[:3] <> 'arg' or not isnum(sub[3:])):
        raise arg_error, ('bad subscript', sub)
    #
    return type, mode, num, sub


# List of functions for which we have generated stubs
#
functions = []


# Generate the stub for the given function, using the database of argument
# information build by successive calls to checkarg()
#
def generate(type, func, database):
    #
    # Check that we can handle this case:
    # no variable size reply arrays yet
    #
    n_in_args = 0
    n_out_args = 0
    #
    for a_type, a_mode, a_factor, a_sub in database:
        if a_mode == 's':
            n_in_args = n_in_args + 1
        elif a_mode == 'r':
            n_out_args = n_out_args + 1
        else:
            # Can't happen
            raise arg_error, ('bad a_mode', a_mode)
        if (a_mode == 'r' and a_sub) or a_sub == 'retval':
            err('Function', func, 'too complicated:',
                a_type, a_mode, a_factor, a_sub)
            print '/* XXX Too complicated to generate code for */'
            return
    #
    functions.append(func)
    #
    # Stub header
    #
    print
    print 'static PyObject *'
    print 'gl_' + func + '(self, args)'
    print '\tPyObject *self;'
    print '\tPyObject *args;'
    print '{'
    #
    # Declare return value if any
    #
    if type <> 'void':
        print '\t' + type, 'retval;'
    #
    # Declare arguments
    #
    for i in range(len(database)):
        a_type, a_mode, a_factor, a_sub = database[i]
        print '\t' + a_type,
        brac = ket = ''
        if a_sub and not isnum(a_sub):
            if a_factor:
                brac = '('
                ket = ')'
            print brac + '*',
        print 'arg' + repr(i+1) + ket,
        if a_sub and isnum(a_sub):
            print '[', a_sub, ']',
        if a_factor:
            print '[', a_factor, ']',
        print ';'
    #
    # Find input arguments derived from array sizes
    #
    for i in range(len(database)):
        a_type, a_mode, a_factor, a_sub = database[i]
        if a_mode == 's' and a_sub[:3] == 'arg' and isnum(a_sub[3:]):
            # Sending a variable-length array
            n = eval(a_sub[3:])
            if 1 <= n <= len(database):
                b_type, b_mode, b_factor, b_sub = database[n-1]
                if b_mode == 's':
                    database[n-1] = b_type, 'i', a_factor, repr(i)
                    n_in_args = n_in_args - 1
    #
    # Assign argument positions in the Python argument list
    #
    in_pos = []
    i_in = 0
    for i in range(len(database)):
        a_type, a_mode, a_factor, a_sub = database[i]
        if a_mode == 's':
            in_pos.append(i_in)
            i_in = i_in + 1
        else:
            in_pos.append(-1)
    #
    # Get input arguments
    #
    for i in range(len(database)):
        a_type, a_mode, a_factor, a_sub = database[i]
        if a_type[:9] == 'unsigned ':
            xtype = a_type[9:]
        else:
            xtype = a_type
        if a_mode == 'i':
            #
            # Implicit argument;
            # a_factor is divisor if present,
            # a_sub indicates which arg (`database index`)
            #
            j = eval(a_sub)
            print '\tif',
            print '(!geti' + xtype + 'arraysize(args,',
            print repr(n_in_args) + ',',
            print repr(in_pos[j]) + ',',
            if xtype <> a_type:
                print '('+xtype+' *)',
            print '&arg' + repr(i+1) + '))'
            print '\t\treturn NULL;'
            if a_factor:
                print '\targ' + repr(i+1),
                print '= arg' + repr(i+1),
                print '/', a_factor + ';'
        elif a_mode == 's':
            if a_sub and not isnum(a_sub):
                # Allocate memory for varsize array
                print '\tif ((arg' + repr(i+1), '=',
                if a_factor:
                    print '('+a_type+'(*)['+a_factor+'])',
                print 'PyMem_NEW(' + a_type, ',',
                if a_factor:
                    print a_factor, '*',
                print a_sub, ')) == NULL)'
                print '\t\treturn PyErr_NoMemory();'
            print '\tif',
            if a_factor or a_sub: # Get a fixed-size array array
                print '(!geti' + xtype + 'array(args,',
                print repr(n_in_args) + ',',
                print repr(in_pos[i]) + ',',
                if a_factor: print a_factor,
                if a_factor and a_sub: print '*',
                if a_sub: print a_sub,
                print ',',
                if (a_sub and a_factor) or xtype <> a_type:
                    print '('+xtype+' *)',
                print 'arg' + repr(i+1) + '))'
            else: # Get a simple variable
                print '(!geti' + xtype + 'arg(args,',
                print repr(n_in_args) + ',',
                print repr(in_pos[i]) + ',',
                if xtype <> a_type:
                    print '('+xtype+' *)',
                print '&arg' + repr(i+1) + '))'
            print '\t\treturn NULL;'
    #
    # Begin of function call
    #
    if type <> 'void':
        print '\tretval =', func + '(',
    else:
        print '\t' + func + '(',
    #
    # Argument list
    #
    for i in range(len(database)):
        if i > 0: print ',',
        a_type, a_mode, a_factor, a_sub = database[i]
        if a_mode == 'r' and not a_factor:
            print '&',
        print 'arg' + repr(i+1),
    #
    # End of function call
    #
    print ');'
    #
    # Free varsize arrays
    #
    for i in range(len(database)):
        a_type, a_mode, a_factor, a_sub = database[i]
        if a_mode == 's' and a_sub and not isnum(a_sub):
            print '\tPyMem_DEL(arg' + repr(i+1) + ');'
    #
    # Return
    #
    if n_out_args:
        #
        # Multiple return values -- construct a tuple
        #
        if type <> 'void':
            n_out_args = n_out_args + 1
        if n_out_args == 1:
            for i in range(len(database)):
                a_type, a_mode, a_factor, a_sub = database[i]
                if a_mode == 'r':
                    break
            else:
                raise arg_error, 'expected r arg not found'
            print '\treturn',
            print mkobject(a_type, 'arg' + repr(i+1)) + ';'
        else:
            print '\t{ PyObject *v = PyTuple_New(',
            print n_out_args, ');'
            print '\t  if (v == NULL) return NULL;'
            i_out = 0
            if type <> 'void':
                print '\t  PyTuple_SetItem(v,',
                print repr(i_out) + ',',
                print mkobject(type, 'retval') + ');'
                i_out = i_out + 1
            for i in range(len(database)):
                a_type, a_mode, a_factor, a_sub = database[i]
                if a_mode == 'r':
                    print '\t  PyTuple_SetItem(v,',
                    print repr(i_out) + ',',
                    s = mkobject(a_type, 'arg' + repr(i+1))
                    print s + ');'
                    i_out = i_out + 1
            print '\t  return v;'
            print '\t}'
    else:
        #
        # Simple function return
        # Return None or return value
        #
        if type == 'void':
            print '\tPy_INCREF(Py_None);'
            print '\treturn Py_None;'
        else:
            print '\treturn', mkobject(type, 'retval') + ';'
    #
    # Stub body closing brace
    #
    print '}'


# Subroutine to return a function call to mknew<type>object(<arg>)
#
def mkobject(type, arg):
    if type[:9] == 'unsigned ':
        type = type[9:]
        return 'mknew' + type + 'object((' + type + ') ' + arg + ')'
    return 'mknew' + type + 'object(' + arg + ')'


defined_archs = []

# usage: cgen [ -Dmach ... ] [ file ]
for arg in sys.argv[1:]:
    if arg[:2] == '-D':
        defined_archs.append(arg[2:])
    else:
        # Open optional file argument
        sys.stdin = open(arg, 'r')


# Input line number
lno = 0


# Input is divided in two parts, separated by a line containing '%%'.
#       <part1>         -- literally copied to stdout
#       <part2>         -- stub definitions

# Variable indicating the current input part.
#
part = 1

# Main loop over the input
#
while 1:
    try:
        line = raw_input()
    except EOFError:
        break
    #
    lno = lno+1
    words = string.split(line)
    #
    if part == 1:
        #
        # In part 1, copy everything literally
        # except look for a line of just '%%'
        #
        if words == ['%%']:
            part = part + 1
        else:
            #
            # Look for names of manually written
            # stubs: a single percent followed by the name
            # of the function in Python.
            # The stub name is derived by prefixing 'gl_'.
            #
            if words and words[0][0] == '%':
                func = words[0][1:]
                if (not func) and words[1:]:
                    func = words[1]
                if func:
                    functions.append(func)
            else:
                print line
        continue
    if not words:
        continue                # skip empty line
    elif words[0] == 'if':
        # if XXX rest
        # if !XXX rest
        if words[1][0] == '!':
            if words[1][1:] in defined_archs:
                continue
        elif words[1] not in defined_archs:
            continue
        words = words[2:]
    if words[0] == '#include':
        print line
    elif words[0][:1] == '#':
        pass                    # ignore comment
    elif words[0] not in return_types:
        err('Line', lno, ': bad return type :', words[0])
    elif len(words) < 2:
        err('Line', lno, ': no funcname :', line)
    else:
        if len(words) % 2 <> 0:
            err('Line', lno, ': odd argument list :', words[2:])
        else:
            database = []
            try:
                for i in range(2, len(words), 2):
                    x = checkarg(words[i], words[i+1])
                    database.append(x)
                print
                print '/*',
                for w in words: print w,
                print '*/'
                generate(words[0], words[1], database)
            except arg_error, msg:
                err('Line', lno, ':', msg)


print
print 'static struct PyMethodDef gl_methods[] = {'
for func in functions:
    print '\t{"' + func + '", gl_' + func + '},'
print '\t{NULL, NULL} /* Sentinel */'
print '};'
print
print 'void'
print 'initgl()'
print '{'
print '\t(void) Py_InitModule("gl", gl_methods);'
print '}'
