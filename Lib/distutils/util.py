"""distutils.util

General-purpose utility functions used throughout the Distutils
(especially in command classes).  Mostly filesystem manipulation, but
not limited to that.  The functions in this module generally raise
DistutilsFileError when they have problems with the filesystem, because
os.error in pre-1.5.2 Python only gives the error message and not the
file causing it."""

# created 1999/03/08, Greg Ward

__rcsid__ = "$Id$"

import os
from distutils.errors import *


# cache for by mkpath() -- in addition to cheapening redundant calls,
# eliminates redundant "creating /foo/bar/baz" messages in dry-run mode
PATH_CREATED = {}

# I don't use os.makedirs because a) it's new to Python 1.5.2, and
# b) it blows up if the directory already exists (I want to silently
# succeed in that case).
def mkpath (name, mode=0777, verbose=0, dry_run=0):
    """Create a directory and any missing ancestor directories.  If the
       directory already exists, return silently.  Raise
       DistutilsFileError if unable to create some directory along the
       way (eg. some sub-path exists, but is a file rather than a
       directory).  If 'verbose' is true, print a one-line summary of
       each mkdir to stdout."""

    global PATH_CREATED

    # XXX what's the better way to handle verbosity? print as we create
    # each directory in the path (the current behaviour), or only announce
    # the creation of the whole path? (quite easy to do the latter since
    # we're not using a recursive algorithm)

    if os.path.isdir (name):
        return
    if PATH_CREATED.get (name):
        return

    (head, tail) = os.path.split (name)
    if not tail:                        # in case 'name' has trailing slash
        (head, tail) = os.path.split (head)
    tails = [tail]                      # stack of lone dirs to create
    
    while head and tail and not os.path.isdir (head):
        #print "splitting '%s': " % head,
        (head, tail) = os.path.split (head)
        #print "to ('%s','%s')" % (head, tail)
        tails.insert (0, tail)          # push next higher dir onto stack

    #print "stack of tails:", tails

    # now 'head' contains the deepest directory that already exists
    # (that is, the child of 'head' in 'name' is the highest directory
    # that does *not* exist)
    for d in tails:
        #print "head = %s, d = %s: " % (head, d),
        head = os.path.join (head, d)
        if PATH_CREATED.get (head):
            continue

        if verbose:
            print "creating", head

        if not dry_run:
            try:
                os.mkdir (head)
            except os.error, (errno, errstr):
                raise DistutilsFileError, "%s: %s" % (head, errstr)

        PATH_CREATED[head] = 1

# mkpath ()


def newer (source, target):
    """Return true if 'source' exists and is more recently modified than
       'target', or if 'source' exists and 'target' doesn't.  Return
       false if both exist and 'target' is the same age or younger than
       'source'.  Raise DistutilsFileError if 'source' does not
       exist."""

    if not os.path.exists (source):
        raise DistutilsFileError, "file '%s' does not exist" % source
    if not os.path.exists (target):
        return 1

    from stat import ST_MTIME
    mtime1 = os.stat(source)[ST_MTIME]
    mtime2 = os.stat(target)[ST_MTIME]

    return mtime1 > mtime2

# newer ()


def newer_pairwise (sources, targets):

    """Walk two filename lists in parallel, testing if each 'target' is
       up-to-date relative to its corresponding 'source'.  If so, both
       are deleted from their respective lists.  Return a list of tuples
       containing the deleted (source,target) pairs."""

    if len (sources) != len (targets):
        raise ValueError, "'sources' and 'targets' must be same length"

    goners = []
    for i in range (len (sources)-1, -1, -1):
        if not newer (sources[i], targets[i]):
            goners.append ((sources[i], targets[i]))
            del sources[i]
            del targets[i]
    goners.reverse()
    return goners

# newer_pairwise ()


def newer_group (sources, target):
    """Return true if 'target' is out-of-date with respect to any
       file listed in 'sources'.  In other words, if 'target' exists and
       is newer than every file in 'sources', return false; otherwise
       return true."""

    # If the target doesn't even exist, then it's definitely out-of-date.
    if not os.path.exists (target):
        return 1
   
    # Otherwise we have to find out the hard way: if *any* source file
    # is more recent than 'target', then 'target' is out-of-date and
    # we can immediately return true.  If we fall through to the end
    # of the loop, then 'target' is up-to-date and we return false.
    from stat import ST_MTIME
    target_mtime = os.stat (target)[ST_MTIME]
    for source in sources:
        source_mtime = os.stat(source)[ST_MTIME]
        if source_mtime > target_mtime:
            return 1
    else:
        return 0

