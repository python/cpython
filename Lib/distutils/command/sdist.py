"""distutils.command.sdist

Implements the Distutils 'sdist' command (create a source distribution)."""

# created 1999/09/22, Greg Ward

__revision__ = "$Id$"

import sys, os, string
from types import *
from glob import glob
from distutils.core import Command
from distutils.util import \
     create_tree, remove_tree, newer, write_file, \
     check_archive_formats
from distutils.text_file import TextFile
from distutils.errors import DistutilsExecError, DistutilsOptionError
from distutils.filelist import FileList


def show_formats ():
    """Print all possible values for the 'formats' option (used by
    the "--help-formats" command-line option).
    """
    from distutils.fancy_getopt import FancyGetopt
    from distutils.archive_util import ARCHIVE_FORMATS
    formats=[]
    for format in ARCHIVE_FORMATS.keys():
        formats.append(("formats=" + format, None,
                        ARCHIVE_FORMATS[format][2]))
    formats.sort()
    pretty_printer = FancyGetopt(formats)
    pretty_printer.print_help(
        "List of available source distribution formats:")


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
        ('no-defaults', None,
         "don't include the default file set"),
        ('prune', None,
         "specifically exclude files/directories that should not be "
         "distributed (build tree, RCS/CVS dirs, etc.) "
         "[default; disable with --no-prune]"),
        ('no-prune', None,
         "don't automatically exclude anything"),
        ('manifest-only', 'o',
         "just regenerate the manifest and then stop "
         "(implies --force-manifest)"),
        ('force-manifest', 'f',
         "forcibly regenerate the manifest and carry on as usual"),
        ('formats=', None,
         "formats for source distribution (comma-separated list)"),
        ('keep-tree', 'k',
         "keep the distribution tree around after creating " +
         "archive file(s)"),
        ('dist-dir=', 'd',
         "directory to put the source distribution archive(s) in "
         "[default: dist]"),
        ]


    help_options = [
        ('help-formats', None,
         "list available distribution formats", show_formats),
	]

    negative_opt = {'no-defaults': 'use-defaults',
                    'no-prune': 'prune' }

    default_format = { 'posix': 'gztar',
                       'nt': 'zip' }

    def initialize_options (self):
        # 'template' and 'manifest' are, respectively, the names of
        # the manifest template and manifest file.
        self.template = None
        self.manifest = None

        # 'use_defaults': if true, we will include the default file set
        # in the manifest
        self.use_defaults = 1
        self.prune = 1

        self.manifest_only = 0
        self.force_manifest = 0

        self.formats = None
        self.keep_tree = 0
        self.dist_dir = None

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

        if self.dist_dir is None:
            self.dist_dir = "dist"


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
        """Ensure that all required elements of meta-data (name, version,
        URL, (author and author_email) or (maintainer and
        maintainer_email)) are supplied by the Distribution object; warn if
        any are missing.
        """
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
        distribution, and put it in 'self.files'.  This might involve
        reading the manifest template (and writing the manifest), or just
        reading the manifest, or just using the default file set -- it all
        depends on the user's options and the state of the filesystem.
        """

        # If we have a manifest template, see if it's newer than the
        # manifest; if so, we'll regenerate the manifest.
        template_exists = os.path.isfile (self.template)
        if template_exists:
            template_newer = newer (self.template, self.manifest)

        # The contents of the manifest file almost certainly depend on the
        # setup script as well as the manifest template -- so if the setup
        # script is newer than the manifest, we'll regenerate the manifest
        # from the template.  (Well, not quite: if we already have a
        # manifest, but there's no template -- which will happen if the
        # developer elects to generate a manifest some other way -- then we
        # can't regenerate the manifest, so we don't.)
        setup_newer = newer(sys.argv[0], self.manifest)

        # cases:
        #   1) no manifest, template exists: generate manifest
        #      (covered by 2a: no manifest == template newer)
        #   2) manifest & template exist:
        #      2a) template or setup script newer than manifest:
        #          regenerate manifest
        #      2b) manifest newer than both:
        #          do nothing (unless --force or --manifest-only)
        #   3) manifest exists, no template:
        #      do nothing (unless --force or --manifest-only)
        #   4) no manifest, no template: generate w/ warning ("defaults only")

        # Regenerate the manifest if necessary (or if explicitly told to)
        if ((template_exists and (template_newer or setup_newer)) or
            self.force_manifest or self.manifest_only):

            if not template_exists:
                self.warn (("manifest template '%s' does not exist " +
                            "(using default file list)") %
                           self.template)

            # Add default file set to 'files'
            if self.use_defaults:
                self.add_defaults ()

            # Read manifest template if it exists
            if template_exists:
                self.read_template ()

            # Prune away any directories that don't belong in the source
            # distribution
            if self.prune:
                self.prune_file_list()

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


    def add_defaults (self):
        """Add all the default files to self.files:
          - README or README.txt
          - setup.py
          - test/test*.py
          - all pure Python modules mentioned in setup script
          - all C sources listed as part of extensions or C libraries
            in the setup script (doesn't catch C headers!)
        Warns if (README or README.txt) or setup.py are missing; everything
        else is optional.
        """

        # XXX name of setup script and config file should be taken
        # programmatically from the Distribution object (except
        # it doesn't have that capability... yet!)
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

        optional = ['test/test*.py', 'setup.cfg']
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

    # add_defaults ()
    

    def read_template (self):
        """Read and parse the manifest template file named by
        'self.template' (usually "MANIFEST.in").  Process all file
        specifications (include and exclude) in the manifest template and
        update 'self.files' accordingly (filenames may be added to
        or removed from 'self.files' based on the manifest template).
        """
        assert self.files is not None and type (self.files) is ListType
        self.announce("reading manifest template '%s'" % self.template)

        template = TextFile (self.template,
                             strip_comments=1,
                             skip_blanks=1,
                             join_lines=1,
                             lstrip_ws=1,
                             rstrip_ws=1,
                             collapse_ws=1)

        # if we give Template() a list, it modifies this list
        filelist = FileList(files=self.files,
                            warn=self.warn,
                            debug_print=self.debug_print)

        while 1:
            line = template.readline()
            if line is None:            # end of file
                break

            filelist.process_template_line(line)

    # read_template ()


    def prune_file_list (self):
        """Prune off branches that might slip into the file list as created
        by 'read_template()', but really don't belong there:
          * the build tree (typically "build")
          * the release tree itself (only an issue if we ran "sdist"
            previously with --keep-tree, or it aborted)
          * any RCS or CVS directories
        """
        build = self.get_finalized_command('build')
        base_dir = self.distribution.get_fullname()

        # if we give FileList a list, it modifies this list
        filelist = FileList(files=self.files,
                            warn=self.warn,
                            debug_print=self.debug_print)
        filelist.exclude_pattern(None, prefix=build.build_base)
        filelist.exclude_pattern(None, prefix=base_dir)
        filelist.exclude_pattern(r'/(RCS|CVS)/.*', is_regex=1)


    def write_manifest (self):
        """Write the file list in 'self.files' (presumably as filled in by
        'add_defaults()' and 'read_template()') to the manifest file named
        by 'self.manifest'.
        """
        self.execute(write_file,
                     (self.manifest, self.files),
                     "writing manifest file '%s'" % self.manifest)

    # write_manifest ()


    def read_manifest (self):
        """Read the manifest file (named by 'self.manifest') and use it to
        fill in 'self.files', the list of files to include in the source
        distribution.
        """
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
        """Create the directory tree that will become the source
        distribution archive.  All directories implied by the filenames in
        'files' are created under 'base_dir', and then we hard link or copy
        (if hard linking is unavailable) those files into place.
        Essentially, this duplicates the developer's source tree, but in a
        directory named after the distribution, containing only the files
        to be distributed.
        """
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
        """Create the source distribution(s).  First, we create the release
        tree with 'make_release_tree()'; then, we create all required
        archive files (according to 'self.formats') from the release tree.
        Finally, we clean up by blowing away the release tree (unless
        'self.keep_tree' is true).  The list of archive files created is
        stored so it can be retrieved later by 'get_archive_files()'.
        """
        # Don't warn about missing meta-data here -- should be (and is!)
        # done elsewhere.
        base_dir = self.distribution.get_fullname()
        base_name = os.path.join(self.dist_dir, base_dir)

        self.make_release_tree (base_dir, self.files)
        archive_files = []              # remember names of files we create
        if self.dist_dir:
            self.mkpath(self.dist_dir)
        for fmt in self.formats:
            file = self.make_archive (base_name, fmt, base_dir=base_dir)
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
