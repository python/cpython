"""distutils.util

General-purpose utility functions used throughout the Distutils
(especially in command classes).  Mostly filesystem manipulation, but
not limited to that.  The functions in this module generally raise
DistutilsFileError when they have problems with the filesystem, because
os.error in pre-1.5.2 Python only gives the error message and not the
file causing it."""

# created 1999/03/08, Greg Ward

__revision__ = "$Id$"

import sys, os, string, re, shutil
from distutils.errors import *
from distutils.spawn import spawn

# cache for by mkpath() -- in addition to cheapening redundant calls,
# eliminates redundant "creating /foo/bar/baz" messages in dry-run mode
PATH_CREATED = {}

# for generating verbose output in 'copy_file()'
_copy_action = { None:   'copying',
                 'hard': 'hard linking',
                 'sym':  'symbolically linking' }

# I don't use os.makedirs because a) it's new to Python 1.5.2, and
# b) it blows up if the directory already exists (I want to silently
# succeed in that case).
def mkpath (name, mode=0777, verbose=0, dry_run=0):
    """Create a directory and any missing ancestor directories.  If the
       directory already exists (or if 'name' is the empty string, which
       means the current directory, which of course exists), then do
       nothing.  Raise DistutilsFileError if unable to create some
       directory along the way (eg. some sub-path exists, but is a file
       rather than a directory).  If 'verbose' is true, print a one-line
       summary of each mkdir to stdout.  Return the list of directories
       actually created."""

    global PATH_CREATED

    # XXX what's the better way to handle verbosity? print as we create
    # each directory in the path (the current behaviour), or only announce
    # the creation of the whole path? (quite easy to do the latter since
    # we're not using a recursive algorithm)

    name = os.path.normpath (name)
    created_dirs = []
    if os.path.isdir (name) or name == '':
        return created_dirs
    if PATH_CREATED.get (name):
        return created_dirs

    (head, tail) = os.path.split (name)
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
                created_dirs.append(head)
            except OSError, exc:
                raise DistutilsFileError, \
                      "could not create '%s': %s" % (head, exc[-1])

        PATH_CREATED[head] = 1
    return created_dirs

# mkpath ()


def create_tree (base_dir, files, mode=0777, verbose=0, dry_run=0):

    """Create all the empty directories under 'base_dir' needed to
       put 'files' there.  'base_dir' is just the a name of a directory
       which doesn't necessarily exist yet; 'files' is a list of filenames
       to be interpreted relative to 'base_dir'.  'base_dir' + the
       directory portion of every file in 'files' will be created if it
       doesn't already exist.  'mode', 'verbose' and 'dry_run' flags are as
       for 'mkpath()'."""

    # First get the list of directories to create
    need_dir = {}
    for file in files:
        need_dir[os.path.join (base_dir, os.path.dirname (file))] = 1
    need_dirs = need_dir.keys()
    need_dirs.sort()

    # Now create them
    for dir in need_dirs:
        mkpath (dir, mode, verbose, dry_run)

# create_tree ()


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
    """Walk two filename lists in parallel, testing if each source is newer
       than its corresponding target.  Return a pair of lists (sources,
       targets) where source is newer than target, according to the
       semantics of 'newer()'."""

    if len (sources) != len (targets):
        raise ValueError, "'sources' and 'targets' must be same length"

    # build a pair of lists (sources, targets) where  source is newer
    n_sources = []
    n_targets = []
    for i in range (len (sources)):
        if newer (sources[i], targets[i]):
            n_sources.append (sources[i])
            n_targets.append (targets[i])

    return (n_sources, n_targets)

# newer_pairwise ()


def newer_group (sources, target, missing='error'):
    """Return true if 'target' is out-of-date with respect to any
       file listed in 'sources'.  In other words, if 'target' exists and
       is newer than every file in 'sources', return false; otherwise
       return true.  'missing' controls what we do when a source file is
       missing; the default ("error") is to blow up with an OSError from
       inside 'stat()'; if it is "ignore", we silently drop any missing
       source files; if it is "newer", any missing source files make us
       assume that 'target' is out-of-date (this is handy in "dry-run"
       mode: it'll make you pretend to carry out commands that wouldn't
       work because inputs are missing, but that doesn't matter because
       you're not actually going to run the commands)."""

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
        if not os.path.exists (source):
            if missing == 'error':      # blow up when we stat() the file
                pass                    
            elif missing == 'ignore':   # missing source dropped from 
                continue                #  target's dependency list
            elif missing == 'newer':    # missing source means target is
                return 1                #  out-of-date
            
        source_mtime = os.stat(source)[ST_MTIME]
        if source_mtime > target_mtime:
            return 1
    else:
        return 0

