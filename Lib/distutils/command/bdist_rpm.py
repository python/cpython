"""distutils.command.bdist_rpm

Implements the Distutils 'bdist_rpm' command (create RPM source and binary
distributions."""

# created 2000/04/25, by Harry Henry Gebel

__revision__ = "$Id$"

from os.path import exists, basename
import os
from distutils.core import Command
from distutils.util import mkpath, write_file, copy_file
from distutils.errors import *
from string import join, lower
from types import StringType, DictType, LongType, FloatType, IntType, \
     ListType, TupleType

class bdist_rpm (Command):

    description = "create an RPM distribution"

    user_options = [
        ('spec-only', None,
         "Only regenerate spec file"),
        ('source-only', None,
         "Only generate source RPM"),
        ('binary-only', None,
         "Only generate binary RPM"),
        ('use-bzip2', None,
         "Use bzip2 instead of gzip to create source distribution"),
        ('no-clean', None,
         "Do not clean RPM build directory"),
        ('no-rpm-opt-flags', None,
         "Do not pass any RPM CFLAGS to compiler")
        ]


    def initialize_options (self):
        self.spec_only = None
        self.binary_only = None
        self.source_only = None
        self.use_bzip2 = None
        self.no_clean = None
        self.no_rpm_opt_flags = None

    # initialize_options()


    def finalize_options (self):
        if os.name != 'posix':
            raise DistutilsPlatformError, \
                  ("don't know how to create RPM "
                   "distributions on platform %s" % os.name)
        if self.binary_only and self.source_only:
            raise DistutilsOptionsError, \
                  "Cannot supply both '--source-only' and '--binary-only'"
        # don't pass CFLAGS to pure python distributions
        if not self.distribution.has_ext_modules():
            self.no_rpm_opt_flags = 1

    # finalize_options()


    def run (self):
        self._get_package_data() # get packaging info


        # make directories
        if self.spec_only:
            self.execute(mkpath, ('redhat',), "Created './redhat' directory")
        else:
            self.execute(mkpath, ('build/rpm/SOURCES',),
                         "Created RPM source directory")
            self.execute(mkpath, ('build/rpm/SPECS',),
                         "Created RPM source directory")
            self.execute(mkpath, ('build/rpm/BUILD',),
                         "Created RPM source directory")
            self.execute(mkpath, ('build/rpm/RPMS',),
                         "Created RPM source directory")
            self.execute(mkpath, ('build/rpm/SRPMS',),
                         "Created RPM source directory")

        # spec file goes into .redhat directory if '--spec-only specified',
        # into build/rpm/spec otherwisu
        if self.spec_only:
            spec_path = './redhat/%s.spec' % self.distribution.get_name()
        else:
            spec_path = ('build/rpm/SPECS/%s.spec' %
                         self.distribution.get_name())
        self.execute(write_file,
                     (spec_path,
                      self._make_spec_file()),
                     'Writing .spec file')

        if self.spec_only: # stop if requested
            return

        # make a source distribution and copy to SOURCES directory with
        # optional icon
        sdist = self.find_peer ('sdist')
        if self.use_bzip2:
            sdist.formats = ['bztar']
        else:
            sdist.formats = ['gztar']
        self.run_peer('sdist')
        if self.use_bzip2:
            source = self.distribution.get_fullname() + '.tar.bz2'
        else:
            source = self.distribution.get_fullname() + '.tar.gz'
        self.execute(copy_file, (source, 'build/rpm/SOURCES'),
                     'Copying source distribution to SOURCES')
        if self.icon:
            if exists(self.icon):
                self.execute(copy_file, (self.icon, 'build/rpm/SOURCES'),
                     'Copying icon to SOURCES')
            else:
                raise DistutilsFileError, \
                      "Unable to find icon file '%s'" % self.icon
        

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
        if not self.no_clean:
            rpm_args.append('--clean')
        rpm_args.append(spec_path)
        self.spawn(rpm_args)

    # run()


    def _get_package_data(self):
        ''' Get data needed to generate spec file, first from the
        DistributionMetadata class, then from the package_data file, which is
        Python code read with execfile() '''

        package_type = 'rpm'
        
        # read in package data, if any
        if exists('package_data'):
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
        if not self.no_rpm_opt_flags:
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
                if exists(readme) and readme not in self.doc:
                    self.doc.append(readme)
            self.doc = join(self.doc)
        self.provides = join(self._check_string_list('provides'))
        self.requires = join(self._check_string_list('requires'))
        self.conflicts = join(self._check_string_list('conflicts'))
        self.build_requires = join(self._check_string_list('build_requires'))
        self.obsoletes = join(self._check_string_list('obsoletes'))

    def _make_spec_file(self):
        ''' Generate an RPM spec file '''

        # definitons and headers
        spec_file = [
            '%define name ' + self.distribution.get_name(),
            '%define version ' + self.distribution.get_version(),
            '%define release ' + self.release,
            '',
            'Summary: ' + self.distribution.get_description(),
            ]

        # put locale summaries into spec file
        for locale in self.summaries.keys():
            spec_file.append('Summary(%s): %s' % (locale,
                                                  self.summaries[locale]))

        spec_file.extend([
            'Name: %{name}',
            'Version: %{version}',
            'Release: %{release}',])
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
            if getattr(self, lower(field)):
                spec_file.append('%s: %s' % (field, getattr(self,
                                                            lower(field))))
                      
        if self.distribution.get_url() != 'UNKNOWN':
            spec_file.append('Url: ' + self.distribution.get_url())

        if self.distribution_name:
             spec_file.append('Distribution: ' + self.distribution_name)

        if self.build_requires:
             spec_file.append('BuildRequires: ' + self.build_requires)

        if self.icon:
            spec_file.append('Icon: ' + basename(self.icon))

        spec_file.extend([
            '',
            '%description',
            self.distribution.get_long_description()
            ])

        # put locale descriptions into spec file
        for locale in self.descriptions.keys():
            spec_file.extend([
                '',
                '%description -l ' + locale,
                self.descriptions[locale],
                ])

        # rpm scripts
        for script in ('prep',
                       'build',
                       'install',
                       'clean',
                       'pre',
                       'post',
                       'preun',
                       'postun',
                       ):
            if getattr(self, script):
                spec_file.extend([
                    '',
                    '%' + script,
                    getattr(self, script),
                    ])

        
        # files section
        spec_file.extend([
            '',
            '%files -f INSTALLED_FILES',
            '%defattr(-,root,root)',
            ])

        if self.doc:
            spec_file.append('%doc ' + self.doc)

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

# class bdist_rpm