# newer_group ()


def make_file (src, dst, func, args,
               verbose=0, update_message=None, noupdate_message=None):
    """Makes 'dst' from 'src' (both filenames) by calling 'func' with
       'args', but only if it needs to: i.e. if 'dst' does not exist or
       'src' is newer than 'dst'."""

    if newer (src, dst):
        if verbose and update_message:
            print update_message
        apply (func, args)
    else:
        if verbose and noupdate_message:
            print noupdate_message

# make_file ()


def _copy_file_contents (src, dst, buffer_size=16*1024):
    """Copy the file 'src' to 'dst'; both must be filenames.  Any error
       opening either file, reading from 'src', or writing to 'dst',
       raises DistutilsFileError.  Data is read/written in chunks of
       'buffer_size' bytes (default 16k).  No attempt is made to handle
       anything apart from regular files."""

    # Stolen from shutil module in the standard library, but with
    # custom error-handling added.

    fsrc = None
    fdst = None
    try:
        try:
            fsrc = open(src, 'rb')
        except os.error, (errno, errstr):
            raise DistutilsFileError, "could not open %s: %s" % (src, errstr)
        
        try:
            fdst = open(dst, 'wb')
        except os.error, (errno, errstr):
            raise DistutilsFileError, "could not create %s: %s" % (dst, errstr)
        
        while 1:
            try:
                buf = fsrc.read (buffer_size)
            except os.error, (errno, errstr):
                raise DistutilsFileError, \
                      "could not read from %s: %s" % (src, errstr)
            
            if not buf:
                break

            try:
                fdst.write(buf)
            except os.error, (errno, errstr):
                raise DistutilsFileError, \
                      "could not write to %s: %s" % (dst, errstr)
            
    finally:
        if fdst:
            fdst.close()
        if fsrc:
            fsrc.close()

# _copy_file_contents()


def copy_file (src, dst,
               preserve_mode=1,
               preserve_times=1,
               update=0,
               verbose=0,
               dry_run=0):

    """Copy a file 'src' to 'dst'.  If 'dst' is a directory, then 'src'
       is copied there with the same name; otherwise, it must be a
       filename.  (If the file exists, it will be ruthlessly clobbered.)
       If 'preserve_mode' is true (the default), the file's mode (type
       and permission bits, or whatever is analogous on the current
       platform) is copied.  If 'preserve_times' is true (the default),
       the last-modified and last-access times are copied as well.  If
       'update' is true, 'src' will only be copied if 'dst' does not
       exist, or if 'dst' does exist but is older than 'src'.  If
       'verbose' is true, then a one-line summary of the copy will be
       printed to stdout.

       Return true if the file was copied (or would have been copied),
       false otherwise (ie. 'update' was true and the destination is
       up-to-date)."""

    # XXX doesn't copy Mac-specific metadata
       
    from stat import *

    if not os.path.isfile (src):
        raise DistutilsFileError, \
              "can't copy %s: not a regular file" % src

    if os.path.isdir (dst):
        dir = dst
        dst = os.path.join (dst, os.path.basename (src))
    else:
        dir = os.path.dirname (dst)

    if update and not newer (src, dst):
        if verbose:
            print "not copying %s (output up-to-date)" % src
        return 0

    if verbose:
        print "copying %s -> %s" % (src, dir)

    if dry_run:
        return 1

    _copy_file_contents (src, dst)
    if preserve_mode or preserve_times:
        st = os.stat (src)

        # According to David Ascher <da@ski.org>, utime() should be done
        # before chmod() (at least under NT).
        if preserve_times:
            os.utime (dst, (st[ST_ATIME], st[ST_MTIME]))
        if preserve_mode:
            os.chmod (dst, S_IMODE (st[ST_MODE]))

    return 1

# copy_file ()


