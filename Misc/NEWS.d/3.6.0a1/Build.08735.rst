Issue #24421: Compile Modules/_math.c once, before building extensions.
Previously it could fail to compile properly if the math and cmath builds
were concurrent.