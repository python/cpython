/*
** Glue code for MetroWerks CodeWarrior, which misses
** unix-like routines for file-access.
*/

#ifdef __MWERKS__
#include <Types.h>
#include <Files.h>
#include <Strings.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* #define DBGMALLOC /**/

#ifdef DBGMALLOC
#define NMALLOC 50000
static long m_index;
static long m_addrs[NMALLOC];
static long m_sizes[NMALLOC];
static long m_lastaddr;

#define SSLOP 2
#define SLOP (SSLOP+0)

void
m_abort() {
	printf("ABORT\n");
	exit(1);
}

void *
checkrv(ptr, size)
	void *ptr;
	int size;
{
	long b = (long)ptr, e = (long)ptr + size+SSLOP;
	int i;
	
	if ( m_index >= NMALLOC ) {
		printf("too many mallocs\n");
		m_abort();
	}
	m_lastaddr = (long)ptr;
	for(i=0; i<m_index; i++) {
		if ( m_addrs[i] > 0 && b < m_addrs[i]+m_sizes[i] && e > m_addrs[i] ) {
			printf("overlapping block with %d\n", i);
			m_abort();
		}
	}
	m_sizes[m_index] = size;
	m_addrs[m_index++] = (long)ptr;
	*(short *)ptr = m_index-1;
	return (void *)((char *)ptr+SSLOP);
	
}

void *
checkfree(ptr)
	void *ptr;
{
	int i;
	
	if ( ptr == 0 ) {
		printf("free null\n");
		m_abort();
	}
	m_lastaddr = (long)ptr;
	for(i=0; i<m_index; i++) {
		if ( m_addrs[i] == (long)ptr-SSLOP ) {
			m_addrs[i] = -m_addrs[i];
			ptr = (void *)((char *)ptr-SSLOP);
			if ( *(short *)ptr != i ) {
				int saved_i = i, saved_ptr = *(short *)ptr;
				
				printf("Wrong starter value\n");
				m_abort();
			}
			return ptr;
		}
	}
	printf("free unknown\n");
	m_abort();
	return 0;
}

void *
m_malloc(size)
{
	void *ptr;
	
	ptr = malloc(size+SLOP);
	if ( !ptr ) {
		printf("malloc failure\n");
		m_abort();
	}
	return checkrv(ptr, size);
}

void *
m_realloc(optr, size)
	void *optr;
{
	void *ptr;
	
	ptr = checkfree(ptr);
	ptr = realloc(ptr, size+SLOP);
	if ( !ptr ) {
		printf("realloc failure\n");
		m_abort();
	}
	return checkrv(ptr, size);
}

void *
m_calloc(size, nelem)
{
	void *ptr;
	
	ptr = calloc(1, nelem*size+SLOP);
	return checkrv(ptr, nelem*size);
}

void
m_free(ptr)
	void *ptr;
{
	
	ptr = checkfree(ptr);
	free(ptr);
}
#endif /* DBGMALLOC */

#ifdef CW4
int
fileno(fp)
	FILE *fp;
{
	if (fp==stdin) return 0;
	else if (fp==stdout) return 1;
	else if (fp==stderr) return 2;
	else return 3;
}

int
isatty(fd)
	int fd;
{
	return (fd >= 0 && fd <= 2);
}

int
unlink(old)
	char *old;
{
	OSErr err;
	
	if ((err=FSDelete((ConstStr255Param)Pstring(old), 0)) == noErr)
		return 0;
	errno= err;
	return -1;
}
#endif /* CW4 */

#endif /* __MWERKS__ */