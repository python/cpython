#!/usr/bin/env/python

# perfect_hash.py
#        
# Outputs C code for a minimal perfect hash.
# The hash is produced using the algorithm described in
# "Optimal algorithms for minimal perfect hashing",
# G. Havas, B.S. Majewski.  Available as a technical report
# from the CS department, University of Queensland
# (ftp://ftp.cs.uq.oz.au/).
#
# This is a modified version of Andrew Kuchling's code
# (http://starship.python.net/crew/amk/python/code/perfect-hash.html)
# and generates C fragments suitable for compilation as a Python
# extension module.
#

# Difference between this algorithm and gperf:
# Gperf will complete in finite time with a successful function,
# or by giving up.
# This algorithm may never complete, although it is extremely likely
# when c >= 2.

# The algorithm works like this:
#   0) You have K keys, that you want to perfectly hash to a bunch
#      of hash values.
#
#   1) Choose a number N larger than K.  This is the number of
#      vertices in a graph G, and also the size of the resulting table.
#
#   2) Pick two random hash functions f1, f2, that output values from
#      0...N-1.
#
#   3) for key in keys:
#          h1 = f1(key) ; h2 = f2(key)
#          Draw an edge between vertices h1 and h2 of the graph.
#          Associate the desired hash value with that edge.
#
#   4) Check if G is acyclic; if not, go back to step 1 and pick a bigger N.
#
#   5) Assign values to each vertex such that, for each edge, you can
#      add the values for the two vertices and get the desired value
#      for that edge -- which is the desired hash key.  This task is
#      dead easy, because the graph is acyclic.  This is done by
#      picking a vertex V, and assigning it a value of 0.  You then do a
#      depth-first search, assigning values to new vertices so that
#      they sum up properly.
#
#   6) f1, f2, and G now make up your perfect hash function.


import sys, whrandom, string
import pprint
import perfhash
import time

class Hash:
    """Random hash function
    For simplicity and speed, this doesn't implement any byte-level hashing
    scheme.  Instead, a random string is generated and prefixing to
    str(key), and then Python's hashing function is used."""
        
    def __init__(self, N, caseInsensitive=0):
        self.N = N
        junk = ""
        for i in range(10):
            junk = junk + whrandom.choice(string.letters + string.digits)
        self.junk = junk
        self.caseInsensitive = caseInsensitive
        self.seed = perfhash.calcSeed(junk)

    def __call__(self, key):
      key = str(key)
      if self.caseInsensitive:
        key = string.upper(key)
      x = perfhash.hash(self.seed, len(self.junk), key, self.N) 
      return x
      
    def generate_code(self):
      s = """{
    register int len;
    register unsigned char *p;
    register unsigned long x;

    len = cch;
    p = (unsigned char *) key;
    x = %(junkSeed)s;
    while (--len >= 0)
    {   
        /* (1000003 * x) ^ toupper(*(p++)) 
         * translated to handle > 32 bit longs 
         */
        x = (0xf4243 * x);
        x = x & 0xFFFFFFFF;
        x = x ^ """ % \
      {
        "lenJunk" : len(self.junk),
        "junkSeed" : hex(self.seed),
      }
      
      if self.caseInsensitive:
        s = s + "toupper(*(p++));"
      else:
        s = s + "*(p++);"
      s = s + """
    }
    x ^= cch + %(lenJunk)d;
    if (x == 0xFFFFFFFF)
        x = 0xfffffffe;
    if (x & 0x80000000) 
    {
        /* Emulate 32-bit signed (2's complement) modulo operation */
        x = (~x & 0xFFFFFFFF) + 1;
        x %%= k_cHashElements;
        if (x != 0)
        {
            x = x + (~k_cHashElements & 0xFFFFFFFF) + 1;
            x = (~x & 0xFFFFFFFF) + 1;
        }
    }
    else
        x %%= k_cHashElements;
    return x;
}
""" % { "lenJunk" : len(self.junk),
        "junkSeed" : hex(self.seed), }
      return s
        
