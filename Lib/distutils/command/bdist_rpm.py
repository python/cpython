"""distutils.command.bdist_rpm

Implements the Distutils 'bdist_rpm' command (create RPM source and binary
distributions)."""

# created 2000/04/25, by Harry Henry Gebel

__revision__ = "$Id$"

import os, string, re
from types import *
from distutils.core import Command
from distutils.util import get_platform, write_file
from distutils.errors import *

class bdist_rpm (Command):

    description = "create an RPM distribution"

    user_options = [
        ('bdist-base', None,
         "base directory for creating built distributions"),
        ('spec-only', None,
         "only regenerate spec file"),
        ('source-only', None,
         "only generate source RPM"),
        ('binary-only', None,
         "only generate binary RPM"),
        ('use-bzip2', None,
         "use bzip2 instead of gzip to create source distribution"),

        # More meta-data: too RPM-specific to put in the setup script,
        # but needs to go in the .spec file -- so we make these options
        # to "bdist_rpm".  The idea is that packagers would put this
        # info in setup.cfg, although they are of course free to
        # supply it on the command line.
        ('distribution-name', None,
         "name of the (Linux) distribution name to which this "
         "RPM applies (*not* the name of the module distribution!)"),
        ('group', None,
         "package classification [default: \"Development/Libraries\"]"),
        ('release', None,
         "RPM release number"),
        ('serial', None,
         "???"),
        ('vendor', None,
         "RPM \"vendor\" (eg. \"Joe Blow <joe@example.com>\") "
         "[default: maintainer or author from setup script]"),
        ('packager', None,
         "RPM packager (eg. \"Jane Doe <jane@example.net>\")"
         "[default: vendor]"),
        ('doc-files', None,
         "list of documentation files (space or comma-separated)"),
        ('changelog', None,
         "RPM changelog"),
        ('icon', None,
         "name of icon file"),

        ('prep-cmd', None,
         "?? pre-build command(s) ??"),
        ('build-cmd', None,
         "?? build command(s) ??"),
        ('install-cmd', None,
         "?? installation command(s) ??"),
        ('clean-cmd', None,
         "?? clean command(s) ??"),
        ('pre-install', None,
         "pre-install script (Bourne shell code)"),
        ('post-install', None,
         "post-install script (Bourne shell code)"),
        ('pre-uninstall', None,
         "pre-uninstall script (Bourne shell code)"),
        ('post-uninstall', None,
         "post-uninstall script (Bourne shell code)"),

        ('provides', None,
         "???"),
        ('requires', None,
         "???"),
        ('conflicts', None,
         "???"),
        ('build-requires', None,
         "???"),
        ('obsoletes', None,
         "???"),

        # Actions to take when building RPM
        ('clean', None,
         "clean up RPM build directory [default]"),
        ('no-clean', None,
         "don't clean up RPM build directory"),
        ('use-rpm-opt-flags', None,
         "compile with RPM_OPT_FLAGS when building from source RPM"),
        ('no-rpm-opt-flags', None,
         "do not pass any RPM CFLAGS to compiler"),
       ]

    negative_opt = {'no-clean': 'clean',
                    'no-rpm-opt-flags': 'use-rpm-opt-flags'}

                    
    def initialize_options (self):
        self.bdist_base = None
        self.spec_only = None
        self.binary_only = None
        self.source_only = None
        self.use_bzip2 = None

        self.distribution_name = None
        self.group = None
        self.release = None
        self.serial = None
        self.vendor = None
        self.packager = None
        self.doc_files = None
        self.changelog = None
        self.icon = None

        self.prep_cmd = None
        self.build_cmd = None
        self.install_cmd = None
        self.clean_cmd = None
        self.pre_install = None
        self.post_install = None
        self.pre_uninstall = None
        self.post_uninstall = None
        self.prep = None
        self.provides = None
        self.requires = None
        self.conflicts = None
        self.build_requires = None
        self.obsoletes = None

        self.clean = 1
        self.use_rpm_opt_flags = 1

    # initialize_options()


    def finalize_options (self):
        self.set_undefined_options('bdist', ('bdist_base', 'bdist_base'))
        if os.name != 'posix':
            raise DistutilsPlatformError, \
                  ("don't know how to create RPM "
                   "distributions on platform %s" % os.name)
        if self.binary_only and self.source_only:
            raise DistutilsOptionsError, \
                  "cannot supply both '--source-only' and '--binary-only'"

        # don't pass CFLAGS to pure python distributions
        if not self.distribution.has_ext_modules():
            self.use_rpm_opt_flags = 0

        self.finalize_package_data()

    # finalize_options()

    def finalize_package_data (self):
        self.ensure_string('group', "Development/Libraries")
        self.ensure_string('vendor',
                           "%s <%s>" % (self.distribution.get_contact(),
                                        self.distribution.get_contact_email()))
        self.ensure_string('packager', self.vendor) # or nothing?
        self.ensure_string_list('doc_files')
        if type(self.doc_files) is ListType:
            for readme in ('README', 'README.txt'):
                if os.path.exists(readme) and readme not in self.doc_files:
                    self.doc.append(readme)

        self.ensure_string('release', "1")   # should it be an int?
        self.ensure_string('serial')   # should it be an int?

        self.ensure_string('icon')
        self.ensure_string('distribution_name')

        self.ensure_string('prep_cmd', "%setup") # string or filename?

        if self.use_rpm_opt_flags:
            def_build = 'env CFLAGS="$RPM_OPT_FLAGS" python setup.py build'
        else:
            def_build = 'python setup.py build'
        self.ensure_string('build_cmd', def_build)
        self.ensure_string('install_cmd',
                           "python setup.py install --root=$RPM_BUILD_ROOT "
                           "--record=INSTALLED_FILES")
        self.ensure_string('clean_cmd',
                           "rm -rf $RPM_BUILD_ROOT")
        self.ensure_filename('pre_install')
        self.ensure_filename('post_install')
        self.ensure_filename('pre_uninstall')
        self.ensure_filename('post_uninstall')

        # XXX don't forget we punted on summaries and descriptions -- they
        # should be handled here eventually!

        # Now *this* is some meta-data that belongs in the setup script...
        self.ensure_string_list('provides')
        self.ensure_string_list('requires')
        self.ensure_string_list('conflicts')
        self.ensure_string_list('build_requires')
        self.ensure_string_list('obsoletes')

    # finalize_package_data ()


    # XXX these look awfully handy: should probably move them
    # up to Command and use more widely.
    def _ensure_stringlike (self, option, what, default=None):
        val = getattr(self, option)
        if val is None:
            setattr(self, option, default)
            return default
        elif type(val) is not StringType:
            raise DistutilsOptionError, \
                  "'%s' must be a %s (got `%s`)" % (option, what, val)
        return val

    def ensure_string (self, option, default=None):
        self._ensure_stringlike(option, "string", default)

    def ensure_string_list (self, option):
        val = getattr(self, option)
        if val is None:
            return
        elif type(val) is StringType:
            setattr(self, option, re.split(r',\s*|\s+', val))
        else:
            if type(val) is ListType:
                types = map(type, val)
                ok = (types == [StringType] * len(val))
            else:
                ok = 0

            if not ok:
                raise DistutilsOptionError, \
                      "'%s' must be a list of strings (got %s)" % \
                      (option, `val`)
        
    def ensure_filename (self, option, default=None):
        val = self._ensure_stringlike(option, "filename", None)
        if val is not None and not os.path.exists(val):
            raise DistutilsOptionError, \
                  "error in '%s' option: file '%s' does not exist" % \
                  (option, val)



    def run (self):

        print "before _get_package_data():"
        print "vendor =", self.vendor
        print "packager =", self.packager
        print "doc_files =", self.doc_files
        print "changelog =", self.changelog

        # make directories
        if self.spec_only:
            spec_dir = "dist"
            self.mkpath(spec_dir)       # XXX should be configurable
        else:
            rpm_base = os.path.join(self.bdist_base, "rpm")
            rpm_dir = {}
            for d in ('SOURCES', 'SPECS', 'BUILD', 'RPMS', 'SRPMS'):
                rpm_dir[d] = os.path.join(rpm_base, d)
                self.mkpath(rpm_dir[d])
            spec_dir = rpm_dir['SPECS']

        # Spec file goes into 'dist' directory if '--spec-only specified',
        # into build/rpm.<plat> otherwise.
        spec_path = os.path.join(spec_dir,
                                 "%s.spec" % self.distribution.get_name())
        self.execute(write_file,
                     (spec_path,
                      self._make_spec_file()),
                     "writing '%s'" % spec_path)

        if self.spec_only: # stop if requested
            return

        # Make a source distribution and copy to SOURCES directory with
        # optional icon.
        sdist = self.reinitialize_command ('sdist')
        if self.use_bzip2:
            sdist.formats = ['bztar']
        else:
            sdist.formats = ['gztar']
        self.run_command('sdist')

        source = sdist.get_archive_files()[0]
        source_dir = rpm_dir['SOURCES']
        self.copy_file(source, source_dir)

        if self.icon:
            if os.path.exists(self.icon):
                self.copy_file(self.icon, source_dir)
            else:
                raise DistutilsFileError, \
                      "icon file '%s' does not exist" % self.icon
        

        # build package
        self.announce('Building RPMs')
        rpm_args = ['rpm',]
        if self.source_only: # what kind of RPMs?
            rpm_args.append('-bs')
        elif self.binary_only:
            rpm_args.append('-bb')
        else:
            rpm_args.append('-ba')
        topdir = os.getcwd() + 'build/rpm'
        rpm_args.extend(['--define',
                         '_topdir ' + os.getcwd() + '/build/rpm',])
        if self.clean:
            rpm_args.append('--clean')
        rpm_args.append(spec_path)
        self.spawn(rpm_args)

    # run()


    def _get_package_data(self):
        ''' Get data needed to generate spec file, first from the
        DistributionMetadata class, then from the package_data file, which is
        Python code read with execfile() '''

        from string import join

        package_type = 'rpm'
        
        # read in package data, if any
        if os.path.exists('package_data'):
            try:
                exec(open('package_data'))
            except:
                raise DistutilsOptionError, 'Unable to parse package data file'

        # set instance variables, supplying default value if not provided in
        # package data file
        self.package_data = locals()

        # the following variables must be {string (len() = 2): string}
        self.summaries = self._check_string_dict('summaries')
        self.descriptions = self._check_string_dict('descriptions')

        # The following variable must be an ordinary number or a string
        self.release = self._check_number_or_string('release', '1')
        self.serial = self._check_number_or_string('serial')

        # The following variables must be strings
        self.group = self._check_string('group', 'Development/Libraries')
        self.vendor = self._check_string('vendor')
        self.packager = self._check_string('packager')
        self.changelog = self._check_string('changelog')
        self.icon = self._check_string('icon')
        self.distribution_name = self._check_string('distribution_name')
        self.pre = self._check_string('pre')
        self.post = self._check_string('post')
        self.preun = self._check_string('preun')
        self.postun = self._check_string('postun')
        self.prep = self._check_string('prep', '%setup')
        if self.use_rpm_opt_flags:
            self.build = (self._check_string(
                'build',
                'env CFLAGS="$RPM_OPT_FLAGS" python setup.py build'))
        else:
            self.build = (self._check_string('build', 'python setup.py build'))
        self.install = self._check_string(
            'install',
            'python setup.py install --root=$RPM_BUILD_ROOT --record')
        self.clean = self._check_string(
            'clean',
            'rm -rf $RPM_BUILD_ROOT')

        # The following variables must be a list or tuple of strings, or a
        # string
        self.doc = self._check_string_list('doc')
        if type(self.doc) == ListType:
            for readme in ('README', 'README.txt'):
                if os.path.exists(readme) and readme not in self.doc:
                    self.doc.append(readme)
            self.doc = join(self.doc)
        self.provides = join(self._check_string_list('provides'))
        self.requires = join(self._check_string_list('requires'))
        self.conflicts = join(self._check_string_list('conflicts'))
        self.build_requires = join(self._check_string_list('build_requires'))
        self.obsoletes = join(self._check_string_list('obsoletes'))

    def _make_spec_file(self):
        """Generate the text of an RPM spec file and return it as a
        list of strings (one per line).
        """
        # definitions and headers
        spec_file = [
            '%define name ' + self.distribution.get_name(),
            '%define version ' + self.distribution.get_version(),
            '%define release ' + self.release,
            '',
            'Summary: ' + self.distribution.get_description(),
            ]

        # put locale summaries into spec file
        # XXX not supported for now (hard to put a dictionary
        # in a config file -- arg!)
        #for locale in self.summaries.keys():
        #    spec_file.append('Summary(%s): %s' % (locale,
        #                                          self.summaries[locale]))

        spec_file.extend([
            'Name: %{name}',
            'Version: %{version}',
            'Release: %{release}',])

        # XXX yuck! this filename is available from the "sdist" command,
        # but only after it has run: and we create the spec file before
        # running "sdist", in case of --spec-only.
        if self.use_bzip2:
            spec_file.append('Source0: %{name}-%{version}.tar.bz2')
        else:
            spec_file.append('Source0: %{name}-%{version}.tar.gz')

        spec_file.extend([
            'Copyright: ' + self.distribution.get_licence(),
            'Group: ' + self.group,
            'BuildRoot: %{_tmppath}/%{name}-buildroot',
            'Prefix: %{_prefix}', ])

        # noarch if no extension modules
        if not self.distribution.has_ext_modules():
            spec_file.append('BuildArchitectures: noarch')

        for field in ('Vendor',
                      'Packager',
                      'Provides',
                      'Requires',
                      'Conflicts',
                      'Obsoletes',
                      ):
            val = getattr(self, string.lower(field))
            if type(val) is ListType:
                spec_file.append('%s: %s' % (field, string.join(val)))
            elif val is not None:
                spec_file.append('%s: %s' % (field, val))
                
                      
        if self.distribution.get_url() != 'UNKNOWN':
            spec_file.append('Url: ' + self.distribution.get_url())

        if self.distribution_name:
             spec_file.append('Distribution: ' + self.distribution_name)

        if self.build_requires:
             spec_file.append('BuildRequires: ' +
                              string.join(self.build_requires))

        if self.icon:
            spec_file.append('Icon: ' + os.path.basename(self.icon))

        spec_file.extend([
            '',
            '%description',
            self.distribution.get_long_description()
            ])

        # put locale descriptions into spec file
        # XXX again, suppressed because config file syntax doesn't
        # easily support this ;-(
        #for locale in self.descriptions.keys():
        #    spec_file.extend([
        #        '',
        #        '%description -l ' + locale,
        #        self.descriptions[locale],
        #        ])

        # rpm scripts
        for (rpm_opt, attr) in (('prep', 'prep_cmd'),
                                ('build', 'build_cmd'),
                                ('install', 'install_cmd'),
                                ('clean', 'clean_cmd'),
                                ('pre', 'pre_install'),
                                ('post', 'post_install'),
                                ('preun', 'pre_uninstall'),
                                ('postun', 'post_uninstall')):
            # XXX oops, this doesn't distinguish between "raw code"
            # options and "script filename" options -- well, we probably
            # should settle on one or the other, and not make the
            # distinction!
            val = getattr(self, attr)
            if val:
                spec_file.extend([
                    '',
                    '%' + rpm_opt,
                    val
                    ])

        
        # files section
        spec_file.extend([
            '',
            '%files -f INSTALLED_FILES',
            '%defattr(-,root,root)',
            ])

        if self.doc_files:
            spec_file.append('%doc ' + string.join(self.doc_files))

        if self.changelog:
            spec_file.extend([
                '',
                '%changelog',
                self.changelog
                ])

        return spec_file

    def _check_string_dict(self, var_name, default_value = {}):
        ''' Tests a wariable to determine if it is {string: string},
        var_name is the name of the wariable. Return the value if it is valid,
        returns default_value if the variable does not exist, raises
        DistutilsOptionError if the variable is not valid'''
        if self.package_data.has_key(var_name):
            pass_test = 1 # set to 0 if fails test
            value = self.package_data[var_name]
            if type(value) == DictType:
                for locale in value.keys():
                    if (type(locale) != StringType) or (type(value[locale]) !=
                                                         StringType):
                        pass_test = 0
                        break
                if pass_test:
                    return test_me
            raise DistutilsOptionError, \
                  ("Error in package_data: '%s' must be dictionary: "
                   '{string: string}' % var_name)
        else:
            return default_value

    def _check_string(self, var_name, default_value = None):
        ''' Tests a variable in package_data to determine if it is a string,
        var_name is the name of the wariable. Return the value if it is a
        string, returns default_value if the variable does not exist, raises
        DistutilsOptionError if the variable is not a string'''
        if self.package_data.has_key(var_name):
            if type(self.package_data[var_name]) == StringType:
                return self.package_data[var_name]
            else:
                raise DistutilsOptionError, \
                      "Error in package_data: '%s' must be a string" % var_name
        else:
            return default_value

    def _check_number_or_string(self, var_name, default_value = None):
        ''' Tests a variable in package_data to determine if it is a number or
        a string, var_name is the name of the wariable. Return the value if it
        is valid, returns default_value if the variable does not exist, raises
        DistutilsOptionError if the variable is not valid'''
        if self.package_data.has_key(var_name):
            if type(self.package_data[var_name]) in (StringType, LongType,
                                                     IntType, FloatType):
                return str(self.package_data[var_name])
            else:
                raise DistutilsOptionError, \
                      ("Error in package_data: '%s' must be a string or a "
                       'number' % var_name)
        else:
            return default_value

    def _check_string_list(self, var_name, default_value = []):
        ''' Tests a variable in package_data to determine if it is a string or
        a list or tuple of strings, var_name is the name of the wariable.
        Return the value as a string or a list if it is valid, returns
        default_value if the variable does not exist, raises
        DistutilsOptionError if the variable is not valid'''
        if self.package_data.has_key(var_name):
            value = self.package_data[var_name]
            pass_test = 1
            if type(value) == StringType:
                return value
            elif type(value) in (ListType, TupleType):
                for item in value:
                    if type(item) != StringType:
                        pass_test = 0
                        break
                if pass_test:
                    return list(value)
            raise DistutilsOptionError, \
                  ("Error in package_data: '%s' must be a string or a "
                   'list or tuple of strings' % var_name)
        else:
            return default_value
