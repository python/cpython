#include "Python.h"
#include "asdl.h"

asdl_seq *
asdl_seq_new(int size, PyArena *arena)
{
	asdl_seq *seq = NULL;
	size_t n = sizeof(asdl_seq) +
			(size ? (sizeof(void *) * (size - 1)) : 0);

    seq = (asdl_seq *)PyArena_Malloc(arena, n);
	if (!seq) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(seq, 0, n);
	seq->size = size;
	return seq;
}
