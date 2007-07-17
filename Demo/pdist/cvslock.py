"""CVS locking algorithm.

CVS locking strategy
====================

As reverse engineered from the CVS 1.3 sources (file lock.c):

- Locking is done on a per repository basis (but a process can hold
write locks for multiple directories); all lock files are placed in
the repository and have names beginning with "#cvs.".

- Before even attempting to lock, a file "#cvs.tfl.<pid>" is created
(and removed again), to test that we can write the repository.  [The
algorithm can still be fooled (1) if the repository's mode is changed
while attempting to lock; (2) if this file exists and is writable but
the directory is not.]

- While creating the actual read/write lock files (which may exist for
a long time), a "meta-lock" is held.  The meta-lock is a directory
named "#cvs.lock" in the repository.  The meta-lock is also held while
a write lock is held.

- To set a read lock:

        - acquire the meta-lock
        - create the file "#cvs.rfl.<pid>"
        - release the meta-lock

- To set a write lock:

        - acquire the meta-lock
        - check that there are no files called "#cvs.rfl.*"
                - if there are, release the meta-lock, sleep, try again
        - create the file "#cvs.wfl.<pid>"

- To release a write lock:

        - remove the file "#cvs.wfl.<pid>"
        - rmdir the meta-lock

- To release a read lock:

        - remove the file "#cvs.rfl.<pid>"


Additional notes
----------------

- A process should read-lock at most one repository at a time.

- A process may write-lock as many repositories as it wishes (to avoid
deadlocks, I presume it should always lock them top-down in the
directory hierarchy).

- A process should make sure it removes all its lock files and
directories when it crashes.

- Limitation: one user id should not be committing files into the same
repository at the same time.


Turn this into Python code
--------------------------

rl = ReadLock(repository, waittime)

wl = WriteLock(repository, waittime)

list = MultipleWriteLock([repository1, repository2, ...], waittime)

"""


import os
import time
import stat
import pwd


# Default wait time
DELAY = 10


# XXX This should be the same on all Unix versions
EEXIST = 17


# Files used for locking (must match cvs.h in the CVS sources)
CVSLCK = "#cvs.lck"
CVSRFL = "#cvs.rfl."
CVSWFL = "#cvs.wfl."


class Error:

    def __init__(self, msg):
        self.msg = msg

    def __repr__(self):
        return repr(self.msg)

    def __str__(self):
        return str(self.msg)


class Locked(Error):
    pass


class Lock:

    def __init__(self, repository = ".", delay = DELAY):
        self.repository = repository
        self.delay = delay
        self.lockdir = None
        self.lockfile = None
        pid = repr(os.getpid())
        self.cvslck = self.join(CVSLCK)
        self.cvsrfl = self.join(CVSRFL + pid)
        self.cvswfl = self.join(CVSWFL + pid)

    def __del__(self):
        print("__del__")
        self.unlock()

    def setlockdir(self):
        while 1:
            try:
                self.lockdir = self.cvslck
                os.mkdir(self.cvslck, 0o777)
                return
            except os.error as msg:
                self.lockdir = None
                if msg[0] == EEXIST:
                    try:
                        st = os.stat(self.cvslck)
                    except os.error:
                        continue
                    self.sleep(st)
                    continue
                raise Error("failed to lock %s: %s" % (
                        self.repository, msg))

    def unlock(self):
        self.unlockfile()
        self.unlockdir()

    def unlockfile(self):
        if self.lockfile:
            print("unlink", self.lockfile)
            try:
                os.unlink(self.lockfile)
            except os.error:
                pass
            self.lockfile = None

    def unlockdir(self):
        if self.lockdir:
            print("rmdir", self.lockdir)
            try:
                os.rmdir(self.lockdir)
            except os.error:
                pass
            self.lockdir = None

    def sleep(self, st):
        sleep(st, self.repository, self.delay)

    def join(self, name):
        return os.path.join(self.repository, name)


def sleep(st, repository, delay):
    if delay <= 0:
        raise Locked(st)
    uid = st[stat.ST_UID]
    try:
        pwent = pwd.getpwuid(uid)
        user = pwent[0]
    except KeyError:
        user = "uid %d" % uid
    print("[%s]" % time.ctime(time.time())[11:19], end=' ')
    print("Waiting for %s's lock in" % user, repository)
    time.sleep(delay)


class ReadLock(Lock):

    def __init__(self, repository, delay = DELAY):
        Lock.__init__(self, repository, delay)
        ok = 0
        try:
            self.setlockdir()
            self.lockfile = self.cvsrfl
            fp = open(self.lockfile, 'w')
            fp.close()
            ok = 1
        finally:
            if not ok:
                self.unlockfile()
            self.unlockdir()


class WriteLock(Lock):

    def __init__(self, repository, delay = DELAY):
        Lock.__init__(self, repository, delay)
        self.setlockdir()
        while 1:
            uid = self.readers_exist()
            if not uid:
                break
            self.unlockdir()
            self.sleep(uid)
        self.lockfile = self.cvswfl
        fp = open(self.lockfile, 'w')
        fp.close()

    def readers_exist(self):
        n = len(CVSRFL)
        for name in os.listdir(self.repository):
            if name[:n] == CVSRFL:
                try:
                    st = os.stat(self.join(name))
                except os.error:
                    continue
                return st
        return None


def MultipleWriteLock(repositories, delay = DELAY):
    while 1:
        locks = []
        for r in repositories:
            try:
                locks.append(WriteLock(r, 0))
            except Locked as instance:
                del locks
                break
        else:
            break
        sleep(instance.msg, r, delay)
    return list


def test():
    import sys
    if sys.argv[1:]:
        repository = sys.argv[1]
    else:
        repository = "."
    rl = None
    wl = None
    try:
        print("attempting write lock ...")
        wl = WriteLock(repository)
        print("got it.")
        wl.unlock()
        print("attempting read lock ...")
        rl = ReadLock(repository)
        print("got it.")
        rl.unlock()
    finally:
        print([1])
        print([2])
        if rl:
            rl.unlock()
        print([3])
        if wl:
            wl.unlock()
        print([4])
        rl = None
        print([5])
        wl = None
        print([6])


if __name__ == '__main__':
    test()
