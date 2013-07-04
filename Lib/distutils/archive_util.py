"""distutils.archive_util

Utility functions for creating archive files (tarballs, zip files,
that sort of thing)."""

import os
from warnings import warn
import sys

try:
    import zipfile
except ImportError:
    zipfile = None


from distutils.errors import DistutilsExecError
from distutils.spawn import spawn
from distutils.dir_util import mkpath
from distutils import log

def make_tarball(base_name, base_dir, compress="gzip", verbose=0, dry_run=0):
    """Create a (possibly compressed) tar file from all the files under
    'base_dir'.

    'compress' must be "gzip" (the default), "compress", "bzip2", or None.
    Both "tar" and the compression utility named by 'compress' must be on
    the default program search path, so this is probably Unix-specific.
    The output tar file will be named 'base_dir' +  ".tar", possibly plus
    the appropriate compression extension (".gz", ".bz2" or ".Z").
    Returns the output filename.
    """
    tar_compression = {'gzip': 'gz', 'bzip2': 'bz2', None: '', 'compress': ''}
    compress_ext = {'gzip': '.gz', 'bzip2': '.bz2', 'compress': '.Z'}

    # flags for compression program, each element of list will be an argument
    if compress is not None and compress not in compress_ext.keys():
        raise ValueError(
              "bad value for 'compress': must be None, 'gzip', 'bzip2' "
              "or 'compress'")

    archive_name = base_name + '.tar'
    if compress != 'compress':
        archive_name += compress_ext.get(compress, '')

    mkpath(os.path.dirname(archive_name), dry_run=dry_run)

    # creating the tarball
    import tarfile  # late import so Python build itself doesn't break

    log.info('Creating tar archive')
    if not dry_run:
        tar = tarfile.open(archive_name, 'w|%s' % tar_compression[compress])
        try:
            tar.add(base_dir)
        finally:
            tar.close()

    # compression using `compress`
    if compress == 'compress':
        warn("'compress' will be deprecated.", PendingDeprecationWarning)
        # the option varies depending on the platform
        compressed_name = archive_name + compress_ext[compress]
        if sys.platform == 'win32':
            cmd = [compress, archive_name, compressed_name]
        else:
            cmd = [compress, '-f', archive_name]
        spawn(cmd, dry_run=dry_run)
        return compressed_name

    return archive_name

def make_zipfile(base_name, base_dir, verbose=0, dry_run=0):
    """Create a zip file from all the files under 'base_dir'.

    The output zip file will be named 'base_name' + ".zip".  Uses either the
    "zipfile" Python module (if available) or the InfoZIP "zip" utility
    (if installed and found on the default search path).  If neither tool is
    available, raises DistutilsExecError.  Returns the name of the output zip
    file.
    """
    zip_filename = base_name + ".zip"
    mkpath(os.path.dirname(zip_filename), dry_run=dry_run)

    # If zipfile module is not available, try spawning an external
    # 'zip' command.
    if zipfile is None:
        if verbose:
            zipoptions = "-r"
        else:
            zipoptions = "-rq"

        try:
            spawn(["zip", zipoptions, zip_filename, base_dir],
                  dry_run=dry_run)
        except DistutilsExecError:
            # XXX really should distinguish between "couldn't find
            # external 'zip' command" and "zip failed".
            raise DistutilsExecError(("unable to create zip file '%s': "
                   "could neither import the 'zipfile' module nor "
                   "find a standalone zip utility") % zip_filename)

    else:
        log.info("creating '%s' and adding '%s' to it",
                 zip_filename, base_dir)

        if not dry_run:
            try:
                zip = zipfile.ZipFile(zip_filename, "w",
                                      compression=zipfile.ZIP_DEFLATED)
            except RuntimeError:
                zip = zipfile.ZipFile(zip_filename, "w",
                                      compression=zipfile.ZIP_STORED)

            for dirpath, dirnames, filenames in os.walk(base_dir):
                for name in filenames:
                    path = os.path.normpath(os.path.join(dirpath, name))
                    if os.path.isfile(path):
                        zip.write(path, path)
                        log.info("adding '%s'" % path)
            zip.close()

    return zip_filename

ARCHIVE_FORMATS = {
    'gztar': (make_tarball, [('compress', 'gzip')], "gzip'ed tar-file"),
    'bztar': (make_tarball, [('compress', 'bzip2')], "bzip2'ed tar-file"),
    'ztar':  (make_tarball, [('compress', 'compress')], "compressed tar file"),
    'tar':   (make_tarball, [('compress', None)], "uncompressed tar file"),
    'zip':   (make_zipfile, [],"ZIP file")
    }

def check_archive_formats(formats):
    """Returns the first format from the 'format' list that is unknown.

    If all formats are known, returns None
    """
    for format in formats:
        if format not in ARCHIVE_FORMATS:
            return format
    return None

def make_archive(base_name, format, root_dir=None, base_dir=None, verbose=0,
                 dry_run=0):
    """Create an archive file (eg. zip or tar).

    'base_name' is the name of the file to create, minus any format-specific
    extension; 'format' is the archive format: one of "zip", "tar", "ztar",
    or "gztar".

    'root_dir' is a directory that will be the root directory of the
    archive; ie. we typically chdir into 'root_dir' before creating the
    archive.  'base_dir' is the directory where we start archiving from;
    ie. 'base_dir' will be the common prefix of all files and
    directories in the archive.  'root_dir' and 'base_dir' both default
    to the current directory.  Returns the name of the archive file.
    """
    save_cwd = os.getcwd()
    if root_dir is not None:
        log.debug("changing into '%s'", root_dir)
        base_name = os.path.abspath(base_name)
        if not dry_run:
            os.chdir(root_dir)

    if base_dir is None:
        base_dir = os.curdir

    kwargs = {'dry_run': dry_run}

    try:
        format_info = ARCHIVE_FORMATS[format]
    except KeyError:
        raise ValueError("unknown archive format '%s'" % format)

    func = format_info[0]
    for arg, val in format_info[1]:
        kwargs[arg] = val
    try:
        filename = func(base_name, base_dir, **kwargs)
    finally:
        if root_dir is not None:
            log.debug("changing back to '%s'", save_cwd)
            os.chdir(save_cwd)

    return filename
