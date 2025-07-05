** For MacOSX only **
---

This directory contains 2 programs :

* dump_shared_mem: view content of shared memory.
* reset_shared_mem: erase all stored datas of shared memory.

the `make_all.sh` batch builds these 2 programs.

# dump_shm.

`dump_shm` tries to connect to the shared memory only if its exists.
This program doesn't use synchronization primitive to read the shared memory.
To quit this program, press `Ctrl+C`.

```zsh
dump_shm -1 300
```
Executes this program forever, and check all 300 *us* if shared memory changes.

When there are changes in the shared memory (only about sempahore count), program prints the new content of shared memory as below:

```zsh
==========
Tue Feb 25 17:04:05 2025

dump_shm_semlock_counters
header:0x1022b4000 - counter array:0x1022b4010
n sems:2 - n sem_slots:87551, n procs:1, size_shm:2801664
p:0x1022b4010, n:/mp-fwl20ahw, v:6, r:0, t:Tue Feb 25 17:04:05 2025
p:0x1022b4030, n:/mp-z3635cdr, v:6, r:0, t:Tue Feb 25 17:04:04 2025

```

# reset_shm.

`reset_shm` tries to connect to the shared memory only if its exists.
This program uses synchronization primitive to read the shared memory.

When exits, this program calls `shm_unlink`.