"""distutils.command.sdist

Implements the Distutils 'sdist' command (create a source distribution)."""

# created 1999/09/22, Greg Ward

__revision__ = "$Id$"

import sys, os, string, re
import fnmatch
from types import *
from glob import glob
from distutils.core import Command
from distutils.util import \
     convert_path, create_tree, remove_tree, newer, write_file, \
     check_archive_formats, ARCHIVE_FORMATS
from distutils.text_file import TextFile
from distutils.errors import DistutilsExecError, DistutilsOptionError


class sdist (Command):

    description = "create a source distribution (tarball, zip file, etc.)"

    user_options = [
        ('template=', 't',
         "name of manifest template file [default: MANIFEST.in]"),
        ('manifest=', 'm',
         "name of manifest file [default: MANIFEST]"),
        ('use-defaults', None,
         "include the default file set in the manifest "
         "[default; disable with --no-defaults]"),
        ('manifest-only', 'o',
         "just regenerate the manifest and then stop"),
        ('force-manifest', 'f',
         "forcibly regenerate the manifest and carry on as usual"),
        ('formats=', None,
         "formats for source distribution"),
        ('keep-tree', 'k',
         "keep the distribution tree around after creating " +
         "archive file(s)"),
        ]


    # XXX ugh: this has to precede the 'help_options' list, because
    # it is mentioned there -- also, this is not a method, even though
    # it's defined in a class: double-ugh!
    def show_formats ():
        """Print all possible values for the 'formats' option -- used by
        the "--help-formats" command-line option.
        """
	from distutils.fancy_getopt import FancyGetopt 
	formats=[]
	for format in ARCHIVE_FORMATS.keys():
	    formats.append(("formats="+format,None,ARCHIVE_FORMATS[format][2]))
	formats.sort()
	pretty_printer = FancyGetopt(formats)
	pretty_printer.print_help(
            "List of available source distribution formats:")

    help_options = [
        ('help-formats', None,
         "lists available distribution formats", show_formats),
	]

    negative_opts = {'use-defaults': 'no-defaults'}

    default_format = { 'posix': 'gztar',
                       'nt': 'zip' }

    exclude_re = re.compile (r'\s*!\s*(\S+)') # for manifest lines


    def initialize_options (self):
        # 'template' and 'manifest' are, respectively, the names of
        # the manifest template and manifest file.
        self.template = None
        self.manifest = None

        # 'use_defaults': if true, we will include the default file set
        # in the manifest
        self.use_defaults = 1

        self.manifest_only = 0
        self.force_manifest = 0

        self.formats = None
        self.keep_tree = 0

        self.archive_files = None


    def finalize_options (self):
        if self.manifest is None:
            self.manifest = "MANIFEST"
        if self.template is None:
            self.template = "MANIFEST.in"

        self.ensure_string_list('formats')
        if self.formats is None:
            try:
                self.formats = [self.default_format[os.name]]
            except KeyError:
                raise DistutilsPlatformError, \
                      "don't know how to create source distributions " + \
                      "on platform %s" % os.name

        bad_format = check_archive_formats (self.formats)
        if bad_format:
            raise DistutilsOptionError, \
                  "unknown archive format '%s'" % bad_format


    def run (self):

        # 'files' is the list of files that will make up the manifest
        self.files = []
        
        # Ensure that all required meta-data is given; warn if not (but
        # don't die, it's not *that* serious!)
        self.check_metadata ()

        # Do whatever it takes to get the list of files to process
        # (process the manifest template, read an existing manifest,
        # whatever).  File list is put into 'self.files'.
        self.get_file_list ()

        # If user just wanted us to regenerate the manifest, stop now.
        if self.manifest_only:
            return

        # Otherwise, go ahead and create the source distribution tarball,
        # or zipfile, or whatever.
        self.make_distribution ()


    def check_metadata (self):

        metadata = self.distribution.metadata

        missing = []
        for attr in ('name', 'version', 'url'):
            if not (hasattr (metadata, attr) and getattr (metadata, attr)):
                missing.append (attr)

        if missing:
            self.warn ("missing required meta-data: " +
                       string.join (missing, ", "))

        if metadata.author:
            if not metadata.author_email:
                self.warn ("missing meta-data: if 'author' supplied, " +
                           "'author_email' must be supplied too")
        elif metadata.maintainer:
            if not metadata.maintainer_email:
                self.warn ("missing meta-data: if 'maintainer' supplied, " +
                           "'maintainer_email' must be supplied too")
        else:
            self.warn ("missing meta-data: either (author and author_email) " +
                       "or (maintainer and maintainer_email) " +
                       "must be supplied")

    # check_metadata ()


    def get_file_list (self):
        """Figure out the list of files to include in the source
           distribution, and put it in 'self.files'.  This might
           involve reading the manifest template (and writing the
           manifest), or just reading the manifest, or just using
           the default file set -- it all depends on the user's
           options and the state of the filesystem."""


        template_exists = os.path.isfile (self.template)
        if template_exists:
            template_newer = newer (self.template, self.manifest)

        # Regenerate the manifest if necessary (or if explicitly told to)
        if ((template_exists and template_newer) or
            self.force_manifest or
            self.manifest_only):

            if not template_exists:
                self.warn (("manifest template '%s' does not exist " +
                            "(using default file list)") %
                           self.template)

            # Add default file set to 'files'
            if self.use_defaults:
                self.find_defaults ()

            # Read manifest template if it exists
            if template_exists:
                self.read_template ()

            # File list now complete -- sort it so that higher-level files
            # come first
            sortable_files = map (os.path.split, self.files)
            sortable_files.sort ()
            self.files = []
            for sort_tuple in sortable_files:
                self.files.append (apply (os.path.join, sort_tuple))

            # Remove duplicates from the file list
            for i in range (len(self.files)-1, 0, -1):
                if self.files[i] == self.files[i-1]:
                    del self.files[i]

            # And write complete file list (including default file set) to
            # the manifest.
            self.write_manifest ()

        # Don't regenerate the manifest, just read it in.
        else:
            self.read_manifest ()

    # get_file_list ()


    def find_defaults (self):

        standards = [('README', 'README.txt'), 'setup.py']
        for fn in standards:
            if type (fn) is TupleType:
                alts = fn
                got_it = 0
                for fn in alts:
                    if os.path.exists (fn):
                        got_it = 1
                        self.files.append (fn)
                        break

                if not got_it:
                    self.warn ("standard file not found: should have one of " +
                               string.join (alts, ', '))
            else:
                if os.path.exists (fn):
                    self.files.append (fn)
                else:
                    self.warn ("standard file '%s' not found" % fn)

        optional = ['test/test*.py']
        for pattern in optional:
            files = filter (os.path.isfile, glob (pattern))
            if files:
                self.files.extend (files)

        if self.distribution.has_pure_modules():
            build_py = self.get_finalized_command ('build_py')
            self.files.extend (build_py.get_source_files ())

        if self.distribution.has_ext_modules():
            build_ext = self.get_finalized_command ('build_ext')
            self.files.extend (build_ext.get_source_files ())

        if self.distribution.has_c_libraries():
            build_clib = self.get_finalized_command ('build_clib')
            self.files.extend (build_clib.get_source_files ())


    def search_dir (self, dir, pattern=None):
        """Recursively find files under 'dir' matching 'pattern' (a string
           containing a Unix-style glob pattern).  If 'pattern' is None,
           find all files under 'dir'.  Return the list of found
           filenames."""

        allfiles = findall (dir)
        if pattern is None:
            return allfiles

        pattern_re = translate_pattern (pattern)
        files = []
        for file in allfiles:
            if pattern_re.match (os.path.basename (file)):
                files.append (file)

        return files

    # search_dir ()