WHITE, GREY, BLACK = 0,1,2
class Graph:
    """Graph class.  This class isn't particularly efficient or general,
    and only has the features I needed to implement this algorithm.

    num_vertices -- number of vertices
    edges -- maps 2-tuples of vertex numbers to the value for this
             edge.  If there's an edge between v1 and v2 (v1<v2),
             (v1,v2) is a key and the value is the edge's value.
    reachable_list -- maps a vertex V to the list of vertices
                      to which V is connected by edges.  Used
                      for traversing the graph.
    values -- numeric value for each vertex
    """
    
    def __init__(self, num_vertices):
        self.num_vertices = num_vertices
        self.edges = {}
        self.reachable_list = {}
        self.values = [-1] * num_vertices

    def connect(self, vertex1, vertex2, value):
        """Connect 'vertex1' and 'vertex2' with an edge, with associated
        value 'value'"""
        
        if vertex1 > vertex2: vertex1, vertex2 = vertex2, vertex1
        if self.edges.has_key( (vertex1, vertex2) ):
            raise ValueError, 'Collision: vertices already connected'
        self.edges[ (vertex1, vertex2) ] = value

        # Add vertices to each other's reachable list
        if not self.reachable_list.has_key( vertex1 ):
            self.reachable_list[ vertex1 ] = [vertex2]
        else:
            self.reachable_list[vertex1].append(vertex2)

        if not self.reachable_list.has_key( vertex2 ):
            self.reachable_list[ vertex2 ] = [vertex1]
        else:
            self.reachable_list[vertex2].append(vertex1)

    def get_edge_value(self, vertex1, vertex2):
        """Retrieve the value corresponding to the edge between
        'vertex1' and 'vertex2'.  Raises KeyError if no such edge"""
        if vertex1 > vertex2:
            vertex1, vertex2 = vertex2, vertex1
        return self.edges[ (vertex1, vertex2) ] 
        
    def is_acyclic(self):
        "Returns true if the graph is acyclic, otherwise false"

        # This is done by doing a depth-first search of the graph;
        # painting each vertex grey and then black.  If the DFS
        # ever finds a vertex that isn't white, there's a cycle.
        colour = {}
        for i in range(self.num_vertices): colour[i] = WHITE

        # Loop over all vertices, taking white ones as starting
        # points for a traversal.
        for i in range(self.num_vertices):
            if colour[i] == WHITE:
                
                # List of vertices to visit
                visit_list = [ (None,i) ]

                # Do a DFS
                while visit_list:
                    # Colour this vertex grey.
                    parent, vertex = visit_list[0] ; del visit_list[0]
                    colour[vertex] = GREY

                    # Make copy of list of neighbours, removing the vertex
                    # we arrived here from.
                    neighbours = self.reachable_list.get(vertex, []) [:]
                    if parent in neighbours: neighbours.remove( parent )

                    for neighbour in neighbours:
                        if colour[neighbour] == WHITE:
                            visit_list.insert(0, (vertex, neighbour) )
                        elif colour[neighbour] != WHITE:
                            # Aha!  Already visited this node,
                            # so the graph isn't acyclic.
                            return 0

                    colour[vertex] = BLACK

        # We got through, so the graph is acyclic.
        return 1

    def assign_values(self):
        """Compute values for each vertex, so that they sum up
        properly to the associated value for each edge."""

        # Also done with a DFS; I simply copied the DFS code
        # from is_acyclic().  (Should generalize the logic so
        # one function could be used from both methods,
        # but I couldn't be bothered.)
        
        colour = {}
        for i in range(self.num_vertices): colour[i] = WHITE

        # Loop over all vertices, taking white ones as starting
        # points for a traversal.
        for i in range(self.num_vertices):
            if colour[i] == WHITE:
                # Set this vertex's value, arbitrarily, to zero.
                self.set_vertex_value( i, 0 )

                # List of vertices to visit
                visit_list = [ (None,i) ]

                # Do a DFS
                while visit_list:
                    # Colour this vertex grey.
                    parent, vertex = visit_list[0] ; del visit_list[0]
                    colour[vertex] = GREY

                    # Make copy of list of neighbours, removing the vertex
                    # we arrived here from.
                    neighbours = self.reachable_list.get(vertex, []) [:]
                    if parent in neighbours: neighbours.remove( parent )

                    for neighbour in self.reachable_list.get(vertex, []):
                        edge_value = self.get_edge_value( vertex, neighbour )
                        if colour[neighbour] == WHITE:
                            visit_list.insert(0, (vertex, neighbour) )

                            # Set new vertex's value to the desired
                            # edge value, minus the value of the
                            # vertex we came here from.
                            new_val = (edge_value -
                                       self.get_vertex_value( vertex ) )
                            self.set_vertex_value( neighbour,
                                                   new_val % self.num_vertices)

                    colour[vertex] = BLACK
                    
        # Returns nothing
        return
    
    def __getitem__(self, index):
        if index < self.num_vertices: return index
        raise IndexError
    
    def get_vertex_value(self, vertex):
        "Get value for a vertex"
        return self.values[ vertex ]

    def set_vertex_value(self, vertex, value):
        "Set value for a vertex"
        self.values[ vertex ] = value
    
    def generate_code(self, out, width = 70):
        "Return nicely formatted table"
        out.write("{ ")
        pos = 0
        for v in self.values:
            v=str(v)+', '
            out.write(v)
            pos = pos + len(v) + 1
            if pos > width: out.write('\n '); pos = 0
        out.write('};\n')


