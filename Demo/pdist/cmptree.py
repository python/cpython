"""Compare local and remote dictionaries and transfer differing files -- like rdist."""

import sys
from repr import repr
import FSProxy
import time
import os

def raw_input(prompt):
    sys.stdout.write(prompt)
    sys.stdout.flush()
    return sys.stdin.readline()

def main():
    pwd = os.getcwd()
    s = input("chdir [%s] " % pwd)
    if s:
        os.chdir(s)
        pwd = os.getcwd()
    host = ask("host", 'voorn.cwi.nl')
    port = 4127
    verbose = 1
    mode = ''
    print("""\
Mode should be a string of characters, indicating what to do with differences.
r - read different files to local file system
w - write different files to remote file system
c - create new files, either remote or local
d - delete disappearing files, either remote or local
""")
    s = input("mode [%s] " % mode)
    if s: mode = s
    address = (host, port)
    t1 = time.time()
    local = FSProxy.FSProxyLocal()
    remote = FSProxy.FSProxyClient(address, verbose)
    compare(local, remote, mode)
    remote._close()
    local._close()
    t2 = time.time()
    dt = t2-t1
    mins, secs = divmod(dt, 60)
    print(mins, "minutes and", round(secs), "seconds")
    input("[Return to exit] ")

def ask(prompt, default):
    s = input("%s [%s] " % (prompt, default))
    return s or default

def askint(prompt, default):
    s = input("%s [%s] " % (prompt, str(default)))
    if s: return string.atoi(s)
    return default

def compare(local, remote, mode):
    print()
    print("PWD =", repr(os.getcwd()))
    sums_id = remote._send('sumlist')
    subdirs_id = remote._send('listsubdirs')
    remote._flush()
    print("calculating local sums ...")
    lsumdict = {}
    for name, info in local.sumlist():
        lsumdict[name] = info
    print("getting remote sums ...")
    sums = remote._recv(sums_id)
    print("got", len(sums))
    rsumdict = {}
    for name, rsum in sums:
        rsumdict[name] = rsum
        if name not in lsumdict:
            print(repr(name), "only remote")
            if 'r' in mode and 'c' in mode:
                recvfile(local, remote, name)
        else:
            lsum = lsumdict[name]
            if lsum != rsum:
                print(repr(name), end=' ')
                rmtime = remote.mtime(name)
                lmtime = local.mtime(name)
                if rmtime > lmtime:
                    print("remote newer", end=' ')
                    if 'r' in mode:
                        recvfile(local, remote, name)
                elif lmtime > rmtime:
                    print("local newer", end=' ')
                    if 'w' in mode:
                        sendfile(local, remote, name)
                else:
                    print("same mtime but different sum?!?!", end=' ')
                print()
    for name in lsumdict.keys():
        if not list(rsumdict.keys()):
            print(repr(name), "only locally", end=' ')
            fl()
            if 'w' in mode and 'c' in mode:
                sendfile(local, remote, name)
            elif 'r' in mode and 'd' in mode:
                os.unlink(name)
                print("removed.")
            print()
    print("gettin subdirs ...")
    subdirs = remote._recv(subdirs_id)
    common = []
    for name in subdirs:
        if local.isdir(name):
            print("Common subdirectory", repr(name))
            common.append(name)
        else:
            print("Remote subdirectory", repr(name), "not found locally")
            if 'r' in mode and 'c' in mode:
                pr = "Create local subdirectory %s? [y] " % \
                     repr(name)
                if 'y' in mode:
                    ok = 'y'
                else:
                    ok = ask(pr, "y")
                if ok[:1] in ('y', 'Y'):
                    local.mkdir(name)
                    print("Subdirectory %s made" % \
                            repr(name))
                    common.append(name)
    lsubdirs = local.listsubdirs()
    for name in lsubdirs:
        if name not in subdirs:
            print("Local subdirectory", repr(name), "not found remotely")
    for name in common:
        print("Entering subdirectory", repr(name))
        local.cd(name)
        remote.cd(name)
        compare(local, remote, mode)
        remote.back()
        local.back()

def sendfile(local, remote, name):
    try:
        remote.create(name)
    except (IOError, os.error) as msg:
        print("cannot create:", msg)
        return

    print("sending ...", end=' ')
    fl()

    data = open(name).read()

    t1 = time.time()

    remote._send_noreply('write', name, data)
    remote._flush()

    t2 = time.time()

    dt = t2-t1
    print(len(data), "bytes in", round(dt), "seconds", end=' ')
    if dt:
        print("i.e.", round(len(data)/dt), "bytes/sec", end=' ')
    print()

def recvfile(local, remote, name):
    ok = 0
    try:
        rv = recvfile_real(local, remote, name)
        ok = 1
        return rv
    finally:
        if not ok:
            print("*** recvfile of %r failed, deleting" % (name,))
            local.delete(name)

def recvfile_real(local, remote, name):
    try:
        local.create(name)
    except (IOError, os.error) as msg:
        print("cannot create:", msg)
        return

    print("receiving ...", end=' ')
    fl()

    f = open(name, 'w')
    t1 = time.time()

    length = 4*1024
    offset = 0
    id = remote._send('read', name, offset, length)
    remote._flush()
    while 1:
        newoffset = offset + length
        newid = remote._send('read', name, newoffset, length)
        data = remote._recv(id)
        id = newid
        if not data: break
        f.seek(offset)
        f.write(data)
        offset = newoffset
    size = f.tell()

    t2 = time.time()
    f.close()

    dt = t2-t1
    print(size, "bytes in", round(dt), "seconds", end=' ')
    if dt:
        print("i.e.", int(size/dt), "bytes/sec", end=' ')
    print()
    remote._recv(id) # ignored

def fl():
    sys.stdout.flush()

if __name__ == '__main__':
    main()
