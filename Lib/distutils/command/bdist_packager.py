"""distutils.command.bdist_ packager

Modified from bdist_dumb by Mark W. Alexander <slash@dotnetslash.net>

Implements the Distutils 'bdist_packager' abstract command
to be subclassed by binary package creation commands."""


__revision__ = "$Id: bdist_packager.py,v 0.1 2001/04/4 mwa"

import os
from distutils.core import Command
from distutils.util import get_platform
from distutils.dir_util import create_tree, remove_tree
from distutils.file_util import write_file
from distutils.errors import *
from distutils import log
import string, sys

class bdist_packager (Command):

    description = "abstract base for package manager specific bdist commands"

# XXX update user_options
    user_options = [
         ('bdist-base=', None,
         "base directory for creating built distributions"),
        ('pkg-dir=', None,
         "base directory for creating binary packages (defaults to \"binary\" under "),
        ('dist-dir=', 'd',
         "directory to put final RPM files in "
         "(and .spec files if --spec-only)"),
        ('category=', None,
         "Software category (packager dependent format)"),
        ('revision=', None,
         "package revision number"),
        # the following have moved into the distribution class
        #('packager=', None,
         #"Package maintainer"),
        #('packager-mail=', None,
         #"Package maintainer's email address"),
        #('author=', None,
         #"Package author"),
        #('author-mail=', None,
         #"Package author's email address"),
        #('license=', None,
         #"License code"),
        #('licence=', None,
         #"alias for license"),
        ('icon=', None,
         "Package icon"),
        ('subpackages=', None,
         "Comma seperated list of seperately packaged trees"),
        ('preinstall=', None,
         "preinstall script (Bourne shell code)"),
        ('postinstall=', None,
         "postinstall script (Bourne shell code)"),
        ('preremove=', None,
         "preremove script (Bourne shell code)"),
        ('postremove=', None,
         "postremove script (Bourne shell code)"),
        ('requires=', None,
         "capabilities required by this package"),
        ('keep-temp', 'k',
         "don't clean up RPM build directory"),
        ('control-only', None,
         "Generate package control files and stop"),
        ('no-autorelocate', None,
         "Inhibit automatic relocation to installed site-packages"),
         ]

    boolean_options = ['keep-temp', 'control-only', 'no_autorelocate']

    def ensure_string_not_none (self,option,default=None):
        val = getattr(self,option)
        if val is not None:
            return
        Command.ensure_string(self,option,default)
        val = getattr(self,option)
        if val is None:
            raise DistutilsOptionError, "'%s' must be provided" % option

    def ensure_script (self,arg):
        if not arg:
            return
        try:
            self.ensure_string(arg, None)
        except:
            try:
                self.ensure_filename(arg, None)
            except:
                raise RuntimeError, \
                    "cannot decipher script option (%s)" \
                    % arg

    def write_script (self,path,attr,default=None):
        """ write the script specified in attr to path. if attr is None,
            write use default instead """
        val = getattr(self,attr)
        if not val:
            if not default:
                return
            else:
                setattr(self,attr,default)
                val = default
        if val != "":
            log.info('Creating %s script', attr)
            self.execute(write_file,
                     (path, self.get_script(attr)),
                     "writing '%s'" % path)

    def get_binary_name(self):
        py_ver = sys.version[0:string.find(sys.version,' ')]
        return self.name + '-' + self.version + '-' + \
               self.revision + '-' + py_ver

    def get_script (self,attr):
        # accept a script as a string ("line\012line\012..."),
        # a filename, or a list
        # XXX We could probably get away with copy_file, but I'm
        # guessing this will be more flexible later on....
        val = getattr(self,attr)
        ret=None
        if val:
            try:
                os.stat(val)
                # script is a file
                ret=[]
                f=open(val)
                ret=string.split(f.read(),"\012");
                f.close()
                #return ret
            except:
                if type(val)==type(""):
                    # script is a string
                    ret = string.split(val,"\012")
                elif type(val)==type([]):
                    # script is a list
                    ret = val
                else:
                    raise RuntimeError, \
                        "cannot figure out what to do with 'request' option (%s)" \
                        % val
            return ret


    def initialize_options (self):
        self.keep_temp = 0
        self.control_only = 0
        self.no_autorelocate = 0
        self.pkg_dir = None
        self.plat_name = None
        self.icon = None
        self.requires = None
        self.subpackages = None
        self.category = None
        self.revision = None

        # PEP 241 Metadata
        self.name = None
        self.version = None
        #self.url = None
        #self.author = None
        #self.author_email = None
        #self.maintainer = None
        #self.maintainer_email = None
        #self.description = None
        #self.long_description = None
        #self.licence = None
        #self.platforms = None
        #self.keywords = None
        self.root_package = None

        # package installation scripts
        self.preinstall = None
        self.postinstall = None
        self.preremove = None
        self.postremove = None
    # initialize_options()


    def finalize_options (self):

        if self.pkg_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.pkg_dir = os.path.join(bdist_base, 'binary')

        if not self.plat_name:
            self.plat = self.distribution.get_platforms()
        if self.distribution.has_ext_modules():
            self.plat_name = [sys.platform,]
        else:
            self.plat_name = ["noarch",]

        d = self.distribution
        self.ensure_string_not_none('name', d.get_name())
        self.ensure_string_not_none('version', d.get_version())
        self.ensure_string('category')
        self.ensure_string('revision',"1")
        #self.ensure_string('url',d.get_url())
        if type(self.distribution.packages) == type([]):
            self.root_package=self.distribution.packages[0]
        else:
            self.root_package=self.name
        self.ensure_string('root_package',self.root_package)
        #self.ensure_string_list('keywords')
        #self.ensure_string_not_none('author', d.get_author())
        #self.ensure_string_not_none('author_email', d.get_author_email())
        self.ensure_filename('icon')
        #self.ensure_string_not_none('maintainer', d.get_maintainer())
        #self.ensure_string_not_none('maintainer_email',
                        #d.get_maintainer_email())
        #self.ensure_string_not_none('description', d.get_description())
        #self.ensure_string_not_none('long_description', d.get_long_description())
        #if self.long_description=='UNKNOWN':
            #self.long_description=self.description
        #self.ensure_string_not_none('license', d.get_license())
        self.ensure_string_list('requires')
        self.ensure_filename('preinstall')
        self.ensure_filename('postinstall')
        self.ensure_filename('preremove')
        self.ensure_filename('postremove')

    # finalize_options()


    def run (self):

        raise RuntimeError, \
            "abstract method -- subclass %s must override" % self.__class__
        self.run_command('build')

        install = self.reinitialize_command('install', reinit_subcommands=1)
        install.root = self.pkg_dir

        log.info("installing to %s", self.pkg_dir)
        self.run_command('install')
        if not self.keep_temp:
            remove_tree(self.pkg_dir, dry_run=self.dry_run)

    # run()

# class bdist_packager