class PerfectHash:
  def __init__(self, cchMax, f1, f2, G, cHashElements, cKeys, maxHashValue):
    self.cchMax = cchMax
    self.f1 = f1
    self.f2 = f2
    self.G  = G
    self.cHashElements = cHashElements
    self.cKeys = cKeys
    # determine the necessary type for storing our hash function
    # helper table:
    self.type = self.determineType(maxHashValue)

  def generate_header(self, structName):
    header = """
#include "Python.h"
#include <stdlib.h>

/* --- C API ----------------------------------------------------*/
/* C API for usage by other Python modules */
typedef struct %(structName)s
{
    unsigned long cKeys;
    unsigned long cchMax;
    unsigned long (*hash)(const char *key, unsigned int cch);
    const void *(*getValue)(unsigned long iKey);
} %(structName)s;
""" % { "structName" : structName }
    return header

  def determineType(self, maxHashValue):
    if maxHashValue <= 255:
      return "unsigned char"
    elif maxHashValue <= 65535:
      return "unsigned short"
    else:
      # Take the cheesy way out... 
      return "unsigned long"

  def generate_code(self, moduleName, dataArrayName, dataArrayType, structName):
    # Output C code for the hash functions and tables
    code = """
/*
 * The hash is produced using the algorithm described in
 * "Optimal algorithms for minimal perfect hashing",
 * G. Havas, B.S. Majewski.  Available as a technical report
 * from the CS department, University of Queensland
 * (ftp://ftp.cs.uq.oz.au/).
 *
 * Generated using a heavily tweaked version of Andrew Kuchling's
 * perfect_hash.py: 
 * http://starship.python.net/crew/amk/python/code/perfect-hash.html
 *
 * Generated on: %s
 */
""" % time.ctime(time.time())
    # MSVC SP3 was complaining when I actually used a global constant
    code = code + """
#define k_cHashElements %i
#define k_cchMaxKey  %d
#define k_cKeys  %i

""" % (self.cHashElements, self.cchMax, self.cKeys)
    
    code = code + """
staticforward const %s G[k_cHashElements]; 
staticforward const %s %s[k_cKeys];   
""" % (self.type, dataArrayType, dataArrayName)
    
    code = code + """
static long f1(const char *key, unsigned int cch)
"""
    code = code + self.f1.generate_code()
    code = code + """
    
static long f2(const char *key, unsigned int cch)
"""
    code = code + self.f2.generate_code()
    code = code + """
    
static unsigned long hash(const char *key, unsigned int cch)
{
    return ((unsigned long)(G[ f1(key, cch) ]) + (unsigned long)(G[ f2(key, cch) ]) ) %% k_cHashElements;
}

const void *getValue(unsigned long iKey)
{
    return &%(dataArrayName)s[iKey];
}

/* Helper for adding objects to dictionaries. Check for errors with
   PyErr_Occurred() */
static 
void insobj(PyObject *dict,
     char *name,
     PyObject *v)
{
    PyDict_SetItemString(dict, name, v);
    Py_XDECREF(v);
}

static const %(structName)s hashAPI = 
{
    k_cKeys,
    k_cchMaxKey,
    &hash,
    &getValue,
};

static  
PyMethodDef Module_methods[] =
{   
    {NULL, NULL},
};

static char *Module_docstring = "%(moduleName)s hash function module";

/* Error reporting for module init functions */

#define Py_ReportModuleInitError(modname) {			\\
    PyObject *exc_type, *exc_value, *exc_tb;			\\
    PyObject *str_type, *str_value;				\\
								\\
    /* Fetch error objects and convert them to strings */	\\
    PyErr_Fetch(&exc_type, &exc_value, &exc_tb);		\\
    if (exc_type && exc_value) {				\\
	    str_type = PyObject_Str(exc_type);			\\
	    str_value = PyObject_Str(exc_value);			\\
    }								\\
    else {							\\
	   str_type = NULL;					\\
	   str_value = NULL;					\\
    }								\\
    /* Try to format a more informative error message using the	\\
       original error */					\\
    if (str_type && str_value &&				\\
	    PyString_Check(str_type) && PyString_Check(str_value))	\\
	    PyErr_Format(						\\
   		    PyExc_ImportError,				\\
		    "initialization of module "modname" failed "	\\
		    "(%%s:%%s)",					\\
		PyString_AS_STRING(str_type),			\\
		PyString_AS_STRING(str_value));			\\
    else							\\
	    PyErr_SetString(					\\
		    PyExc_ImportError,				\\
		    "initialization of module "modname" failed");	\\
    Py_XDECREF(str_type);					\\
    Py_XDECREF(str_value);					\\
    Py_XDECREF(exc_type);					\\
    Py_XDECREF(exc_value);					\\
    Py_XDECREF(exc_tb);						\\
}


/* Create PyMethodObjects and register them in the module\'s dict */
DL_EXPORT(void) 
init%(moduleName)s(void)
{
    PyObject *module, *moddict;
    /* Create module */
    module = Py_InitModule4("%(moduleName)s", /* Module name */
             Module_methods, /* Method list */
             Module_docstring, /* Module doc-string */
             (PyObject *)NULL, /* always pass this as *self */
             PYTHON_API_VERSION); /* API Version */
    if (module == NULL)
        goto onError;
    /* Add some constants to the module\'s dict */
    moddict = PyModule_GetDict(module);
    if (moddict == NULL)
        goto onError;

    /* Export C API */
    insobj(
        moddict,
        "%(moduleName)sAPI",
        PyCObject_FromVoidPtr((void *)&hashAPI, NULL));
    
onError:
    /* Check for errors and report them */
    if (PyErr_Occurred())
        Py_ReportModuleInitError("%(moduleName)s");
    return;
}
""" % { "moduleName" : moduleName,
        "dataArrayName" : dataArrayName,
        "structName" : structName, }
        
    return code    

  def generate_graph(self, out):
    out.write("""
static const unsigned short G[] = 
""")
    self.G.generate_code(out)

    
