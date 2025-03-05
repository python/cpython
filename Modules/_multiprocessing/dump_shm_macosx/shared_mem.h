#ifndef SHARED_MEM_H
#define SHARED_MEM_H

extern CountersWorkaround shm_semlock_counters;
extern HeaderObject *header;
extern CounterObject *counter;

int acquire_lock(SEM_HANDLE sem);
int release_lock(SEM_HANDLE sem);

void connect_shm_semlock_counters(int unlink,  int force_connect, int release_lock);
void delete_shm_semlock_counters_without_unlink(void);
void delete_shm_semlock_counters(void);

void dump_shm_semlock_header(void);
void dump_shm_semlock_header_counters(void);

#endif /* SHARED_MEM_H */
