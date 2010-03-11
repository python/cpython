#! /usr/bin/env python3

"""Mirror a remote ftp subtree into a local directory tree.

usage: ftpmirror [-v] [-q] [-i] [-m] [-n] [-r] [-s pat]
                 [-l username [-p passwd [-a account]]]
                 hostname[:port] [remotedir [localdir]]
-v: verbose
-q: quiet
-i: interactive mode
-m: macintosh server (NCSA telnet 2.4) (implies -n -s '*.o')
-n: don't log in
-r: remove local files/directories no longer pertinent
-l username [-p passwd [-a account]]: login info (default .netrc or anonymous)
-s pat: skip files matching pattern
hostname: remote host w/ optional port separated by ':'
remotedir: remote directory (default initial)
localdir: local directory (default current)
"""

import os
import sys
import time
import getopt
import ftplib
import netrc
from fnmatch import fnmatch

# Print usage message and exit
def usage(*args):
    sys.stdout = sys.stderr
    for msg in args: print(msg)
    print(__doc__)
    sys.exit(2)

verbose = 1 # 0 for -q, 2 for -v
interactive = 0
mac = 0
rmok = 0
nologin = 0
skippats = ['.', '..', '.mirrorinfo']

# Main program: parse command line and start processing
def main():
    global verbose, interactive, mac, rmok, nologin
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'a:bil:mnp:qrs:v')
    except getopt.error as msg:
        usage(msg)
    login = ''
    passwd = ''
    account = ''
    if not args: usage('hostname missing')
    host = args[0]
    port = 0
    if ':' in host:
        host, port = host.split(':', 1)
        port = int(port)
    try:
        auth = netrc.netrc().authenticators(host)
        if auth is not None:
            login, account, passwd = auth
    except (netrc.NetrcParseError, IOError):
        pass
    for o, a in opts:
        if o == '-l': login = a
        if o == '-p': passwd = a
        if o == '-a': account = a
        if o == '-v': verbose = verbose + 1
        if o == '-q': verbose = 0
        if o == '-i': interactive = 1
        if o == '-m': mac = 1; nologin = 1; skippats.append('*.o')
        if o == '-n': nologin = 1
        if o == '-r': rmok = 1
        if o == '-s': skippats.append(a)
    remotedir = ''
    localdir = ''
    if args[1:]:
        remotedir = args[1]
        if args[2:]:
            localdir = args[2]
            if args[3:]: usage('too many arguments')
    #
    f = ftplib.FTP()
    if verbose: print("Connecting to '%s%s'..." % (host,
                                                   (port and ":%d"%port or "")))
    f.connect(host,port)
    if not nologin:
        if verbose:
            print('Logging in as %r...' % (login or 'anonymous'))
        f.login(login, passwd, account)
    if verbose: print('OK.')
    pwd = f.pwd()
    if verbose > 1: print('PWD =', repr(pwd))
    if remotedir:
        if verbose > 1: print('cwd(%s)' % repr(remotedir))
        f.cwd(remotedir)
        if verbose > 1: print('OK.')
        pwd = f.pwd()
        if verbose > 1: print('PWD =', repr(pwd))
    #
    mirrorsubdir(f, localdir)

