"""distutils.archive_util

Utility functions for creating archive files (tarballs, zip files,
that sort of thing)."""

# created 2000/04/03, Greg Ward (extracted from util.py)

__revision__ = "$Id$"

import os
from distutils.errors import DistutilsExecError
from distutils.spawn import spawn


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
        kwargs['compress'] = None
    elif format == 'zip':
        func = make_zipfile

    apply (func, (base_name, base_dir), kwargs)

    if root_dir is not None:
        if verbose:
            print "changing back to '%s'" % save_cwd
        os.chdir (save_cwd)

# make_archive ()
