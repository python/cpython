"""distutils.command.bdist_pkgtool


Author: Mark W. Alexander <slash@dotnet.net>

Implements the Distutils 'bdist_pkgtool' command (create Solaris pkgtool
distributions)."""

import os, string, sys, pwd, grp
import glob
from types import *
from distutils.core import Command, DEBUG
from distutils.util import get_platform
from distutils.file_util import write_file
from distutils.errors import *
from distutils.command import bdist_packager
from distutils import sysconfig
from distutils import log
import compileall
from commands import getoutput

__revision__ = "$Id: bdist_pkgtool.py,v 0.3 mwa "

# default request script - Is also wrapped around user's request script
# unless --no-autorelocate is requested. Finds the python site-packages
# directory and prompts for verification
DEFAULT_REQUEST="""#!/bin/sh
######################################################################
#         Distutils internal package relocation support              #
######################################################################

PRODUCT="__DISTUTILS_NAME__"

trap `exit 3` 15
/usr/bin/which python 2>&1 >/dev/null
if [ $? -ne 0 ]; then
    echo "The python interpretor needs to be on your path!"
    echo
    echo "If you have more than one, make sure the first one is where"
    echo "you want this module installed:"
    exit 1
fi

PY_DIR=`python -c "import sys;print '%s/lib/python%s' % (sys.exec_prefix,sys.version[0:3])" 2>/dev/null`
PY_PKG_DIR=`python -c "import sys;print '%s/lib/python%s/site-packages' % (sys.exec_prefix,sys.version[0:3])" 2>/dev/null`

echo ""
if [ -z "${PY_DIR}" ]; then
        echo "I can't seem to find the python distribution."
        echo "I'm assuming the default path for site-packages"
else
        BASEDIR="${PY_PKG_DIR}"
        cat <<EOF
        Python found! The default path:

        ${BASEDIR}

        will install ${PRODUCT} such that all python users
        can import it.

        If you just want individual access, you can install into
        any directory and add that directory to your \$PYTHONPATH
EOF
fi
echo ""

BASEDIR=`ckpath -d ${BASEDIR} -ay \
        -p "Where should ${PRODUCT} be installed? [${BASEDIR}]"` || exit $?

######################################################################
#                user supplied request script follows                #
######################################################################
"""
### request script

# default postinstall compiles and changes ownership on the module directory
# XXX The ownership change _should_ be handled by adjusting the prototype file
DEFAULT_POSTINSTALL="""#!/bin/sh
/usr/bin/test -d ${BASEDIR}/__DISTUTILS_NAME__
if [ $? -eq 0 ]; then
        python -c "import compileall;compileall.compile_dir(\\"${BASEDIR}/__DISTUTILS_NAME__\\")"
        chown -R root:other ${BASEDIR}/__DISTUTILS_NAME__
fi
"""

# default preremove deletes *.pyc and *.pyo from the module tree
# This should leave the tree empty, so pkgtool will remove it.
# If the user's scripts place anything else in there (e.g. config files),
# pkgtool will leave the directory intact
DEFAULT_PREREMOVE="""#!/bin/sh

/usr/bin/which python 2>&1 >/dev/null
if [ $? -ne 0 ]; then
    echo "The python interpretor needs to be on your path!"
    echo
    echo "If you have more than one, make sure the first one is where"
    echo "you want this module removed from"
    exit 1
fi

/usr/bin/test -d ${BASEDIR}/__DISTUTILS_NAME__
if [ $? -eq 0 ]; then
        find ${BASEDIR}/__DISTUTILS_NAME__ -name "*.pyc" -exec rm {} \;
        find ${BASEDIR}/__DISTUTILS_NAME__ -name "*.pyo" -exec rm {} \;
fi
"""

# default postremove removes the module directory _IF_ no files are
# there (Turns out this isn't needed if the preremove does it's job
# Left for posterity
DEFAULT_POSTREMOVE="""#!/bin/sh

/usr/bin/test -d ${BASEDIR}/__DISTUTILS_NAME__
if [ $? -eq 0 ]; then
        if [ `find ${BASEDIR}/__DISTUTILS_NAME__ ! -type d | wc -l` -eq 0 ]; then
        rm -rf ${BASEDIR}/__DISTUTILS_NAME__
        fi
fi
"""