# newer_group ()


# XXX this isn't used anywhere, and worse, it has the same name as a method
# in Command with subtly different semantics.  (This one just has one
# source -> one dest; that one has many sources -> one dest.)  Nuke it?
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
            raise DistutilsFileError, \
                  "could not open '%s': %s" % (src, errstr)
        
        try:
            fdst = open(dst, 'wb')
        except os.error, (errno, errstr):
            raise DistutilsFileError, \
                  "could not create '%s': %s" % (dst, errstr)
        
        while 1:
            try:
                buf = fsrc.read (buffer_size)
            except os.error, (errno, errstr):
                raise DistutilsFileError, \
                      "could not read from '%s': %s" % (src, errstr)
            
            if not buf:
                break

            try:
                fdst.write(buf)
            except os.error, (errno, errstr):
                raise DistutilsFileError, \
                      "could not write to '%s': %s" % (dst, errstr)
            
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
               link=None,
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

       'link' allows you to make hard links (os.link) or symbolic links
       (os.symlink) instead of copying: set it to "hard" or "sym"; if it
       is None (the default), files are copied.  Don't set 'link' on
       systems that don't support it: 'copy_file()' doesn't check if
       hard or symbolic linking is availalble.

       Under Mac OS, uses the native file copy function in macostools;
       on other systems, uses '_copy_file_contents()' to copy file
       contents.

       Return true if the file was copied (or would have been copied),
       false otherwise (ie. 'update' was true and the destination is
       up-to-date)."""

    # XXX if the destination file already exists, we clobber it if
    # copying, but blow up if linking.  Hmmm.  And I don't know what
    # macostools.copyfile() does.  Should definitely be consistent, and
    # should probably blow up if destination exists and we would be
    # changing it (ie. it's not already a hard/soft link to src OR
    # (not update) and (src newer than dst).

    from stat import *

    if not os.path.isfile (src):
        raise DistutilsFileError, \
              "can't copy '%s': doesn't exist or not a regular file" % src

    if os.path.isdir (dst):
        dir = dst
        dst = os.path.join (dst, os.path.basename (src))
    else:
        dir = os.path.dirname (dst)

    if update and not newer (src, dst):
        if verbose:
            print "not copying %s (output up-to-date)" % src
        return 0

    try:
        action = _copy_action[link]
    except KeyError:
        raise ValueError, \
              "invalid value '%s' for 'link' argument" % link
    if verbose:
        print "%s %s -> %s" % (action, src, dir)

    if dry_run:
        return 1

    # On a Mac, use the native file copy routine
    if os.name == 'mac':
        import macostools
        try:
            macostools.copy (src, dst, 0, preserve_times)
        except OSError, exc:
            raise DistutilsFileError, \
                  "could not copy '%s' to '%s': %s" % (src, dst, exc[-1])
    
    # If linking (hard or symbolic), use the appropriate system call
    # (Unix only, of course, but that's the caller's responsibility)
    elif link == 'hard':
        if not (os.path.exists (dst) and os.path.samefile (src, dst)):
            os.link (src, dst)
    elif link == 'sym':
        if not (os.path.exists (dst) and os.path.samefile (src, dst)):
            os.symlink (src, dst)

    # Otherwise (non-Mac, not linking), copy the file contents and
    # (optionally) copy the times and mode.
    else:
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
       directory, raise DistutilsFileError.  If 'dst' does not exist, it is
       created with 'mkpath()'.  The end result of the copy is that every
       file in 'src' is copied to 'dst', and directories under 'src' are
       recursively copied to 'dst'.  Return the list of files that were
       copied or might have been copied, using their output name.  The
       return value is unaffected by 'update' or 'dry_run': it is simply
       the list of all files under 'src', with the names changed to be
       under 'dst'.

       'preserve_mode' and 'preserve_times' are the same as for
       'copy_file'; note that they only apply to regular files, not to
       directories.  If 'preserve_symlinks' is true, symlinks will be
       copied as symlinks (on platforms that support them!); otherwise
       (the default), the destination of the symlink will be copied.
       'update' and 'verbose' are the same as for 'copy_file'."""

    if not dry_run and not os.path.isdir (src):
        raise DistutilsFileError, \
              "cannot copy tree '%s': not a directory" % src    
    try:
        names = os.listdir (src)
    except os.error, (errno, errstr):
        if dry_run:
            names = []
        else:
            raise DistutilsFileError, \
                  "error listing files in '%s': %s" % (src, errstr)

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
            outputs.extend (
                copy_tree (src_name, dst_name,
                           preserve_mode, preserve_times, preserve_symlinks,
                           update, verbose, dry_run))
        else:
            copy_file (src_name, dst_name,
                       preserve_mode, preserve_times,
                       update, None, verbose, dry_run)
            outputs.append (dst_name)

    return outputs

