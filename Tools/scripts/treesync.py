#! /usr/bin/env python3

"""Script to synchronize two source trees.

Invoke with two arguments:

python treesync.py slave master

The assumption is that "master" contains CVS administration while
slave doesn't.  All files in the slave tree that have a CVS/Entries
entry in the master tree are synchronized.  This means:

    If the files differ:
        if the slave file is newer:
            normalize the slave file
            if the files still differ:
                copy the slave to the master
        else (the master is newer):
            copy the master to the slave

    normalizing the slave means replacing CRLF with LF when the master
    doesn't use CRLF

"""

import os, sys, stat, getopt

# Interactivity options
default_answer = "ask"
create_files = "yes"
create_directories = "no"
write_slave = "ask"
write_master = "ask"

def main():
    global always_no, always_yes
    global create_directories, write_master, write_slave
    opts, args = getopt.getopt(sys.argv[1:], "nym:s:d:f:a:")
    for o, a in opts:
        if o == '-y':
            default_answer = "yes"
        if o == '-n':
            default_answer = "no"
        if o == '-s':
            write_slave = a
        if o == '-m':
            write_master = a
        if o == '-d':
            create_directories = a
        if o == '-f':
            create_files = a
        if o == '-a':
            create_files = create_directories = write_slave = write_master = a
    try:
        [slave, master] = args
    except ValueError:
        print("usage: python", sys.argv[0] or "treesync.py", end=' ')
        print("[-n] [-y] [-m y|n|a] [-s y|n|a] [-d y|n|a] [-f n|y|a]", end=' ')
        print("slavedir masterdir")
        return
    process(slave, master)

def process(slave, master):
    cvsdir = os.path.join(master, "CVS")
    if not os.path.isdir(cvsdir):
        print("skipping master subdirectory", master)
        print("-- not under CVS")
        return
    print("-"*40)
    print("slave ", slave)
    print("master", master)
    if not os.path.isdir(slave):
        if not okay("create slave directory %s?" % slave,
                    answer=create_directories):
            print("skipping master subdirectory", master)
            print("-- no corresponding slave", slave)
            return
        print("creating slave directory", slave)
        try:
            os.mkdir(slave)
        except OSError as msg:
            print("can't make slave directory", slave, ":", msg)
            return
        else:
            print("made slave directory", slave)
    cvsdir = None
    subdirs = []
    names = os.listdir(master)
    for name in names:
        mastername = os.path.join(master, name)
        slavename = os.path.join(slave, name)
        if name == "CVS":
            cvsdir = mastername
        else:
            if os.path.isdir(mastername) and not os.path.islink(mastername):
                subdirs.append((slavename, mastername))
    if cvsdir:
        entries = os.path.join(cvsdir, "Entries")
        for e in open(entries).readlines():
            words = e.split('/')
            if words[0] == '' and words[1:]:
                name = words[1]
                s = os.path.join(slave, name)
                m = os.path.join(master, name)
                compare(s, m)
    for (s, m) in subdirs:
        process(s, m)

def compare(slave, master):
    try:
        sf = open(slave, 'r')
    except IOError:
        sf = None
    try:
        mf = open(master, 'rb')
    except IOError:
        mf = None
    if not sf:
        if not mf:
            print("Neither master nor slave exists", master)
            return
        print("Creating missing slave", slave)
        copy(master, slave, answer=create_files)
        return
    if not mf:
        print("Not updating missing master", master)
        return
    if sf and mf:
        if identical(sf, mf):
            return
    sft = mtime(sf)
    mft = mtime(mf)
    if mft > sft:
        # Master is newer -- copy master to slave
        sf.close()
        mf.close()
        print("Master             ", master)
        print("is newer than slave", slave)
        copy(master, slave, answer=write_slave)
        return
    # Slave is newer -- copy slave to master
    print("Slave is", sft-mft, "seconds newer than master")
    # But first check what to do about CRLF
    mf.seek(0)
    fun = funnychars(mf)
    mf.close()
    sf.close()
    if fun:
        print("***UPDATING MASTER (BINARY COPY)***")
        copy(slave, master, "rb", answer=write_master)
    else:
        print("***UPDATING MASTER***")
        copy(slave, master, "r", answer=write_master)

BUFSIZE = 16*1024

def identical(sf, mf):
    while 1:
        sd = sf.read(BUFSIZE)
        md = mf.read(BUFSIZE)
        if sd != md: return 0
        if not sd: break
    return 1

def mtime(f):
    st = os.fstat(f.fileno())
    return st[stat.ST_MTIME]

def funnychars(f):
    while 1:
        buf = f.read(BUFSIZE)
        if not buf: break
        if '\r' in buf or '\0' in buf: return 1
    return 0

def copy(src, dst, rmode="rb", wmode="wb", answer='ask'):
    print("copying", src)
    print("     to", dst)
    if not okay("okay to copy? ", answer):
        return
    f = open(src, rmode)
    g = open(dst, wmode)
    while 1:
        buf = f.read(BUFSIZE)
        if not buf: break
        g.write(buf)
    f.close()
    g.close()

def raw_input(prompt):
    sys.stdout.write(prompt)
    sys.stdout.flush()
    return sys.stdin.readline()

def okay(prompt, answer='ask'):
    answer = answer.strip().lower()
    if not answer or answer[0] not in 'ny':
        answer = input(prompt)
        answer = answer.strip().lower()
        if not answer:
            answer = default_answer
    if answer[:1] == 'y':
        return 1
    if answer[:1] == 'n':
        return 0
    print("Yes or No please -- try again:")
    return okay(prompt)

if __name__ == '__main__':
    main()