# Core logic: mirror one subdirectory (recursively)
def mirrorsubdir(f, localdir):
    pwd = f.pwd()
    if localdir and not os.path.isdir(localdir):
        if verbose: print('Creating local directory', repr(localdir))
        try:
            makedir(localdir)
        except os.error as msg:
            print("Failed to establish local directory", repr(localdir))
            return
    infofilename = os.path.join(localdir, '.mirrorinfo')
    try:
        text = open(infofilename, 'r').read()
    except IOError as msg:
        text = '{}'
    try:
        info = eval(text)
    except (SyntaxError, NameError):
        print('Bad mirror info in', repr(infofilename))
        info = {}
    subdirs = []
    listing = []
    if verbose: print('Listing remote directory %r...' % (pwd,))
    f.retrlines('LIST', listing.append)
    filesfound = []
    for line in listing:
        if verbose > 1: print('-->', repr(line))
        if mac:
            # Mac listing has just filenames;
            # trailing / means subdirectory
            filename = line.strip()
            mode = '-'
            if filename[-1:] == '/':
                filename = filename[:-1]
                mode = 'd'
            infostuff = ''
        else:
            # Parse, assuming a UNIX listing
            words = line.split(None, 8)
            if len(words) < 6:
                if verbose > 1: print('Skipping short line')
                continue
            filename = words[-1].lstrip()
            i = filename.find(" -> ")
            if i >= 0:
                # words[0] had better start with 'l'...
                if verbose > 1:
                    print('Found symbolic link %r' % (filename,))
                linkto = filename[i+4:]
                filename = filename[:i]
            infostuff = words[-5:-1]
            mode = words[0]
        skip = 0
        for pat in skippats:
            if fnmatch(filename, pat):
                if verbose > 1:
                    print('Skip pattern', repr(pat), end=' ')
                    print('matches', repr(filename))
                skip = 1
                break
        if skip:
            continue
        if mode[0] == 'd':
            if verbose > 1:
                print('Remembering subdirectory', repr(filename))
            subdirs.append(filename)
            continue
        filesfound.append(filename)
        if filename in info and info[filename] == infostuff:
            if verbose > 1:
                print('Already have this version of',repr(filename))
            continue
        fullname = os.path.join(localdir, filename)
        tempname = os.path.join(localdir, '@'+filename)
        if interactive:
            doit = askabout('file', filename, pwd)
            if not doit:
                if filename not in info:
                    info[filename] = 'Not retrieved'
                continue
        try:
            os.unlink(tempname)
        except os.error:
            pass
        if mode[0] == 'l':
            if verbose:
                print("Creating symlink %r -> %r" % (filename, linkto))
            try:
                os.symlink(linkto, tempname)
            except IOError as msg:
                print("Can't create %r: %s" % (tempname, msg))
                continue
        else:
            try:
                fp = open(tempname, 'wb')
            except IOError as msg:
                print("Can't create %r: %s" % (tempname, msg))
                continue
            if verbose:
                print('Retrieving %r from %r as %r...' % (filename, pwd, fullname))
            if verbose:
                fp1 = LoggingFile(fp, 1024, sys.stdout)
            else:
                fp1 = fp
            t0 = time.time()
            try:
                f.retrbinary('RETR ' + filename,
                             fp1.write, 8*1024)
            except ftplib.error_perm as msg:
                print(msg)
            t1 = time.time()
            bytes = fp.tell()
            fp.close()
            if fp1 != fp:
                fp1.close()
        try:
            os.unlink(fullname)
        except os.error:
            pass            # Ignore the error
        try:
            os.rename(tempname, fullname)
        except os.error as msg:
            print("Can't rename %r to %r: %s" % (tempname, fullname, msg))
            continue
        info[filename] = infostuff
        writedict(info, infofilename)
        if verbose and mode[0] != 'l':
            dt = t1 - t0
            kbytes = bytes / 1024.0
            print(int(round(kbytes)), end=' ')
            print('Kbytes in', end=' ')
            print(int(round(dt)), end=' ')
            print('seconds', end=' ')
            if t1 > t0:
                print('(~%d Kbytes/sec)' % \
                          int(round(kbytes/dt),))
            print()
    #
    # Remove files from info that are no longer remote
    deletions = 0
    for filename in list(info.keys()):
        if filename not in filesfound:
            if verbose:
                print("Removing obsolete info entry for", end=' ')
                print(repr(filename), "in", repr(localdir or "."))
            del info[filename]
            deletions = deletions + 1
    if deletions:
        writedict(info, infofilename)
    #
    # Remove local files that are no longer in the remote directory
    try:
        if not localdir: names = os.listdir(os.curdir)
        else: names = os.listdir(localdir)
    except os.error:
        names = []
    for name in names:
        if name[0] == '.' or name in info or name in subdirs:
            continue
        skip = 0
        for pat in skippats:
            if fnmatch(name, pat):
                if verbose > 1:
                    print('Skip pattern', repr(pat), end=' ')
                    print('matches', repr(name))
                skip = 1
                break
        if skip:
            continue
        fullname = os.path.join(localdir, name)
        if not rmok:
            if verbose:
                print('Local file', repr(fullname), end=' ')
                print('is no longer pertinent')
            continue
        if verbose: print('Removing local file/dir', repr(fullname))
        remove(fullname)
    #
    # Recursively mirror subdirectories
    for subdir in subdirs:
        if interactive:
            doit = askabout('subdirectory', subdir, pwd)
            if not doit: continue
        if verbose: print('Processing subdirectory', repr(subdir))
        localsubdir = os.path.join(localdir, subdir)
        pwd = f.pwd()
        if verbose > 1:
            print('Remote directory now:', repr(pwd))
            print('Remote cwd', repr(subdir))
        try:
            f.cwd(subdir)
        except ftplib.error_perm as msg:
            print("Can't chdir to", repr(subdir), ":", repr(msg))
        else:
            if verbose: print('Mirroring as', repr(localsubdir))
            mirrorsubdir(f, localsubdir)
            if verbose > 1: print('Remote cwd ..')
            f.cwd('..')
        newpwd = f.pwd()
        if newpwd != pwd:
            print('Ended up in wrong directory after cd + cd ..')
            print('Giving up now.')
            break
        else:
            if verbose > 1: print('OK.')