class bdist_pkgtool (bdist_packager.bdist_packager):

    description = "create an pkgtool (Solaris) package"

    user_options = bdist_packager.bdist_packager.user_options + [
        ('revision=', None,
         "package revision number (PSTAMP)"),
        ('pkg-abrev=', None,
         "Abbreviation (9 characters or less) of the package name"),
        ('compver=', None,
         "file containing compatible versions of this package (man compver)"),
        ('depend=', None,
         "file containing dependencies for this package (man depend)"),
        #('category=', None,
         #"Software category"),
        ('request=', None,
         "request script (Bourne shell code)"),
       ]

    def initialize_options (self):
        # XXX Check for pkgtools on path...
        bdist_packager.bdist_packager.initialize_options(self)
        self.compver = None
        self.depend = None
        self.vendor = None
        self.classes = None
        self.request = None
        self.pkg_abrev = None
        self.revision = None
        # I'm not sure I should need to do this, but the setup.cfg
        # settings weren't showing up....
        options = self.distribution.get_option_dict('bdist_packager')
        for key in options.keys():
            setattr(self,key,options[key][1])

    # initialize_options()


    def finalize_options (self):
        global DEFAULT_REQUEST, DEFAULT_POSTINSTALL
        global DEFAULT_PREREMOVE, DEFAULT_POSTREMOVE
        if self.pkg_dir is None:
            dist_dir = self.get_finalized_command('bdist').dist_dir
            self.pkg_dir = os.path.join(dist_dir, "pkgtool")

        self.ensure_string('classes', None)
        self.ensure_string('revision', "1")
        self.ensure_script('request')
        self.ensure_script('preinstall')
        self.ensure_script('postinstall')
        self.ensure_script('preremove')
        self.ensure_script('postremove')
        self.ensure_string('vendor', None)
        if self.__dict__.has_key('author'):
            if self.__dict__.has_key('author_email'):
                self.ensure_string('vendor',
                        "%s <%s>" % (self.author,
                           self.author_email))
            else:
                self.ensure_string('vendor',
                     "%s" % (self.author))
        self.ensure_string('category', "System,application")
        self.ensure_script('compver')
        self.ensure_script('depend')
        bdist_packager.bdist_packager.finalize_options(self)
        if self.pkg_abrev is None:
            self.pkg_abrev=self.name
        if len(self.pkg_abrev)>9:
            raise DistutilsOptionError, \
            "pkg-abrev (%s) must be less than 9 characters" % self.pkg_abrev
        # Update default scripts with our metadata name
        DEFAULT_REQUEST = string.replace(DEFAULT_REQUEST,
            "__DISTUTILS_NAME__", self.root_package)
        DEFAULT_POSTINSTALL = string.replace(DEFAULT_POSTINSTALL,
            "__DISTUTILS_NAME__", self.root_package)
        DEFAULT_PREREMOVE = string.replace(DEFAULT_PREREMOVE,
            "__DISTUTILS_NAME__", self.root_package)
        DEFAULT_POSTREMOVE = string.replace(DEFAULT_POSTREMOVE,
            "__DISTUTILS_NAME__", self.root_package)

    # finalize_options()


    def make_package(self,root=None):
        # make directories
        self.mkpath(self.pkg_dir)
        if root:
            pkg_dir = self.pkg_dir+"/"+root
            self.mkpath(pkg_dir)
        else:
            pkg_dir = self.pkg_dir

        install = self.reinitialize_command('install', reinit_subcommands=1)
        # build package
        log.info('Building package')
        self.run_command('build')
        log.info('Creating pkginfo file')
        path = os.path.join(pkg_dir, "pkginfo")
        self.execute(write_file,
                     (path,
                      self._make_info_file()),
                     "writing '%s'" % path)
        # request script handling
        if self.request==None:
            self.request = self._make_request_script()
        if self.request!="":
            path = os.path.join(pkg_dir, "request")
            self.execute(write_file,
                     (path,
                      self.request),
                     "writing '%s'" % path)

        # Create installation scripts, since compver & depend are
        # user created files, they work just fine as scripts
        self.write_script(os.path.join(pkg_dir, "postinstall"),
                 'postinstall',DEFAULT_POSTINSTALL)
        self.write_script(os.path.join(pkg_dir, "preinstall"),
                 'preinstall',None)
        self.write_script(os.path.join(pkg_dir, "preremove"),
                 'preremove',DEFAULT_PREREMOVE)
        self.write_script(os.path.join(pkg_dir, "postremove"),
                 'postremove',None)
        self.write_script(os.path.join(pkg_dir, "compver"),
                 'compver',None)
        self.write_script(os.path.join(pkg_dir, "depend"),
                 'depend',None)

        log.info('Creating prototype file')
        path = os.path.join(pkg_dir, "prototype")
        self.execute(write_file,
                     (path,
                      self._make_prototype()),
                     "writing '%s'" % path)


        if self.control_only: # stop if requested
            return


        log.info('Creating package')
        pkg_cmd = ['pkgmk', '-o', '-f']
        pkg_cmd.append(path)
        pkg_cmd.append('-b')
        pkg_cmd.append(os.environ['PWD'])
        self.spawn(pkg_cmd)
        pkg_cmd = ['pkgtrans', '-s', '/var/spool/pkg']
        path = os.path.join(os.environ['PWD'],pkg_dir,
                           self.get_binary_name() + ".pkg")
        log.info('Transferring package to ' + pkg_dir)
        pkg_cmd.append(path)
        pkg_cmd.append(self.pkg_abrev)
        self.spawn(pkg_cmd)
        os.system("rm -rf /var/spool/pkg/%s" % self.pkg_abrev)


    def run (self):
        if self.subpackages:
            self.subpackages=string.split(self.subpackages,",")
            for pkg in self.subpackages:
                self.make_package(subpackage)
        else:
            self.make_package()
    # run()


    def _make_prototype(self):
        proto_file = ["i pkginfo"]
        if self.request:
            proto_file.extend(['i request'])
        if self.postinstall:
            proto_file.extend(['i postinstall'])
        if self.postremove:
            proto_file.extend(['i postremove'])
        if self.preinstall:
            proto_file.extend(['i preinstall'])
        if self.preremove:
            proto_file.extend(['i preremove'])
        if self.compver:
            proto_file.extend(['i compver'])
        if self.requires:
            proto_file.extend(['i depend'])
        proto_file.extend(['!default 644 root bin'])
        build = self.get_finalized_command('build')

        try:
            self.distribution.packages[0]
            file_list=string.split(
            getoutput("pkgproto %s/%s=%s" % (build.build_lib,
            self.distribution.packages[0],
            self.distribution.packages[0])),"\012")
        except:
            file_list=string.split(
            getoutput("pkgproto %s=" % (build.build_lib)),"\012")
        ownership="%s %s" % (pwd.getpwuid(os.getuid())[0],
            grp.getgrgid(os.getgid())[0])
        for i in range(len(file_list)):
            file_list[i] = string.replace(file_list[i],ownership,"root bin")
        proto_file.extend(file_list)
        return proto_file

    def _make_request_script(self):
        global DEFAULT_REQUEST
        # A little different from other scripts, if we are to automatically
        # relocate to the target site-packages, we have to wrap any provided
        # script with the autorelocation script. If no script is provided,
        # The request script will simply be the autorelocate script
        if self.no_autorelocate==0:
            request=string.split(DEFAULT_REQUEST,"\012")
        else:
            log.info('Creating relocation request script')
        if self.request:
            users_request=self.get_script('request')
            if users_request!=None and users_request!=[]:
                if self.no_autorelocate==0 and users_request[0][0:2]=="#!":
                    users_request.remove(users_request[0])
                for i in users_request:
                    request.append(i)

        if self.no_autorelocate==0:
            request.append("#############################################")
            request.append("#        finalize relocation support        #")
            request.append("#############################################")
            request.append('echo "BASEDIR=\\"${BASEDIR}\\"" >>$1')
        return request

    def _make_info_file(self):
        """Generate the text of a pkgtool info file and return it as a
        list of strings (one per line).
        """
        # definitions and headers
        # PKG must be alphanumeric, < 9 characters
        info_file = [
            'PKG="%s"' % self.pkg_abrev,
            'NAME="%s"' % self.name,
            'VERSION="%s"' % self.version,
            'PSTAMP="%s"' % self.revision,
            ]
        info_file.extend(['VENDOR="%s (%s)"' % (self.distribution.maintainer, \
                self.distribution.license) ])
        info_file.extend(['EMAIL="%s"' % self.distribution.maintainer_email ])

        p = self.distribution.get_platforms()
        if p is None or p==['UNKNOWN']:
            archs=getoutput('uname -p')
        else:
            archs=string.join(self.distribution.get_platforms(),',')
        #else:
            #print "Assuming a sparc architecure"
            #archs='sparc'
        info_file.extend(['ARCH="%s"' % archs ])

        if self.distribution.get_url():
            info_file.extend(['HOTLINE="%s"' % self.distribution.get_url() ])
        if self.classes:
            info_file.extend(['CLASSES="%s"' % self.classes ])
        if self.category:
            info_file.extend(['CATEGORY="%s"' % self.category ])
        site=None
        for i in  sys.path:
            if i[-13:]=="site-packages":
                site=i
                break
        if site:
            info_file.extend(['BASEDIR="%s"' % site ])

        return info_file

    # _make_info_file ()

    def _format_changelog(self, changelog):
        """Format the changelog correctly and convert it to a list of strings
        """
        if not changelog:
            return changelog
        new_changelog = []
        for line in string.split(string.strip(changelog), '\n'):
            line = string.strip(line)
            if line[0] == '*':
                new_changelog.extend(['', line])
            elif line[0] == '-':
                new_changelog.append(line)
            else:
                new_changelog.append('  ' + line)

        # strip trailing newline inserted by first changelog entry
        if not new_changelog[0]:
            del new_changelog[0]

        return new_changelog

    # _format_changelog()

# class bdist_rpm
