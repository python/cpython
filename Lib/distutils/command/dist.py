"""distutils.command.dist

Implements the Distutils 'dist' command (create a source distribution)."""

# created 1999/09/22, Greg Ward

__rcsid__ = "$Id$"

import sys, os, string, re
import fnmatch
from types import *
from glob import glob
from shutil import rmtree
from distutils.core import Command
from distutils.text_file import TextFile
from distutils.errors import DistutilsExecError


# Possible modes of operation:
#   - require an explicit manifest that lists every single file (presumably
#     along with a way to auto-generate the manifest)
#   - require an explicit manifest, but allow it to have globs or
#     filename patterns of some kind (and also have auto-generation)
#   - allow an explict manifest, but automatically augment it at runtime
#     with the source files mentioned in 'packages', 'py_modules', and
#     'ext_modules' (and any other such things that might come along)

# I'm liking the third way.  Possible gotchas:
#   - redundant specification: 'packages' includes 'foo' and manifest
#     includes 'foo/*.py'
#   - obvious conflict: 'packages' includes 'foo' and manifest
#     includes '! foo/*.py' (can't imagine why you'd want this)
#   - subtle conflict:  'packages' includes 'foo' and manifest
#     includes '! foo/bar.py' (this could well be desired: eg. exclude
#     an experimental module from distribution)

# Syntax for the manifest file:
#   - if a line is just a Unix-style glob by itself, it's a "simple include
#     pattern": go find all files that match and add them to the list
#     of files
#   - if a line is a glob preceded by "!", then it's a "simple exclude
#     pattern": go over the current list of files and exclude any that
#     match the glob pattern
#   - if a line consists of a directory name followed by zero or more
#     glob patterns, then we'll recursively explore that directory tree
#     - the glob patterns can be include (no punctuation) or exclude
#       (prefixed by "!", no space)
#     - if no patterns given or the first pattern is not an include pattern,
#       then assume "*" -- ie. find everything (and then start applying
#       the rest of the patterns)
#     - the patterns are given in order of increasing precedence, ie.
#       the *last* one to match a given file applies to it
# 
# example (ignoring auto-augmentation!):
#   distutils/*.py
#   distutils/command/*.py
#   ! distutils/bleeding_edge.py
#   examples/*.py
#   examples/README
# 
# smarter way (that *will* include distutils/command/bleeding_edge.py!)
#   distutils *.py
#   ! distutils/bleeding_edge.py
#   examples !*~ !*.py[co]     (same as: examples * !*~ !*.py[co])
#   test test_* *.txt !*~ !*.py[co]
#   README
#   setup.py
#
# The actual Distutils manifest (don't need to mention source files,
# README, setup.py -- they're automatically distributed!):
#   examples !*~ !*.py[co]
#   test !*~ !*.py[co]

# The algorithm that will make it work:
#   files = stuff from 'packages', 'py_modules', 'ext_modules',
#     plus README, setup.py, ... ?
#   foreach pattern in manifest file:
#     if simple-include-pattern:         # "distutils/*.py"
#       files.append (glob (pattern))
#     elif simple-exclude-pattern:       # "! distutils/foo*"
#       xfiles = glob (pattern)
#       remove all xfiles from files
#     elif recursive-pattern:            # "examples" (just a directory name)
#       patterns = rest-of-words-on-line
#       dir_files = list of all files under dir
#       if patterns:
#         if patterns[0] is an exclude-pattern:
#           insert "*" at patterns[0]
#         for file in dir_files:
#           for dpattern in reverse (patterns):
#             if file matches dpattern:
#               if dpattern is an include-pattern:
#                 files.append (file)
#               else:
#                 nothing, don't include it
#               next file
#       else:
#         files.extend (dir_files)    # ie. accept all of them


# Anyways, this is all implemented below -- BUT it is largely untested; I
# know it works for the simple case of distributing the Distutils, but
# haven't tried it on more complicated examples.  Undoubtedly doing so will
# reveal bugs and cause delays, so I'm waiting until after I've released
# Distutils 0.1.