#     def exclude_pattern (self, pattern):
#         """Remove filenames from 'self.files' that match 'pattern'."""
#         self.debug_print("exclude_pattern: pattern=%s" % pattern)
#         pattern_re = translate_pattern (pattern)
#         for i in range (len (self.files)-1, -1, -1):
#             if pattern_re.match (self.files[i]):
#                 self.debug_print("removing %s" % self.files[i])
#                 del self.files[i]


    def recursive_exclude_pattern (self, dir, pattern=None):
        """Remove filenames from 'self.files' that are under 'dir'
           and whose basenames match 'pattern'."""

        self.debug_print("recursive_exclude_pattern: dir=%s, pattern=%s" %
                         (dir, pattern))
        if pattern is None:
            pattern_re = None
        else:
            pattern_re = translate_pattern (pattern)

        for i in range (len (self.files)-1, -1, -1):
            (cur_dir, cur_base) = os.path.split (self.files[i])
            if (cur_dir == dir and
                (pattern_re is None or pattern_re.match (cur_base))):
                self.debug_print("removing %s" % self.files[i])
                del self.files[i]


    def read_template (self):
        """Read and parse the manifest template file named by
           'self.template' (usually "MANIFEST.in").  Process all file
           specifications (include and exclude) in the manifest template
           and add the resulting filenames to 'self.files'."""

        assert self.files is not None and type (self.files) is ListType
        self.announce("reading manifest template '%s'" % self.template)

        template = TextFile (self.template,
                             strip_comments=1,
                             skip_blanks=1,
                             join_lines=1,
                             lstrip_ws=1,
                             rstrip_ws=1,
                             collapse_ws=1)

        all_files = findall ()

        while 1:

            line = template.readline()
            if line is None:            # end of file
                break

            words = string.split (line)
            action = words[0]

            # First, check that the right number of words are present
            # for the given action (which is the first word)
            if action in ('include','exclude',
                          'global-include','global-exclude'):
                if len (words) < 2:
                    template.warn \
                        ("invalid manifest template line: " +
                         "'%s' expects <pattern1> <pattern2> ..." %
                         action)
                    continue

                pattern_list = map(convert_path, words[1:])

            elif action in ('recursive-include','recursive-exclude'):
                if len (words) < 3:
                    template.warn \
                        ("invalid manifest template line: " +
                         "'%s' expects <dir> <pattern1> <pattern2> ..." %
                         action)
                    continue

                dir = convert_path(words[1])
                pattern_list = map (convert_path, words[2:])

            elif action in ('graft','prune'):
                if len (words) != 2:
                    template.warn \
                        ("invalid manifest template line: " +
                         "'%s' expects a single <dir_pattern>" %
                         action)
                    continue

                dir_pattern = convert_path (words[1])

            else:
                template.warn ("invalid manifest template line: " +
                               "unknown action '%s'" % action)
                continue

            # OK, now we know that the action is valid and we have the
            # right number of words on the line for that action -- so we
            # can proceed with minimal error-checking.  Also, we have
            # defined either (pattern), (dir and pattern), or
            # (dir_pattern) -- so we don't have to spend any time
            # digging stuff up out of 'words'.

            if action == 'include':
                self.debug_print("include " + string.join(pattern_list))
                for pattern in pattern_list:
                    files = self.select_pattern (all_files, pattern, anchor=1)
                    if not files:
                        template.warn ("no files found matching '%s'" %
                                       pattern)
                    else:
                        self.files.extend (files)

            elif action == 'exclude':
                self.debug_print("exclude " + string.join(pattern_list))
                for pattern in pattern_list:
                    num = self.exclude_pattern (self.files, pattern, anchor=1)
                    if num == 0:
                        template.warn (
                            "no previously-included files found matching '%s'"%
                            pattern)

            elif action == 'global-include':
                self.debug_print("global-include " + string.join(pattern_list))
                for pattern in pattern_list:
                    files = self.select_pattern (all_files, pattern, anchor=0)
                    if not files:
                        template.warn (("no files found matching '%s' " +
                                        "anywhere in distribution") %
                                       pattern)
                    else:
                        self.files.extend (files)

            elif action == 'global-exclude':
                self.debug_print("global-exclude " + string.join(pattern_list))
                for pattern in pattern_list:
                    num = self.exclude_pattern (self.files, pattern, anchor=0)
                    if num == 0:
                        template.warn \
                            (("no previously-included files matching '%s' " +
                              "found anywhere in distribution") %
                             pattern)

            elif action == 'recursive-include':
                self.debug_print("recursive-include %s %s" %
                                 (dir, string.join(pattern_list)))
                for pattern in pattern_list:
                    files = self.select_pattern (
                        all_files, pattern, prefix=dir)
                    if not files:
                        template.warn (("no files found matching '%s' " +
                                        "under directory '%s'") %
                                       (pattern, dir))
                    else:
                        self.files.extend (files)

            elif action == 'recursive-exclude':
                self.debug_print("recursive-exclude %s %s" %
                                 (dir, string.join(pattern_list)))
                for pattern in pattern_list:
                    num = self.exclude_pattern(
                        self.files, pattern, prefix=dir)
                    if num == 0:
                        template.warn \
                            (("no previously-included files matching '%s' " +
                              "found under directory '%s'") %
                             (pattern, dir))

            elif action == 'graft':
                self.debug_print("graft " + dir_pattern)
                files = self.select_pattern(
                    all_files, None, prefix=dir_pattern)
                if not files:
                    template.warn ("no directories found matching '%s'" %
                                   dir_pattern)
                else:
                    self.files.extend (files)

            elif action == 'prune':
                self.debug_print("prune " + dir_pattern)
                num = self.exclude_pattern(
                    self.files, None, prefix=dir_pattern)
                if num == 0:
                    template.warn \
                        (("no previously-included directories found " +
                          "matching '%s'") %
                         dir_pattern)
            else:
                raise RuntimeError, \
                      "this cannot happen: invalid action '%s'" % action

        # while loop over lines of template file

        # Prune away the build and source distribution directories
        build = self.get_finalized_command ('build')
        self.exclude_pattern (self.files, None, prefix=build.build_base)

        base_dir = self.distribution.get_fullname()
        self.exclude_pattern (self.files, None, prefix=base_dir)

    # read_template ()


    def select_pattern (self, files, pattern, anchor=1, prefix=None):
        """Select strings (presumably filenames) from 'files' that match
        'pattern', a Unix-style wildcard (glob) pattern.  Patterns are not
        quite the same as implemented by the 'fnmatch' module: '*' and '?'
        match non-special characters, where "special" is platform-dependent:
        slash on Unix, colon, slash, and backslash on DOS/Windows, and colon on
        Mac OS.

        If 'anchor' is true (the default), then the pattern match is more
        stringent: "*.py" will match "foo.py" but not "foo/bar.py".  If
        'anchor' is false, both of these will match.

        If 'prefix' is supplied, then only filenames starting with 'prefix'
        (itself a pattern) and ending with 'pattern', with anything in between
        them, will match.  'anchor' is ignored in this case.

        Return the list of matching strings, possibly empty.
        """
        matches = []
        pattern_re = translate_pattern (pattern, anchor, prefix)
        self.debug_print("select_pattern: applying regex r'%s'" %
                         pattern_re.pattern)
        for name in files:
            if pattern_re.search (name):
                matches.append (name)
                self.debug_print(" adding " + name)

        return matches

    # select_pattern ()


    def exclude_pattern (self, files, pattern, anchor=1, prefix=None):
        """Remove strings (presumably filenames) from 'files' that match
        'pattern'.  'pattern', 'anchor', 'and 'prefix' are the same
        as for 'select_pattern()', above.  The list 'files' is modified
        in place.
        """
        pattern_re = translate_pattern (pattern, anchor, prefix)
        self.debug_print("exclude_pattern: applying regex r'%s'" %
                         pattern_re.pattern)
        for i in range (len(files)-1, -1, -1):
            if pattern_re.search (files[i]):
                self.debug_print(" removing " + files[i])
                del files[i]

    # exclude_pattern ()


    def write_manifest (self):
        """Write the file list in 'self.files' (presumably as filled in
           by 'find_defaults()' and 'read_template()') to the manifest file
           named by 'self.manifest'."""

        self.execute(write_file,
                     (self.manifest, self.files),
                     "writing manifest file '%s'" % self.manifest)

    # write_manifest ()


    def read_manifest (self):
        """Read the manifest file (named by 'self.manifest') and use
           it to fill in 'self.files', the list of files to include
           in the source distribution."""

        self.announce("reading manifest file '%s'" % self.manifest)
        manifest = open (self.manifest)
        while 1:
            line = manifest.readline ()
            if line == '':              # end of file
                break
            if line[-1] == '\n':
                line = line[0:-1]
            self.files.append (line)

    # read_manifest ()
            

    def make_release_tree (self, base_dir, files):

        # Create all the directories under 'base_dir' necessary to
        # put 'files' there.
        create_tree (base_dir, files,
                     verbose=self.verbose, dry_run=self.dry_run)

        # And walk over the list of files, either making a hard link (if
        # os.link exists) to each one that doesn't already exist in its
        # corresponding location under 'base_dir', or copying each file
        # that's out-of-date in 'base_dir'.  (Usually, all files will be
        # out-of-date, because by default we blow away 'base_dir' when
        # we're done making the distribution archives.)
    
        if hasattr (os, 'link'):        # can make hard links on this system
            link = 'hard'
            msg = "making hard links in %s..." % base_dir
        else:                           # nope, have to copy
            link = None
            msg = "copying files to %s..." % base_dir

        self.announce (msg)
        for file in files:
            dest = os.path.join (base_dir, file)
            self.copy_file (file, dest, link=link)

    # make_release_tree ()


    def make_distribution (self):

        # Don't warn about missing meta-data here -- should be (and is!)
        # done elsewhere.
        base_dir = self.distribution.get_fullname()

        # Remove any files that match "base_dir" from the fileset -- we
        # don't want to go distributing the distribution inside itself!
        self.exclude_pattern (self.files, base_dir + "*")
 
        self.make_release_tree (base_dir, self.files)
        archive_files = []              # remember names of files we create
        for fmt in self.formats:
            file = self.make_archive (base_dir, fmt, base_dir=base_dir)
            archive_files.append(file)

        self.archive_files = archive_files

        if not self.keep_tree:
            remove_tree (base_dir, self.verbose, self.dry_run)

    def get_archive_files (self):
        """Return the list of archive files created when the command
        was run, or None if the command hasn't run yet.
        """
        return self.archive_files

