"""distutils.command.bdist_pkgtool

Implements the Distutils 'bdist_sdux' command to create HP-UX 
swinstall depot"""

# Mark Alexander <slash@dotnetslash.net>

__revision__ = "$Id: bdist_sdux.py,v 0.2 "
import os, string
from types import *
from distutils.core import Command, DEBUG
from distutils.util import get_platform
from distutils.file_util import write_file
from distutils.errors import *
from distutils.command import bdist_packager
from distutils import log
import sys
from commands import getoutput

DEFAULT_CHECKINSTALL="""#!/bin/sh
/usr/bin/which python 2>&1 >/dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: Python must be on your PATH"  &>2
    echo "ERROR: (You may need to link it to /usr/bin) "  &>2
    exit 1
fi
PY_DIR=`python -c "import sys;print '%s/lib/python%s' % (sys.exec_prefix,sys.version[0:3])" #2>/dev/null`
PY_PKG_DIR=`python -c "import sys;print '%s/lib/python%s/site-packages' % (sys.exec_prefix,sys.version[0:3])" #2>/dev/null`
PY_LIB_DIR=`dirname $PY_PKG_DIR`

if [ "`dirname ${SW_LOCATION}`" = "__DISTUTILS_PKG_DIR__" ]; then
    # swinstall to default location
    if [ "${PY_PKG_DIR}" != "__DISTUTILS_PKG_DIR__" ]; then
        echo "ERROR: " &>2
        echo "ERROR: Python is not installed where this package expected!" &>2
        echo "ERROR: You need to manually relocate this package to your python installation." &>2
        echo "ERROR: " &>2
        echo "ERROR: Re-run swinstall specifying the product name:location, e.g.:" &>2
        echo "ERROR: " &>2
        echo "ERROR:     swinstall -s [source] __DISTUTILS_NAME__:${PY_PKG_DIR}/__DISTUTILS_DIRNAME__" &>2
        echo "ERROR: " &>2
        echo "ERROR: to relocate this package to match your python installation" &>2
        echo "ERROR: " &>2
        exit 1
    fi
else
    if [ "`dirname ${SW_LOCATION}`" != "${PY_PKG_DIR}" -a "`dirname ${SWLOCATION}`" != "${PY_LIB_DIR}" ]; then
        echo "WARNING: " &>2
        echo "WARNING: Package is being installed outside the 'normal' python search path!" &>2
        echo "WARNING: Add ${SW_LOCATION} to PYTHONPATH to use this package" &>2
        echo "WARNING: " &>2
    fi
fi
"""

DEFAULT_POSTINSTALL="""#!/bin/sh
/usr/bin/which python 2>&1 >/dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: Python must be on your PATH"  &>2
    echo "ERROR: (You may need to link it to /usr/bin) "  &>2
    exit 1
fi
python -c "import compileall;compileall.compile_dir(\\"${SW_LOCATION}\\")"
"""

DEFAULT_PREREMOVE="""#!/bin/sh
# remove compiled bytecode files
find ${SW_LOCATION} -name "*.pyc" -exec rm {} \;
find ${SW_LOCATION} -name "*.pyo" -exec rm {} \;
"""

DEFAULT_POSTREMOVE="""#!/bin/sh
if [ `find ${SW_LOCATION} ! -type d | wc -l` -eq 0 ]; then
    # remove if there's nothing but empty directories left
    rm -rf ${SW_LOCATION}
fi
"""

