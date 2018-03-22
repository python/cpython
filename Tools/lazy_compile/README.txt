Lazy Execution of Top-level Module Bytecode
===========================================

Background
----------

This is an idea generated while trying to reduce the interpreter
startup time, for Python 3.7.  At the core sprint in the fall of 2017,
Larry Hastings mentioned that PHP did a similar thing and saw a good
performance improvement.

Generally, when a Python program starts up, much of the bytecode
executed during module imports is not actually needed.  E.g. a module
that creates many functions but only a few of them are used.  Or, a
module imports sub-modules but only uses some fraction of what they
provide.

This overhead of unused bytecode can be reduced by splitting modules
into multiple modules and through other techniques.  However, often
those approaches are considered bad style as they make the code harder
to maintain.  It would be better if modules can be written in a straight
forward manner and the Python interpreter could provide "pay only for
what you use" runtime behavior.


Lazy execution
--------------

The bytecode compiler provided with the patch allows module bytecode to
be partially executed.  I.e. each top-level definition (functions,
classes, imports, global constants) can potentially have its own
bytecode string.  This separate bytecode is not executed when the module
is loaded.  Instead, accessing the corresponding module attribute would
cause the bytecode to be executed.

To trap the attribute access, we need a mechanism similar to PEP 262
(module __getattr__).  However, it must also work for global variable
access from within the module.  That requires modification of the
LOAD_NAME and LOAD_GLOBAL opcodes.  If attribute lookup fails, the
globals dict is checked for a __lazy_code__ dict.  If the attribute name
exists in the dict, the value should be a marshaled code object and it
will be executed in order to create the attribute.  This extra check is
done in the module getattr method, in LOAD_NAME and LOAD_GLOBAL.

A demo of this behavior is provided in the
Tools/lazy_compile/lazy_demo.py script.  Rather than using a special
compiler, the demo script creates a module with the __lazy_code__
global.  Running the script shows how the code is executed on attribute
access.


Lazy compiler
-------------

Modules that wish to be compiled with lazy execution behavior need to
explicitly marked by defining a __lazy_module__ global variable.  The
major behavioral difference is that import statements will not execute
imported modules immediately.

Modules marked with __lazy_module__ should probably also provide
__all__.  Otherwise, dir() on the module will not show the lazy
attributes unless they have been already loaded.  Also, 'from <mod>
import *' will not import the attributes.

The compiler uses the lazy_analyze module in order to determine which
top-level module definitions are safe to be converted into lazily
executed attributes.  Some more thought should be given to what side
effects are safe given that a module is marked with __lazy_module__.


Performance impact
------------------

As a quick and dirty test of the performance impact of this patch, I ran
did the following test.  First, compile the standard library using the
modified compiler:

    $ ./python Tools/lazy_compile/lazy_compile.py -f Lib

To get an idea of the process memory use, I added this patch to
regrtest:

    diff --git a/Lib/test/regrtest.py b/Lib/test/regrtest.py
    index 21b0edfd07..8c24245314 100755
    --- a/Lib/test/regrtest.py
    +++ b/Lib/test/regrtest.py
    @@ -47,4 +47,8 @@ def _main():


    if __name__ == '__main__':
    -    _main()
    +    try:
    +        _main()
    +    finally:
    +        import resource
    +        print('RSS', resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1000)


The test case was:

    $ time ./python Lib/test/regrtest.py test_bisect

I choose test_bisect because it executes relatively quickly but also
imports a number of sub-modules.  Compared with the "master" branch of
Python, the lazy module version uses 67% of the run time and 70% of the
max resident set size (peak memory use).


Futher ideas
------------

Inspired by a suggestion by Carl S. Shapiro, we could sometimes do away
with one layer of bytecode execution.  E.g. if the top-level definition
is:

    a = 1

rather than executing the bytecode

  1           0 LOAD_CONST               0 (1)
              2 STORE_NAME               0 (a)

we could instead store the marshalled (or some other serialized form) of
the integer 1.  I.e. the value in __lazy_code__ for 'a' would be a
serialized version of the integer 1.  Then, when 'a' is accessed, we
unserialize the value and assign it to the global name.  Carl's specific
suggestion was to store the value in the initialized data segment of the
interpreter.  That would eliminate the unserialize step and the data
could be paged into RAM by the OS when the object is first accessed.
Due to the representation of Python objects (ref counts, heavy use of
pointers), that idea seems quite challenging to implement.  Python
objects in general are not designed to be statically allocated on the
heap.

Rather than loading the __lazy_code__ dict when the module is first
loaded, it would be better if we could leave that code on disk until it
is needed.  Doing so would require some engineering, in order to have
good performance.  A naive approach of opening the .pyc file and reading
each __lazy_code__ attribute as it is needed would likely perform
poorly (too many system calls).  The dream would be to have a single
file that contains all the bytecode for all the modules.  It would be
organized as an associative array (b-tree, etc) and mmapped into the
Python process space.  Then, to load bytecode for a module or a module
attribute, we find the offset in the file for the bytecode and the OS
will page it in as needed.  No system calls required once the mmap is
setup.  As an additional refinement, the order of modules in the file
could be such that OS/disk read-ahead matches the order in which modules
are loaded.

Having a single file for bytecode would perform best but for usability,
it would be nice to allow multiple files.  E.g. have one file for the
standard library and allow a second file for the application.  Or,
provide a tool that builds a single bytecode file based on a stdlib
bytecode file and a set of application Python modules/packages.  Given
the low cost of disk space, I expect many applications would like this
approach.  If the file can have a magic number detected by the Linux
kernel, it would even be possible to make those files directly
executable.

-- Neil Schemenauer <neil@python.ca>, 2018-03-22