# copy_tree ()


def remove_tree (directory, verbose=0, dry_run=0):
    """Recursively remove an entire directory tree.  Any errors are ignored
       (apart from being reported to stdout if 'verbose' is true)."""

    if verbose:
        print "removing '%s' (and everything under it)" % directory
    if dry_run:
        return
    try:
        shutil.rmtree(directory,1)
    except (IOError, OSError), exc:
        if verbose:
            if exc.filename:
                print "error removing %s: %s (%s)" % \
                       (directory, exc.strerror, exc.filename)
            else:
                print "error removing %s: %s" % (directory, exc.strerror)


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
    """Create a file with the specified name and write 'contents' (a
       sequence of strings without line terminators) to it."""

    f = open (filename, "w")
    for line in contents:
        f.write (line + "\n")
    f.close ()


def get_platform ():
    """Return a string (suitable for tacking onto directory names) that
       identifies the current platform.  Under Unix, identifies both the OS
       and hardware architecture, e.g. "linux-i586", "solaris-sparc",
       "irix-mips".  For Windows and Mac OS, just returns 'sys.platform' --
       i.e. "???" or "???"."""

    if os.name == 'posix':
        uname = os.uname()
        OS = uname[0]
        arch = uname[4]
        return "%s-%s" % (string.lower (OS), string.lower (arch))
    else:
        return sys.platform

# get_platform()


def native_path (pathname):
    """Return 'pathname' as a name that will work on the native
       filesystem, i.e. split it on '/' and put it back together again
       using the current directory separator.  Needed because filenames in
       the setup script are always supplied in Unix style, and have to be
       converted to the local convention before we can actually use them in
       the filesystem.  Raises DistutilsValueError if 'pathname' is
       absolute (starts with '/') or contains local directory separators
       (unless the local separator is '/', of course)."""

    if pathname[0] == '/':
        raise DistutilsValueError, "path '%s' cannot be absolute" % pathname
    if pathname[-1] == '/':
        raise DistutilsValueError, "path '%s' cannot end with '/'" % pathname
    if os.sep != '/' and os.sep in pathname:
        raise DistutilsValueError, \
              "path '%s' cannot contain '%c' character" % \
              (pathname, os.sep)

        paths = string.split (pathname, '/')
        return apply (os.path.join, paths)
    else:
        return pathname

# native_path ()


def _check_environ ():
    """Ensure that 'os.environ' has all the environment variables we
       guarantee that users can use in config files, command-line
       options, etc.  Currently this includes:
         HOME - user's home directory (Unix only)
         PLAT - desription of the current platform, including hardware
                and OS (see 'get_platform()')
    """

    if os.name == 'posix' and not os.environ.has_key('HOME'):
        import pwd
        os.environ['HOME'] = pwd.getpwuid (os.getuid())[5]

    if not os.environ.has_key('PLAT'):
        os.environ['PLAT'] = get_platform ()


def subst_vars (str, local_vars):
    """Perform shell/Perl-style variable substitution on 'string'.
       Every occurence of '$' followed by a name, or a name enclosed in
       braces, is considered a variable.  Every variable is substituted by
       the value found in the 'local_vars' dictionary, or in 'os.environ'
       if it's not in 'local_vars'.  'os.environ' is first checked/
       augmented to guarantee that it contains certain values: see
       '_check_environ()'.  Raise ValueError for any variables not found in
       either 'local_vars' or 'os.environ'."""

    _check_environ ()
    def _subst (match, local_vars=local_vars):
        var_name = match.group(1)
        if local_vars.has_key (var_name):
            return str (local_vars[var_name])
        else:
            return os.environ[var_name]

    return re.sub (r'\$([a-zA-Z_][a-zA-Z_0-9]*)', _subst, str)

# subst_vars ()