def copy_tree (src, dst,
               preserve_mode=1,
               preserve_times=1,
               preserve_symlinks=0,
               update=0,
               verbose=0,
               dry_run=0):


    """Copy an entire directory tree 'src' to a new location 'dst'.  Both
       'src' and 'dst' must be directory names.  If 'src' is not a
       directory, raise DistutilsFileError.  If 'dst' does not exist, it
       is created with 'mkpath'.  The end result of the copy is that
       every file in 'src' is copied to 'dst', and directories under
       'src' are recursively copied to 'dst'.  Return the list of files
       copied (under their output names) -- note that if 'update' is true,
       this might be less than the list of files considered.  Return
       value is not affected by 'dry_run'.

       'preserve_mode' and 'preserve_times' are the same as for
       'copy_file'; note that they only apply to regular files, not to
       directories.  If 'preserve_symlinks' is true, symlinks will be
       copied as symlinks (on platforms that support them!); otherwise
       (the default), the destination of the symlink will be copied.
       'update' and 'verbose' are the same as for 'copy_file'."""

    if not dry_run and not os.path.isdir (src):
        raise DistutilsFileError, \
              "cannot copy tree %s: not a directory" % src    
    try:
        names = os.listdir (src)
    except os.error, (errno, errstr):
        if dry_run:
            names = []
        else:
            raise DistutilsFileError, \
                  "error listing files in %s: %s" % (src, errstr)

    if not dry_run:
        mkpath (dst, verbose=verbose)

    outputs = []

    for n in names:
        src_name = os.path.join (src, n)
        dst_name = os.path.join (dst, n)

        if preserve_symlinks and os.path.islink (src_name):
            link_dest = os.readlink (src_name)
            if verbose:
                print "linking %s -> %s" % (dst_name, link_dest)
            if not dry_run:
                os.symlink (link_dest, dst_name)
            outputs.append (dst_name)
            
        elif os.path.isdir (src_name):
            outputs[-1:] = \
                copy_tree (src_name, dst_name,
                           preserve_mode, preserve_times, preserve_symlinks,
                           update, verbose, dry_run)
        else:
            if (copy_file (src_name, dst_name,
                           preserve_mode, preserve_times,
                           update, verbose, dry_run)):
                outputs.append (dst_name)

    return outputs

# copy_tree ()


# XXX I suspect this is Unix-specific -- need porting help!
def move_file (src, dst,
               verbose=0,
               dry_run=0):

    """Move a file 'src' to 'dst'.  If 'dst' is a directory, the file
       will be moved into it with the same name; otherwise, 'src' is
       just renamed to 'dst'.  Return the new full name of the file.

       Handles cross-device moves on Unix using
       'copy_file()'.  What about other systems???"""

    from os.path import exists, isfile, isdir, basename, dirname

    if verbose:
        print "moving %s -> %s" % (src, dst)

    if dry_run:
        return dst

    if not isfile (src):
        raise DistutilsFileError, \
              "can't move '%s': not a regular file" % src

    if isdir (dst):
        dst = os.path.join (dst, basename (src))
    elif exists (dst):
        raise DistutilsFileError, \
              "can't move '%s': destination '%s' already exists" % \
              (src, dst)

    if not isdir (dirname (dst)):
        raise DistutilsFileError, \
              "can't move '%s': destination '%s' not a valid path" % \
              (src, dst)

    copy_it = 0
    try:
        os.rename (src, dst)
    except os.error, (num, msg):
        if num == errno.EXDEV:
            copy_it = 1
        else:
            raise DistutilsFileError, \
                  "couldn't move '%s' to '%s': %s" % (src, dst, msg)

    if copy_it:
        copy_file (src, dst)
        try:
            os.unlink (src)
        except os.error, (num, msg):
            try:
                os.unlink (dst)
            except os.error:
                pass
            raise DistutilsFileError, \
                  ("couldn't move '%s' to '%s' by copy/delete: " + 
                   "delete '%s' failed: %s") % \
                  (src, dst, src, msg)

    return dst

# move_file ()


def write_file (filename, contents):
    """Create a file with the specified naem and write 'contents' (a
       sequence of strings without line terminators) to it."""

    f = open (filename, "w")
    for line in contents:
        f.write (line + "\n")
    f.close ()