class bdist_sdux(bdist_packager.bdist_packager):

    description = "create an HP swinstall depot"

    user_options = bdist_packager.bdist_packager.user_options + [
        #('revision=', None,
         #"package revision number (PSTAMP)"),
        ('keep-permissions', None,
         "Don't reset permissions and ownership to root/bin"), # XXX
        ('corequisites=', None,
         "corequisites"), # XXX
        ('prerequisites=', None,
         "prerequisites"), # XXX
        #('category=', None,
         #"Software category"),
        ('checkinstall=', None, # XXX ala request
         "checkinstall script (Bourne shell code)"),
        ('configure=', None, # XXX 
         "configure script (Bourne shell code)"),
        ('unconfigure=', None, # XXX
         "unconfigure script (Bourne shell code)"),
        ('verify=', None, # XXX
         "verify script (Bourne shell code)"),
        ('unpreinstall=', None, # XXX
         "unpreinstall script (Bourne shell code)"),
        ('unpostinstall=', None, # XXX
         "unpostinstall script (Bourne shell code)"),
       ]

    boolean_options = ['keep-permissions']

    def initialize_options (self):
        bdist_packager.bdist_packager.initialize_options(self)
        self.corequisites = None
        self.prerequesites = None
        self.checkinstall = None
        self.configure = None
        self.unconfigure = None
        self.verify = None
        self.unpreinstall = None
        self.unpostinstall = None
        # More
        self.copyright = None
        self.readme = None
        self.machine_type = None
        self.os_name = None
        self.os_release = None
        self.directory = None
        self.readme = None
        self.copyright = None
        self.architecture= None
        self.keep_permissions= None
        options = self.distribution.get_option_dict('bdist_packager')
        for key in options.keys():
            setattr(self,key,options[key][1])

    # initialize_options()


    def finalize_options (self):
        global DEFAULT_CHECKINSTALL, DEFAULT_POSTINSTALL
        global DEFAULT_PREREMOVE, DEFAULT_POSTREMOVE
        if self.pkg_dir==None:
            dist_dir = self.get_finalized_command('bdist').dist_dir
            self.pkg_dir = os.path.join(dist_dir, "sdux")  
        self.ensure_script('corequisites')
        self.ensure_script('prerequesites')
        self.ensure_script('checkinstall')
        self.ensure_script('configure')
        self.ensure_script('unconfigure')
        self.ensure_script('verify')
        self.ensure_script('unpreinstall')
        self.ensure_script('unpostinstall')
        self.ensure_script('copyright')
        self.ensure_script('readme')
        self.ensure_string('machine_type','*')
        if not self.__dict__.has_key('platforms'):
            # This is probably HP, but if it's not, use sys.platform
            if sys.platform[0:5] == "hp-ux":
                self.platforms = "HP-UX"
            else:
                self.platforms = string.upper(sys.platform)
        else:
            # we can only handle one
            self.platforms=string.join(self.platforms[0])
        self.ensure_string('os_release','*')
        self.ensure_string('directory','%s/lib/python%s/site-packages' % \
                (sys.exec_prefix, sys.version[0:3]))
        bdist_packager.bdist_packager.finalize_options(self)
        DEFAULT_CHECKINSTALL = string.replace(DEFAULT_CHECKINSTALL,
            "__DISTUTILS_NAME__", self.name)
        DEFAULT_CHECKINSTALL = string.replace(DEFAULT_CHECKINSTALL,
            "__DISTUTILS_DIRNAME__", self.root_package)
        DEFAULT_CHECKINSTALL = string.replace(DEFAULT_CHECKINSTALL,
            "__DISTUTILS_PKG_DIR__", self.directory)
        DEFAULT_POSTINSTALL = string.replace(DEFAULT_POSTINSTALL,
            "__DISTUTILS_DIRNAME__", self.root_package)
        #DEFAULT_PREREMOVE = string.replace(DEFAULT_PREREMOVE,
            #"__DISTUTILS_NAME__", self.root_package)
        #DEFAULT_POSTREMOVE = string.replace(DEFAULT_POSTREMOVE,
            #"__DISTUTILS_NAME__", self.root_package)
    # finalize_options()

    def run (self):
        # make directories
        self.mkpath(self.pkg_dir)
        psf_path = os.path.join(self.pkg_dir, 
                                 "%s.psf" % self.get_binary_name())
        # build package
        log.info('Building package')
        self.run_command('build')
        log.info('Creating psf file')
        self.execute(write_file,
                     (psf_path,
                      self._make_control_file()),
                     "writing '%s'" % psf_path)
        if self.control_only: # stop if requested
            return

        log.info('Creating package')
        spawn_cmd = ['swpackage', '-s']
        spawn_cmd.append(psf_path)
        spawn_cmd.append('-x')
        spawn_cmd.append('target_type=tape')
        spawn_cmd.append('@')
        spawn_cmd.append(self.pkg_dir+"/"+self.get_binary_name()+'.depot')
        self.spawn(spawn_cmd)

    # run()

  
    def _make_control_file(self):
        # Generate a psf file and return it as list of strings (one per line).
        # definitions and headers
        title = "%s %s" % (self.maintainer,self.maintainer_email)
        title=title[0:80]
        #top=self.distribution.packages[0]
        psf_file = [
            'vendor',               # Vendor information
            '    tag                %s' % "DISTUTILS",
            '    title              %s' % title,
            '    description        Distutils package maintainer (%s)' % self.license,
            'end',                  # end of vendor
            'product',              # Product information
            '    tag                %s' % self.name,
            '    title              %s' % self.description,
            '    description        %s' % self.description,
            '    revision           %s' % self.version,
            '    architecture       %s' % self.platforms,
            '    machine_type       %s' % self.machine_type,
            '    os_name            %s' % self.platforms,
            '    os_release         %s' % self.os_release,
            '    directory          %s' % self.directory + "/" + self.root_package,
            ]

        self.write_script(os.path.join(self.pkg_dir, "checkinstall"),
                'checkinstall',DEFAULT_CHECKINSTALL)
        psf_file.extend(['    checkinstall       %s/checkinstall' % self.pkg_dir])
        self.write_script(os.path.join(self.pkg_dir, "postinstall"),
                'postinstall',DEFAULT_POSTINSTALL)
        psf_file.extend(['    postinstall        %s/postinstall' % self.pkg_dir])
        self.write_script(os.path.join(self.pkg_dir, "preremove"),
                'preremove',DEFAULT_PREREMOVE)
        psf_file.extend(['    preremove          %s/preremove' % self.pkg_dir])
        self.write_script(os.path.join(self.pkg_dir, "postremove"),
                'postremove',DEFAULT_POSTREMOVE)
        psf_file.extend(['    postremove         %s/postremove' % self.pkg_dir])
        if self.preinstall:
            self.write_script(self.pkg_dir+"/preinstall", 'preinstall', None)
            psf_file.extend(['    preinstall         %s/preinstall' % self.pkg_dir])
        if self.configure:
            self.write_script(self.pkg_dir+"/configure", 'configure', None)
            psf_file.extend(['    configure          %s/configure' % self.pkg_dir])
        if self.unconfigure:
            self.write_script(self.pkg_dir+"/unconfigure", 'unconfigure', None)
            psf_file.extend(['    unconfigure        %s/unconfigure' % self.pkg_dir])
        if self.verify:
            self.write_script(self.pkg_dir+"/verify", 'verify', None)
            psf_file.extend(['    verify             %s/verify' % self.pkg_dir])
        if self.unpreinstall:
            self.write_script(self.pkg_dir+"/unpreinstall", 'unpreinstall', None)
            psf_file.extend(['    unpreinstall       %s/unpreinstall' % self.pkg_dir])
        if self.unpostinstall:
            self.write_script(self.pkg_dir+"/unpostinstall", 'unpostinstall', None)
            psf_file.extend(['    unpostinstall      %s/unpostinstall' % self.pkg_dir])
        psf_file.extend(['    is_locatable       true'])
        #if self.long_description:
            #psf_file.extend([self.long_description])
        if self.copyright:
            # XX make a copyright file XXX
            write_script('copyright')
            psf_file.extend(['    copyright         <copyright'])
        if self.readme:
            # XX make a readme file XXX
            write_script('readme')
            psf_file.extend(['    readme            <readme'])

        psf_file.extend(['    fileset'])    # start fileset
        if self.distribution.ext_modules:
            platlib=self.get_finalized_command('build').build_platlib
        else:
            platlib=self.get_finalized_command('build').build_lib
        title = "%s installation files" % (self.name)
        psf_file.extend([
            '        tag            %s' % 'FILES',
            '        title          %s' % "%s installation files" % (self.name),
            '        revision       %s' % self.version,
            '        directory      %s/%s=%s/%s' % (platlib,self.root_package,self.directory,self.root_package),
            ])
        if not self.keep_permissions:
            psf_file.extend(['        file_permissions    -m 555 -o root -g bin'])
        psf_file.extend(['        file           *'])
        psf_file.extend(['    end'])        # end of fileset
        psf_file.extend(['end'])    # end of product
               
# XXX add fileset information
        

        return psf_file

    # _make_control_file ()


# class bdist_sdux