def make_tarball (base_name, base_dir, compress="gzip",
                  verbose=0, dry_run=0):
    """Create a (possibly compressed) tar file from all the files under
       'base_dir'.  'compress' must be "gzip" (the default), "compress", or
       None.  Both "tar" and the compression utility named by 'compress'
       must be on the default program search path, so this is probably
       Unix-specific.  The output tar file will be named 'base_dir' +
       ".tar", possibly plus the appropriate compression extension
       (".gz" or ".Z").  Return the output filename."""

    # XXX GNU tar 1.13 has a nifty option to add a prefix directory.
    # It's pretty new, though, so we certainly can't require it --
    # but it would be nice to take advantage of it to skip the
    # "create a tree of hardlinks" step!  (Would also be nice to
    # detect GNU tar to use its 'z' option and save a step.)

    compress_ext = { 'gzip': ".gz",
                     'compress': ".Z" }

    if compress is not None and compress not in ('gzip', 'compress'):
        raise ValueError, \
              "bad value for 'compress': must be None, 'gzip', or 'compress'"

    archive_name = base_name + ".tar"
    cmd = ["tar", "-cf", archive_name, base_dir]
    spawn (cmd, verbose=verbose, dry_run=dry_run)

    if compress:
        spawn ([compress, archive_name], verbose=verbose, dry_run=dry_run)
        return archive_name + compress_ext[compress]
    else:
        return archive_name

# make_tarball ()


def make_zipfile (base_name, base_dir, verbose=0, dry_run=0):
    """Create a zip file from all the files under 'base_dir'.  The
       output zip file will be named 'base_dir' + ".zip".  Uses either the
       InfoZIP "zip" utility (if installed and found on the default search
       path) or the "zipfile" Python module (if available).  If neither
       tool is available, raises DistutilsExecError.  Returns the name
       of the output zip file."""

    # This initially assumed the Unix 'zip' utility -- but
    # apparently InfoZIP's zip.exe works the same under Windows, so
    # no changes needed!

    zip_filename = base_name + ".zip"
    try:
        spawn (["zip", "-rq", zip_filename, base_dir],
               verbose=verbose, dry_run=dry_run)
    except DistutilsExecError:

        # XXX really should distinguish between "couldn't find
        # external 'zip' command" and "zip failed" -- shouldn't try
        # again in the latter case.  (I think fixing this will
        # require some cooperation from the spawn module -- perhaps
        # a utility function to search the path, so we can fallback
        # on zipfile.py without the failed spawn.)
        try:
            import zipfile
        except ImportError:
            raise DistutilsExecError, \
                  ("unable to create zip file '%s': " + 
                   "could neither find a standalone zip utility nor " +
                   "import the 'zipfile' module") % zip_filename

        if verbose:
            print "creating '%s' and adding '%s' to it" % \
                  (zip_filename, base_dir)

        def visit (z, dirname, names):
            for name in names:
                path = os.path.join (dirname, name)
                if os.path.isfile (path):
                    z.write (path, path)

        if not dry_run:
            z = zipfile.ZipFile (zip_filename, "wb",
                                 compression=zipfile.ZIP_DEFLATED)

            os.path.walk (base_dir, visit, z)
            z.close()

    return zip_filename

# make_zipfile ()


def make_archive (base_name, format,
                  root_dir=None, base_dir=None,
                  verbose=0, dry_run=0):

    """Create an archive file (eg. zip or tar).  'base_name' is the name
    of the file to create, minus any format-specific extension; 'format'
    is the archive format: one of "zip", "tar", "ztar", or "gztar".
    'root_dir' is a directory that will be the root directory of the
    archive; ie. we typically chdir into 'root_dir' before creating the
    archive.  'base_dir' is the directory where we start archiving from;
    ie. 'base_dir' will be the common prefix of all files and
    directories in the archive.  'root_dir' and 'base_dir' both default
    to the current directory."""

    save_cwd = os.getcwd()
    if root_dir is not None:
        if verbose:
            print "changing into '%s'" % root_dir
        base_name = os.path.abspath (base_name)
        if not dry_run:
            os.chdir (root_dir)

    if base_dir is None:
        base_dir = os.curdir

    kwargs = { 'verbose': verbose,
               'dry_run': dry_run }
    
    if format == 'gztar':
        func = make_tarball
        kwargs['compress'] = 'gzip'
    elif format == 'ztar':
        func = make_tarball
        kwargs['compress'] = 'compress'
    elif format == 'tar':
        func = make_tarball
    elif format == 'zip':
        func = make_zipfile

    apply (func, (base_name, base_dir), kwargs)

    if root_dir is not None:
        if verbose:
            print "changing back to '%s'" % save_cwd
        os.chdir (save_cwd)

# make_archive ()
