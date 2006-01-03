
Summary
-------
This directory contains ASM implementations of the functions
longest_match() and inflate_fast().


Use instructions
----------------
Copy these files into the zlib source directory, then run the
appropriate makefile, as suggested below.


Build instructions
------------------
* With Microsoft C and MASM:
nmake -f win32/Makefile.msc LOC="-DASMV -DASMINF" OBJA="gvmat32c.obj gvmat32.obj inffas32.obj"

* With Borland C and TASM:
make -f win32/Makefile.bor LOCAL_ZLIB="-DASMV -DASMINF" OBJA="gvmat32c.obj gvmat32.obj inffas32.obj" OBJPA="+gvmat32c.obj+gvmat32.obj+inffas32.obj"