def generate_hash(keys, caseInsensitive=0,
                  minC=None, initC=None,
                  f1Seed=None, f2Seed=None,
                  cIncrement=None, cTries=None):
    """Print out code for a perfect minimal hash.  Input is a list of
    (key, desired hash value) tuples.  """
    
    # K is the number of keys.
    K = len(keys)
    
    # We will be generating graphs of size N, where N = c * K.
    # The larger C is, the fewer trial graphs will need to be made, but
    # the resulting table is also larger.  Increase this starting value
    # if you're impatient.  After 50 failures, c will be increased by 0.025.
    if initC is None:
      initC = 1.5
      
    c = initC
    if cIncrement is None:
      cIncrement = 0.0025

    if cTries is None:
      cTries = 50

    # Number of trial graphs so far
    num_graphs = 0           
    sys.stderr.write('Generating graphs... ')
    
    while 1:
        # N is the number of vertices in the graph G
        N = int(c*K)
        num_graphs = num_graphs + 1
        if (num_graphs % cTries) == 0:
            # Enough failures at this multiplier,
            # increase the multiplier and keep trying....
            c = c + cIncrement
            
            # Whats good with searching for a better
            # hash function if we exceed the size
            # of a function we've generated in the past.... 
            if minC is not None and \
               c > minC:
              c = initC
              sys.stderr.write(' -- c > minC, resetting c to %0.4f\n' % c)
            else:
              sys.stderr.write(' -- increasing c to %0.4f\n' % c)              
            sys.stderr.write('Generating graphs... ')

        # Output a progress message
        sys.stderr.write( str(num_graphs) + ' ')
        sys.stderr.flush()

        # Create graph w/ N vertices
        G = Graph(N)
        # Save the seeds used to generate
        # the following two hash functions.
        _seeds = whrandom._inst._seed
        
        # Create 2 random hash functions
        f1 = Hash(N, caseInsensitive)
        f2 = Hash(N, caseInsensitive)

        # Set the initial hash function seed values if passed in.
        # Doing this protects our hash functions from
        # changes to whrandom's behavior.
        if f1Seed is not None:
          f1.seed = f1Seed
          f1Seed = None
          fSpecifiedSeeds = 1
        if f2Seed is not None:
          f2.seed = f2Seed
          f2Seed = None
          fSpecifiedSeeds = 1

        # Connect vertices given by the values of the two hash functions
        # for each key.  Associate the desired hash value with each
        # edge.
        for k, v in keys:
            h1 = f1(k) ; h2 = f2(k)
            G.connect( h1, h2, v)

        # Check if the resulting graph is acyclic; if it is,
        # we're done with step 1.
        if G.is_acyclic():
          break
        elif fSpecifiedSeeds:
          sys.stderr.write('\nThe initial f1/f2 seeds you specified didn\'t generate a perfect hash function: \n')
          sys.stderr.write('f1 seed: %s\n' % f1.seed)
          sys.stderr.write('f2 seed: %s\n' % f2.seed)
          sys.stderr.write('multipler: %s\n' % c)
          sys.stderr.write('Your data has likely changed, or you forgot what your initial multiplier should be.\n')
          sys.stderr.write('continuing the search for a perfect hash function......\n')
          fSpecifiedSeeds = 0

    # Now we have an acyclic graph, so we assign values to each vertex
    # such that, for each edge, you can add the values for the two vertices
    # involved and get the desired value for that edge -- which is the
    # desired hash key.  This task is dead easy, because the graph is acyclic.
    sys.stderr.write('\nAcyclic graph found; computing vertex values...\n')
    G.assign_values()

    sys.stderr.write('Checking uniqueness of hash values...\n')
    
    # Sanity check the result by actually verifying that all the keys
    # hash to the right value.
    cchMaxKey = 0
    maxHashValue = 0
    
    for k, v in keys:
      hash1 = G.values[ f1(k) ]
      hash2 = G.values[ f2(k) ]
      if hash1 > maxHashValue:
        maxHashValue = hash1
      if hash2 > maxHashValue:
        maxHashValue = hash2
      perfecthash = (hash1 + hash2) % N
      assert perfecthash == v
      cch = len(k)
      if cch > cchMaxKey:
        cchMaxKey = cch

    sys.stderr.write('Found perfect hash function!\n')
    sys.stderr.write('\nIn order to regenerate this hash function, \n')
    sys.stderr.write('you need to pass these following values back in:\n')
    sys.stderr.write('f1 seed: %s\n' % hex(f1.seed))
    sys.stderr.write('f2 seed: %s\n' % hex(f2.seed))
    sys.stderr.write('initial multipler: %s\n' % c)

    return PerfectHash(cchMaxKey, f1, f2, G, N, len(keys), maxHashValue)