# Other things we need to look for in creating a source distribution:
#   - make sure there's a README
#   - make sure the distribution meta-info is supplied and non-empty
#     (*must* have name, version, ((author and author_email) or
#     (maintainer and maintainer_email)), url
#
# Frills:
#   - make sure the setup script is called "setup.py"
#   - make sure the README refers to "setup.py" (ie. has a line matching
#     /^\s*python\s+setup\.py/)

# A crazy idea that conflicts with having/requiring 'version' in setup.py:
#   - make sure there's a version number in the "main file" (main file
#     is __init__.py of first package, or the first module if no packages,
#     or the first extension module if no pure Python modules)
#   - XXX how do we look for __version__ in an extension module?
#   - XXX do we import and look for __version__? or just scan source for
#     /^__version__\s*=\s*"[^"]+"/ ?
#   - what about 'version_from' as an alternative to 'version' -- then
#     we know just where to search for the version -- no guessing about
#     what the "main file" is



class Dist (Command):

    description = "create a source distribution (tarball, zip file, etc.)"

    options = [('formats=', None,
                "formats for source distribution (tar, ztar, gztar, or zip)"),
               ('manifest=', 'm',
                "name of manifest file"),
               ('list-only', 'l',
                "just list files that would be distributed"),
               ('keep-tree', 'k',
                "keep the distribution tree around after creating " +
                "archive file(s)"),
              ]

    default_format = { 'posix': 'gztar',
                       'nt': 'zip' }

    exclude_re = re.compile (r'\s*!\s*(\S+)') # for manifest lines


    def set_default_options (self):
        self.formats = None
        self.manifest = None
        self.list_only = 0
        self.keep_tree = 0


    def set_final_options (self):
        if self.formats is None:
            try:
                self.formats = [self.default_format[os.name]]
            except KeyError:
                raise DistutilsPlatformError, \
                      "don't know how to build source distributions on " + \
                      "%s platform" % os.name
        elif type (self.formats) is StringType:
            self.formats = string.split (self.formats, ',')

        if self.manifest is None:
            self.manifest = "MANIFEST"


    def run (self):

        self.check_metadata ()

        self.files = []
        self.find_defaults ()
        self.read_manifest ()

        if self.list_only:
            for f in self.files:
                print f

        else:
            self.make_distribution ()


    def check_metadata (self):

        dist = self.distribution

        missing = []
        for attr in ('name', 'version', 'url'):
            if not (hasattr (dist, attr) and getattr (dist, attr)):
                missing.append (attr)

        if missing:
            self.warn ("missing required meta-data: " +
                       string.join (missing, ", "))

        if dist.author:
            if not dist.author_email:
                self.warn ("missing meta-data: if 'author' supplied, " +
                           "'author_email' must be supplied too")
        elif dist.maintainer:
            if not dist.maintainer_email:
                self.warn ("missing meta-data: if 'maintainer' supplied, " +
                           "'maintainer_email' must be supplied too")
        else:
            self.warn ("missing meta-data: either (author and author_email) " +
                       "or (maintainer and maintainer_email) " +
                       "must be supplied")

    # check_metadata ()


    def find_defaults (self):

        standards = ['README', 'setup.py']
        for fn in standards:
            if os.path.exists (fn):
                self.files.append (fn)
            else:
                self.warn ("standard file %s not found" % fn)

        optional = ['test/test*.py']
        for pattern in optional:
            files = filter (os.path.isfile, glob (pattern))
            if files:
                self.files.extend (files)

        if self.distribution.packages or self.distribution.py_modules:
            build_py = self.find_peer ('build_py')
            build_py.ensure_ready ()
            self.files.extend (build_py.get_source_files ())

        if self.distribution.ext_modules:
            build_ext = self.find_peer ('build_ext')
            build_ext.ensure_ready ()
            self.files.extend (build_ext.get_source_files ())



    def open_manifest (self, filename):
        return TextFile (filename,
                         strip_comments=1,
                         skip_blanks=1,
                         join_lines=1,
                         lstrip_ws=1,
                         rstrip_ws=1,
                         collapse_ws=1)


    def search_dir (self, dir, patterns):

        allfiles = findall (dir)
        if patterns:
            if patterns[0][0] == "!":   # starts with an exclude spec?
                patterns.insert (0, "*")# then accept anything that isn't
                                        # explicitly excluded

            act_patterns = []           # "action-patterns": (include,regexp)
                                        # tuples where include is a boolean
            for pattern in patterns:
                if pattern[0] == '!':
                    act_patterns.append \
                        ((0, re.compile (fnmatch.translate (pattern[1:]))))
                else:
                    act_patterns.append \
                        ((1, re.compile (fnmatch.translate (pattern))))
            act_patterns.reverse()


            files = []
            for file in allfiles:
                for (include,regexp) in act_patterns:
                    if regexp.search (file):
                        if include:
                            files.append (file)
                        break           # continue to next file
        else:
            files = allfiles

        return files

    # search_dir ()


    def exclude_files (self, pattern):

        regexp = re.compile (fnmatch.translate (pattern))
        for i in range (len (self.files)-1, -1, -1):
            if regexp.search (self.files[i]):
                del self.files[i]


    def read_manifest (self):

        # self.files had better already be defined (and hold the
        # "automatically found" files -- Python modules and extensions,
        # README, setup script, ...)
        assert self.files is not None

        try:
            manifest = self.open_manifest (self.manifest)
        except IOError, exc:
            if type (exc) is InstanceType and hasattr (exc, 'strerror'):
                msg = "could not open MANIFEST (%s)" % \
                      string.lower (exc.strerror)
            else:
                msg = "could not open MANIFST"
    
            self.warn (msg + ": using default file list")
            return
        
        while 1:

            pattern = manifest.readline()
            if pattern is None:            # end of file
                break

            # Cases:
            #   1) simple-include: "*.py", "foo/*.py", "doc/*.html", "FAQ"
            #   2) simple-exclude: same, prefaced by !
            #   3) recursive: multi-word line, first word a directory

            exclude = self.exclude_re.match (pattern)
            if exclude:
                pattern = exclude.group (1)

            words = string.split (pattern)
            assert words                # must have something!
            if os.name != 'posix':
                words[0] = apply (os.path.join, string.split (words[0], '/'))

            # First word is a directory, possibly with include/exclude
            # patterns making up the rest of the line: it's a recursive
            # pattern
            if os.path.isdir (words[0]):
                if exclude:
                    manifest.warn ("exclude (!) doesn't apply to " +
                                   "whole directory trees")
                    continue

                dir_files = self.search_dir (words[0], words[1:])
                self.files.extend (dir_files)

            # Multiple words in pattern: that's a no-no unless the first
            # word is a directory name
            elif len (words) > 1:
                manifest.warn ("can't have multiple words unless first word " +
                               "('%s') is a directory name" % words[0])
                continue

            # Single word, no bang: it's a "simple include pattern"
            elif not exclude:
                matches = filter (os.path.isfile, glob (pattern))
                if matches:
                    self.files.extend (matches)
                else:
                    manifest.warn ("no matches for '%s' found" % pattern)


            # Single word prefixed with a bang: it's a "simple exclude pattern"
            else:
                if self.exclude_files (pattern) == 0:
                    manifest.warn ("no files excluded by '%s'" % pattern)

            # if/elif/.../else on 'pattern'

        # loop over lines of 'manifest'

    # read_manifest ()


    def make_release_tree (self, base_dir, files):

        # XXX this is Unix-specific

        # First get the list of directories to create
        need_dir = {}
        for file in files:
            need_dir[os.path.join (base_dir, os.path.dirname (file))] = 1
        need_dirs = need_dir.keys()
        need_dirs.sort()

        # Now create them
        for dir in need_dirs:
            self.mkpath (dir)

        # And walk over the list of files, either making a hard link (if
        # os.link exists) to each one that doesn't already exist in its
        # corresponding location under 'base_dir', or copying each file
        # that's out-of-date in 'base_dir'.  (Usually, all files will be
        # out-of-date, because by default we blow away 'base_dir' when
        # we're done making the distribution archives.)
    
        try:
            link = os.link
            msg = "making hard links in %s..." % base_dir
        except AttributeError:
            link = 0
            msg = "copying files to %s..." % base_dir

        self.announce (msg)
        for file in files:
            dest = os.path.join (base_dir, file)
            if link:
                if not os.path.exists (dest):
                    self.execute (os.link, (file, dest),
                                  "linking %s -> %s" % (file, dest))
            else:
                self.copy_file (file, dest)

    # make_release_tree ()


    def nuke_release_tree (self, base_dir):
        try:
            self.execute (rmtree, (base_dir,),
                          "removing %s" % base_dir)
        except (IOError, OSError), exc:
            if exc.filename:
                msg = "error removing %s: %s (%s)" % \
                       (base_dir, exc.strerror, exc.filename)
            else:
                msg = "error removing %s: %s" % (base_dir, exc.strerror)
            self.warn (msg)


    def make_tarball (self, base_dir, compress="gzip"):

        # XXX GNU tar 1.13 has a nifty option to add a prefix directory.
        # It's pretty new, though, so we certainly can't require it --
        # but it would be nice to take advantage of it to skip the
        # "create a tree of hardlinks" step!  (Would also be nice to
        # detect GNU tar to use its 'z' option and save a step.)

        if compress is not None and compress not in ('gzip', 'compress'):
            raise ValueError, \
                  "if given, 'compress' must be 'gzip' or 'compress'"

        archive_name = base_dir + ".tar"
        self.spawn (["tar", "-cf", archive_name, base_dir])

        if compress:
            self.spawn ([compress, archive_name])


    def make_zipfile (self, base_dir):

        # This initially assumed the Unix 'zip' utility -- but
        # apparently InfoZIP's zip.exe works the same under Windows, so
        # no changes needed!

        try:
            self.spawn (["zip", "-r", base_dir + ".zip", base_dir])
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
                      ("unable to create zip file '%s.zip': " + 
                       "could neither find a standalone zip utility nor " +
                       "import the 'zipfile' module") % base_dir

            z = zipfile.ZipFile (base_dir + ".zip", "wb",
                                 compression=zipfile.ZIP_DEFLATED)
            
            def visit (z, dirname, names):
                for name in names:
                    path = os.path.join (dirname, name)
                    if os.path.isfile (path):
                        z.write (path, path)
                        
            os.path.walk (base_dir, visit, z)
            z.close()


    def make_distribution (self):

        # Don't warn about missing meta-data here -- should be done
        # elsewhere.
        name = self.distribution.name or "UNKNOWN"
        version = self.distribution.version

        if version:
            base_dir = "%s-%s" % (name, version)
        else:
            base_dir = name

        # Remove any files that match "base_dir" from the fileset -- we
        # don't want to go distributing the distribution inside itself!
        self.exclude_files (base_dir + "*")
 
        self.make_release_tree (base_dir, self.files)
        for fmt in self.formats:
            if fmt == 'gztar':
                self.make_tarball (base_dir, compress='gzip')
            elif fmt == 'ztar':
                self.make_tarball (base_dir, compress='compress')
            elif fmt == 'tar':
                self.make_tarball (base_dir, compress=None)
            elif fmt == 'zip':
                self.make_zipfile (base_dir)

        if not self.keep_tree:
            self.nuke_release_tree (base_dir)

# class Dist


# ----------------------------------------------------------------------
# Utility functions

def findall (dir = os.curdir):
    """Find all files under 'dir' and return the sorted list of full
       filenames (relative to 'dir')."""

    list = []
    stack = [dir]
    pop = stack.pop
    push = stack.append

    while stack:
        dir = pop()
        names = os.listdir (dir)

        for name in names:
            fullname = os.path.join (dir, name)
            list.append (fullname)
            if os.path.isdir (fullname) and not os.path.islink(fullname):
                push (fullname)

    list.sort()
    return list