# class sdist


# ----------------------------------------------------------------------
# Utility functions

def findall (dir = os.curdir):
    """Find all files under 'dir' and return the list of full
       filenames (relative to 'dir')."""

    list = []
    stack = [dir]
    pop = stack.pop
    push = stack.append

    while stack:
        dir = pop()
        names = os.listdir (dir)

        for name in names:
            if dir != os.curdir:        # avoid the dreaded "./" syndrome
                fullname = os.path.join (dir, name)
            else:
                fullname = name
            list.append (fullname)
            if os.path.isdir (fullname) and not os.path.islink(fullname):
                push (fullname)

    return list


def glob_to_re (pattern):
    """Translate a shell-like glob pattern to a regular expression;
       return a string containing the regex.  Differs from
       'fnmatch.translate()' in that '*' does not match "special
       characters" (which are platform-specific)."""
    pattern_re = fnmatch.translate (pattern)

    # '?' and '*' in the glob pattern become '.' and '.*' in the RE, which
    # IMHO is wrong -- '?' and '*' aren't supposed to match slash in Unix,
    # and by extension they shouldn't match such "special characters" under
    # any OS.  So change all non-escaped dots in the RE to match any
    # character except the special characters.
    # XXX currently the "special characters" are just slash -- i.e. this is
    # Unix-only.
    pattern_re = re.sub (r'(^|[^\\])\.', r'\1[^/]', pattern_re)
    return pattern_re

# glob_to_re ()


def translate_pattern (pattern, anchor=1, prefix=None):
    """Translate a shell-like wildcard pattern to a compiled regular
       expression.    Return the compiled regex."""

    if pattern:
        pattern_re = glob_to_re (pattern)
    else:
        pattern_re = ''
        
    if prefix is not None:
        prefix_re = (glob_to_re (prefix))[0:-1] # ditch trailing $
        pattern_re = "^" + os.path.join (prefix_re, ".*" + pattern_re)
    else:                               # no prefix -- respect anchor flag
        if anchor:
            pattern_re = "^" + pattern_re
        
    return re.compile (pattern_re)

# translate_pattern ()
