#ifndef _THREAD_H_included
#define _THREAD_H_included

#ifdef __STDC__
#define _P(args)	args
#else
#define _P(args)	()
#endif

void init_thread _P((void));
int start_new_thread _P((void (*)(void *), void *));
void exit_thread _P((void));
void _exit_thread _P((void));

typedef void *type_lock;

type_lock allocate_lock _P((void));
void free_lock _P((type_lock));
int acquire_lock _P((type_lock, int));
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0
void release_lock _P((type_lock));

typedef void *type_sema;

type_sema allocate_sema _P((int));
void free_sema _P((type_sema));
void down_sema _P((type_sema));
void up_sema _P((type_sema));

void exit_prog _P((int));
void _exit_prog _P((int));

#undef _P

#endif