# Helper to remove a file or directory tree
def remove(fullname):
    if os.path.isdir(fullname) and not os.path.islink(fullname):
        try:
            names = os.listdir(fullname)
        except os.error:
            names = []
        ok = 1
        for name in names:
            if not remove(os.path.join(fullname, name)):
                ok = 0
        if not ok:
            return 0
        try:
            os.rmdir(fullname)
        except os.error as msg:
            print("Can't remove local directory %r: %s" % (fullname, msg))
            return 0
    else:
        try:
            os.unlink(fullname)
        except os.error as msg:
            print("Can't remove local file %r: %s" % (fullname, msg))
            return 0
    return 1

# Wrapper around a file for writing to write a hash sign every block.
class LoggingFile:
    def __init__(self, fp, blocksize, outfp):
        self.fp = fp
        self.bytes = 0
        self.hashes = 0
        self.blocksize = blocksize
        self.outfp = outfp
    def write(self, data):
        self.bytes = self.bytes + len(data)
        hashes = int(self.bytes) / self.blocksize
        while hashes > self.hashes:
            self.outfp.write('#')
            self.outfp.flush()
            self.hashes = self.hashes + 1
        self.fp.write(data)
    def close(self):
        self.outfp.write('\n')

def raw_input(prompt):
    sys.stdout.write(prompt)
    sys.stdout.flush()
    return sys.stdin.readline()

# Ask permission to download a file.
def askabout(filetype, filename, pwd):
    prompt = 'Retrieve %s %s from %s ? [ny] ' % (filetype, filename, pwd)
    while 1:
        reply = raw_input(prompt).strip().lower()
        if reply in ['y', 'ye', 'yes']:
            return 1
        if reply in ['', 'n', 'no', 'nop', 'nope']:
            return 0
        print('Please answer yes or no.')

# Create a directory if it doesn't exist.  Recursively create the
# parent directory as well if needed.
def makedir(pathname):
    if os.path.isdir(pathname):
        return
    dirname = os.path.dirname(pathname)
    if dirname: makedir(dirname)
    os.mkdir(pathname, 0o777)

# Write a dictionary to a file in a way that can be read back using
# rval() but is still somewhat readable (i.e. not a single long line).
# Also creates a backup file.
def writedict(dict, filename):
    dir, fname = os.path.split(filename)
    tempname = os.path.join(dir, '@' + fname)
    backup = os.path.join(dir, fname + '~')
    try:
        os.unlink(backup)
    except os.error:
        pass
    fp = open(tempname, 'w')
    fp.write('{\n')
    for key, value in dict.items():
        fp.write('%r: %r,\n' % (key, value))
    fp.write('}\n')
    fp.close()
    try:
        os.rename(filename, backup)
    except os.error:
        pass
    os.rename(tempname, filename)


if __name__ == '__main__':
    main()
