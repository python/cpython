#ifndef _THREAD_H_included
#define _THREAD_H_included

#ifndef PROTO
#if defined(__STDC__) || defined(__cplusplus)
#define PROTO(args)	args
#else
#define PROTO(args)	()
#endif
#endif

typedef void *type_lock;
typedef void *type_sema;

#ifdef __cplusplus
extern "C" {
#endif

void init_thread PROTO((void));
int start_new_thread PROTO((void (*)(void *), void *));
void exit_thread PROTO((void));
void _exit_thread PROTO((void));

type_lock allocate_lock PROTO((void));
void free_lock PROTO((type_lock));
int acquire_lock PROTO((type_lock, int));
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0
void release_lock PROTO((type_lock));

type_sema allocate_sema PROTO((int));
void free_sema PROTO((type_sema));
void down_sema PROTO((type_sema));
void up_sema PROTO((type_sema));

void exit_prog PROTO((int));
void _exit_prog PROTO((int));

#ifdef __cplusplus
}
#endif

#endif
