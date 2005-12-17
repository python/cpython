#include "Python.h"
#include "asdl.h"

asdl_seq *
asdl_seq_new(int size, PyArena *arena)
{
	asdl_seq *seq = NULL;
	size_t n = sizeof(asdl_seq) +
			(size ? (sizeof(void *) * (size - 1)) : 0);

	seq = (asdl_seq *)malloc(n);
	if (!seq) {
		PyErr_NoMemory();
		return NULL;
	}
        PyArena_AddMallocPointer(arena, (void *)seq);
	memset(seq, 0, n);
	seq->size = size;
	return seq;
}

void
asdl_seq_free(asdl_seq *seq)
{
}
