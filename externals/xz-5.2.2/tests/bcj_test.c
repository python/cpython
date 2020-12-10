///////////////////////////////////////////////////////////////////////////////
//
/// \file       bcj_test.c
/// \brief      Source code of compress_prepared_bcj_*
///
/// This is a simple program that should make the compiler to generate
/// PC-relative branches, jumps, and calls. The compiled files can then
/// be used to test the branch conversion filters. Note that this program
/// itself does nothing useful.
///
/// Compiling: gcc -std=c99 -fPIC -c bcj_test.c
/// Don't optimize or strip.
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

extern int jump(int a, int b);


extern int
call(int a, int b)
{
	if (a < b)
		a = jump(a, b);

	return a;
}


extern int
jump(int a, int b)
{
	// The loop generates conditional jump backwards.
	while (1) {
		if (a < b) {
			a *= 2;
			a += 3 * b;
			break;
		} else {
			// Put enough code here to prevent JMP SHORT on x86.
			a += b;
			a /= 2;
			b += b % 5;
			a -= b / 3;
			b = 2 * b + a - 1;
			a *= b + a + 1;
			b += a - 1;
			a += b * 2 - a / 5;
		}
	}

	return a;
}


int
main(int argc, char **argv)
{
	int a = call(argc, argc + 1);
	return a == 0;
}
