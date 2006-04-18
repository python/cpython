"""setuptools.command.egg_info

Create a distribution's .egg-info directory and contents"""

# This module should be kept compatible with Python 2.3
import os, re
from setuptools import Command
from distutils.errors import *
from distutils import log
from setuptools.command.sdist import sdist
from distutils import file_util
from distutils.util import convert_path
from distutils.filelist import FileList
from pkg_resources import parse_requirements, safe_name, parse_version, \
    safe_version, yield_lines, EntryPoint, iter_entry_points, to_filename
from sdist import walk_revctrl

class egg_info(Command):
    description = "create a distribution's .egg-info directory"

    user_options = [
        ('egg-base=', 'e', "directory containing .egg-info directories"
                           " (default: top of the source tree)"),
        ('tag-svn-revision', 'r',
            "Add subversion revision ID to version number"),
        ('tag-date', 'd', "Add date stamp (e.g. 20050528) to version number"),
        ('tag-build=', 'b', "Specify explicit tag to add to version number"),
    ]

    boolean_options = ['tag-date','tag-svn-revision']

    def initialize_options (self):
        self.egg_name = None
        self.egg_version = None
        self.egg_base = None
        self.egg_info = None
        self.tag_build = None
        self.tag_svn_revision = 0
        self.tag_date = 0
        self.broken_egg_info = False

    def finalize_options (self):
        self.egg_name = safe_name(self.distribution.get_name())
        self.egg_version = self.tagged_version()

        try:
            list(
                parse_requirements('%s==%s' % (self.egg_name,self.egg_version))
            )
        except ValueError:
            raise DistutilsOptionError(
                "Invalid distribution name or version syntax: %s-%s" %
                (self.egg_name,self.egg_version)
            )

        if self.egg_base is None:
            dirs = self.distribution.package_dir
            self.egg_base = (dirs or {}).get('',os.curdir)

        self.ensure_dirname('egg_base')
        self.egg_info = to_filename(self.egg_name)+'.egg-info'
        if self.egg_base != os.curdir:
            self.egg_info = os.path.join(self.egg_base, self.egg_info)
        if '-' in self.egg_name: self.check_broken_egg_info()

        # Set package version for the benefit of dumber commands
        # (e.g. sdist, bdist_wininst, etc.)
        #
        self.distribution.metadata.version = self.egg_version

        # If we bootstrapped around the lack of a PKG-INFO, as might be the
        # case in a fresh checkout, make sure that any special tags get added
        # to the version info
        #
        pd = self.distribution._patched_dist
        if pd is not None and pd.key==self.egg_name.lower():
            pd._version = self.egg_version
            pd._parsed_version = parse_version(self.egg_version)
            self.distribution._patched_dist = None



    def write_or_delete_file(self, what, filename, data, force=False):
        """Write `data` to `filename` or delete if empty

        If `data` is non-empty, this routine is the same as ``write_file()``.
        If `data` is empty but not ``None``, this is the same as calling
        ``delete_file(filename)`.  If `data` is ``None``, then this is a no-op
        unless `filename` exists, in which case a warning is issued about the
        orphaned file (if `force` is false), or deleted (if `force` is true).
        """
        if data:
            self.write_file(what, filename, data)
        elif os.path.exists(filename):
            if data is None and not force:
                log.warn(
                    "%s not set in setup(), but %s exists", what, filename
                )
                return
            else:
                self.delete_file(filename)

    def write_file(self, what, filename, data):
        """Write `data` to `filename` (if not a dry run) after announcing it

        `what` is used in a log message to identify what is being written
        to the file.
        """
        log.info("writing %s to %s", what, filename)
        if not self.dry_run:
            f = open(filename, 'wb')
            f.write(data)
            f.close()

    def delete_file(self, filename):
        """Delete `filename` (if not a dry run) after announcing it"""
        log.info("deleting %s", filename)
        if not self.dry_run:
            os.unlink(filename)




    def run(self):
        self.mkpath(self.egg_info)
        installer = self.distribution.fetch_build_egg
        for ep in iter_entry_points('egg_info.writers'):
            writer = ep.load(installer=installer)
            writer(self, ep.name, os.path.join(self.egg_info,ep.name))
        self.find_sources()

    def tagged_version(self):
        version = self.distribution.get_version()
        if self.tag_build:
            version+=self.tag_build
        if self.tag_svn_revision and (
            os.path.exists('.svn') or os.path.exists('PKG-INFO')
        ):  version += '-r%s' % self.get_svn_revision()
        if self.tag_date:
            import time; version += time.strftime("-%Y%m%d")
        return safe_version(version)

    def get_svn_revision(self):
        revision = 0
        urlre = re.compile('url="([^"]+)"')
        revre = re.compile('committed-rev="(\d+)"')
        for base,dirs,files in os.walk(os.curdir):
            if '.svn' not in dirs:
                dirs[:] = []
                continue    # no sense walking uncontrolled subdirs
            dirs.remove('.svn')
            f = open(os.path.join(base,'.svn','entries'))
            data = f.read()
            f.close()
            dirurl = urlre.search(data).group(1)    # get repository URL
            if base==os.curdir:
                base_url = dirurl+'/'   # save the root url
            elif not dirurl.startswith(base_url):
                dirs[:] = []
                continue    # not part of the same svn tree, skip it
            for match in revre.finditer(data):
                revision = max(revision, int(match.group(1)))
        return str(revision or get_pkg_info_revision())

    def find_sources(self):
        """Generate SOURCES.txt manifest file"""
        manifest_filename = os.path.join(self.egg_info,"SOURCES.txt")
        mm = manifest_maker(self.distribution)
        mm.manifest = manifest_filename
        mm.run()
        self.filelist = mm.filelist

    def check_broken_egg_info(self):
        bei = self.egg_name+'.egg-info'
        if self.egg_base != os.curdir:
            bei = os.path.join(self.egg_base, bei)
        if os.path.exists(bei):
            log.warn(
                "-"*78+'\n'
                "Note: Your current .egg-info directory has a '-' in its name;"
                '\nthis will not work correctly with "setup.py develop".\n\n'
                'Please rename %s to %s to correct this problem.\n'+'-'*78,
                bei, self.egg_info
            )
            self.broken_egg_info = self.egg_info
            self.egg_info = bei     # make it work for now

class FileList(FileList):
    """File list that accepts only existing, platform-independent paths"""

    def append(self, item):
        path = convert_path(item)
        if os.path.exists(path):
            self.files.append(path)











class manifest_maker(sdist):

    template = "MANIFEST.in"

    def initialize_options (self):
        self.use_defaults = 1
        self.prune = 1
        self.manifest_only = 1
        self.force_manifest = 1

    def finalize_options(self):
        pass

    def run(self):
        self.filelist = FileList()
        if not os.path.exists(self.manifest):
            self.write_manifest()   # it must exist so it'll get in the list
        self.filelist.findall()
        self.add_defaults()
        if os.path.exists(self.template):
            self.read_template()
        self.prune_file_list()
        self.filelist.sort()
        self.filelist.remove_duplicates()
        self.write_manifest()

    def write_manifest (self):
        """Write the file list in 'self.filelist' (presumably as filled in
        by 'add_defaults()' and 'read_template()') to the manifest file
        named by 'self.manifest'.
        """
        files = self.filelist.files
        if os.sep!='/':
            files = [f.replace(os.sep,'/') for f in files]
        self.execute(file_util.write_file, (self.manifest, files),
                     "writing manifest file '%s'" % self.manifest)





    def add_defaults(self):
        sdist.add_defaults(self)
        self.filelist.append(self.template)
        self.filelist.append(self.manifest)
        rcfiles = list(walk_revctrl())
        if rcfiles:
            self.filelist.extend(rcfiles)
        elif os.path.exists(self.manifest):
            self.read_manifest()
        ei_cmd = self.get_finalized_command('egg_info')
        self.filelist.include_pattern("*", prefix=ei_cmd.egg_info)

    def prune_file_list (self):
        build = self.get_finalized_command('build')
        base_dir = self.distribution.get_fullname()
        self.filelist.exclude_pattern(None, prefix=build.build_base)
        self.filelist.exclude_pattern(None, prefix=base_dir)
        sep = re.escape(os.sep)
        self.filelist.exclude_pattern(sep+r'(RCS|CVS|\.svn)'+sep, is_regex=1)






















def write_pkg_info(cmd, basename, filename):
    log.info("writing %s", filename)
    if not cmd.dry_run:
        metadata = cmd.distribution.metadata
        metadata.version, oldver = cmd.egg_version, metadata.version
        metadata.name, oldname   = cmd.egg_name, metadata.name
        try:
            # write unescaped data to PKG-INFO, so older pkg_resources
            # can still parse it
            metadata.write_pkg_info(cmd.egg_info)
        finally:
            metadata.name, metadata.version = oldname, oldver

        safe = getattr(cmd.distribution,'zip_safe',None)
        import bdist_egg; bdist_egg.write_safety_flag(cmd.egg_info, safe)

def warn_depends_obsolete(cmd, basename, filename):
    if os.path.exists(filename):
        log.warn(
            "WARNING: 'depends.txt' is not used by setuptools 0.6!\n"
            "Use the install_requires/extras_require setup() args instead."
        )


def write_requirements(cmd, basename, filename):
    dist = cmd.distribution
    data = ['\n'.join(yield_lines(dist.install_requires or ()))]
    for extra,reqs in (dist.extras_require or {}).items():
        data.append('\n\n[%s]\n%s' % (extra, '\n'.join(yield_lines(reqs))))
    cmd.write_or_delete_file("requirements", filename, ''.join(data))

def write_toplevel_names(cmd, basename, filename):
    pkgs = dict.fromkeys(
        [k.split('.',1)[0]
            for k in cmd.distribution.iter_distribution_names()
        ]
    )
    cmd.write_file("top-level names", filename, '\n'.join(pkgs)+'\n')



def overwrite_arg(cmd, basename, filename):
    write_arg(cmd, basename, filename, True)

def write_arg(cmd, basename, filename, force=False):
    argname = os.path.splitext(basename)[0]
    value = getattr(cmd.distribution, argname, None)
    if value is not None:
        value = '\n'.join(value)+'\n'
    cmd.write_or_delete_file(argname, filename, value, force)

def write_entries(cmd, basename, filename):
    ep = cmd.distribution.entry_points

    if isinstance(ep,basestring) or ep is None:
        data = ep
    elif ep is not None:
        data = []
        for section, contents in ep.items():
            if not isinstance(contents,basestring):
                contents = EntryPoint.parse_group(section, contents)
                contents = '\n'.join(map(str,contents.values()))
            data.append('[%s]\n%s\n\n' % (section,contents))
        data = ''.join(data)

    cmd.write_or_delete_file('entry points', filename, data, True)

def get_pkg_info_revision():
    # See if we can get a -r### off of PKG-INFO, in case this is an sdist of
    # a subversion revision
    #
    if os.path.exists('PKG-INFO'):
        f = open('PKG-INFO','rU')
        for line in f:
            match = re.match(r"Version:.*-r(\d+)\s*$", line)
            if match:
                return int(match.group(1))
    return 0




