"""distutils.command.bdist_wininst

Implements the Distutils 'bdist_wininst' command: create a windows installer
exe-program."""

# This module should be kept compatible with Python 1.5.2.

__revision__ = "$Id$"

import sys, os, string
import base64
from distutils.core import Command
from distutils.util import get_platform
from distutils.dir_util import create_tree, remove_tree
from distutils.errors import *
from distutils.sysconfig import get_python_version
from distutils import log

class bdist_wininst (Command):

    description = "create an executable installer for MS Windows"

    user_options = [('bdist-dir=', None,
                     "temporary directory for creating the distribution"),
                    ('keep-temp', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('target-version=', 'v',
                     "require a specific python version" +
                     " on the target system"),
                    ('no-target-compile', 'c',
                     "do not compile .py to .pyc on the target system"),
                    ('no-target-optimize', 'o',
                     "do not compile .py to .pyo (optimized)"
                     "on the target system"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in"),
                    ('bitmap=', 'b',
                     "bitmap to use for the installer instead of python-powered logo"),
                    ('title=', 't',
                     "title to display on the installer background instead of default"),
                    ('skip-build', None,
                     "skip rebuilding everything (for testing/debugging)"),
                    ('install-script=', None,
                     "basename of installation script to be run after"
                     "installation or before deinstallation"),
                   ]

    boolean_options = ['keep-temp', 'no-target-compile', 'no-target-optimize',
                       'skip-build']

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_temp = 0
        self.no_target_compile = 0
        self.no_target_optimize = 0
        self.target_version = None
        self.dist_dir = None
        self.bitmap = None
        self.title = None
        self.skip_build = 0
        self.install_script = None

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'wininst')
        if not self.target_version:
            self.target_version = ""
        if self.distribution.has_ext_modules():
            short_version = get_python_version()
            if self.target_version and self.target_version != short_version:
                raise DistutilsOptionError, \
                      "target version can only be" + short_version
            self.target_version = short_version

        self.set_undefined_options('bdist', ('dist_dir', 'dist_dir'))

        if self.install_script:
            for script in self.distribution.scripts:
                if self.install_script == os.path.basename(script):
                    break
            else:
                raise DistutilsOptionError, \
                      "install_script '%s' not found in scripts" % \
                      self.install_script
    # finalize_options()


    def run (self):
        if (sys.platform != "win32" and
            (self.distribution.has_ext_modules() or
             self.distribution.has_c_libraries())):
            raise DistutilsPlatformError \
                  ("distribution contains extensions and/or C libraries; "
                   "must be compiled on a Windows 32 platform")

        if not self.skip_build:
            self.run_command('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir
        install.skip_build = self.skip_build
        install.warn_dir = 0

        install_lib = self.reinitialize_command('install_lib')
        # we do not want to include pyc or pyo files
        install_lib.compile = 0
        install_lib.optimize = 0

        # Use a custom scheme for the zip-file, because we have to decide
        # at installation time which scheme to use.
        for key in ('purelib', 'platlib', 'headers', 'scripts', 'data'):
            value = string.upper(key)
            if key == 'headers':
                value = value + '/Include/$dist_name'
            setattr(install,
                    'install_' + key,
                    value)

        log.info("installing to %s", self.bdist_dir)
        install.ensure_finalized()

        # avoid warning of 'install_lib' about installing
        # into a directory not in sys.path
        sys.path.insert(0, os.path.join(self.bdist_dir, 'PURELIB'))

        install.run()

        del sys.path[0]

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        from tempfile import mktemp
        archive_basename = mktemp()
        fullname = self.distribution.get_fullname()
        arcname = self.make_archive(archive_basename, "zip",
                                    root_dir=self.bdist_dir)
        # create an exe containing the zip-file
        self.create_exe(arcname, fullname, self.bitmap)
        # remove the zip-file again
        log.debug("removing temporary file '%s'", arcname)
        os.remove(arcname)

        if not self.keep_temp:
            remove_tree(self.bdist_dir, dry_run=self.dry_run)

    # run()

    def get_inidata (self):
        # Return data describing the installation.

        lines = []
        metadata = self.distribution.metadata

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        lines.append("[metadata]")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = (metadata.long_description or '') + '\n'

        for name in ["author", "author_email", "description", "maintainer",
                     "maintainer_email", "name", "url", "version"]:
            data = getattr(metadata, name, "")
            if data:
                info = info + ("\n    %s: %s" % \
                               (string.capitalize(name), data))
                lines.append("%s=%s" % (name, repr(data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append("\n[Setup]")
        if self.install_script:
            lines.append("install_script=%s" % self.install_script)
        lines.append("info=%s" % repr(info)[1:-1])
        lines.append("target_compile=%d" % (not self.no_target_compile))
        lines.append("target_optimize=%d" % (not self.no_target_optimize))
        if self.target_version:
            lines.append("target_version=%s" % self.target_version)

        title = self.title or self.distribution.get_fullname()
        lines.append("title=%s" % repr(title)[1:-1])
        import time
        import distutils
        build_info = "Build %s with distutils-%s" % \
                     (time.ctime(time.time()), distutils.__version__)
        lines.append("build_info=%s" % build_info)
        return string.join(lines, "\n")

    # get_inidata()

    def create_exe (self, arcname, fullname, bitmap=None):
        import struct

        self.mkpath(self.dist_dir)

        cfgdata = self.get_inidata()

        if self.target_version:
            # if we create an installer for a specific python version,
            # it's better to include this in the name
            installer_name = os.path.join(self.dist_dir,
                                          "%s.win32-py%s.exe" %
                                           (fullname, self.target_version))
        else:
            installer_name = os.path.join(self.dist_dir,
                                          "%s.win32.exe" % fullname)
        self.announce("creating %s" % installer_name)

        if bitmap:
            bitmapdata = open(bitmap, "rb").read()
            bitmaplen = len(bitmapdata)
        else:
            bitmaplen = 0

        file = open(installer_name, "wb")
        file.write(self.get_exe_bytes())
        if bitmap:
            file.write(bitmapdata)

        file.write(cfgdata)
        header = struct.pack("<iii",
                             0x1234567A,       # tag
                             len(cfgdata),     # length
                             bitmaplen,        # number of bytes in bitmap
                             )
        file.write(header)
        file.write(open(arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        return base64.decodestring(EXEDATA)
# class bdist_wininst

if __name__ == '__main__':
    # recreate EXEDATA from wininst.exe by rewriting this file

    # If you want to do this at home, you should:
    # - checkout the *distutils* source code
    #   (see also http://sourceforge.net/cvs/?group_id=5470)
    #   by doing:
    #     cvs -d:pserver:anonymous@cvs.python.sourceforge.net:/cvsroot/python login
    #   and
    #     cvs -z3 -d:pserver:anonymous@cvs.python.sourceforge.net:/cvsroot/python co distutils
    # - Built wininst.exe from the MSVC project file distutils/misc/wininst.dsw
    # - Execute this file (distutils/distutils/command/bdist_wininst.py)

    import re
    moddata = open("bdist_wininst.py", "r").read()
    exedata = open("../../misc/wininst.exe", "rb").read()
    print "wininst.exe length is %d bytes" % len(exedata)
    print "wininst.exe encoded length is %d bytes" % len(base64.encodestring(exedata))
    exp = re.compile('EXE'+'DATA = """\\\\(\n.*)*\n"""', re.M)
    data = exp.sub('EXE' + 'DATA = """\\\\\n%s"""' %
                    base64.encodestring(exedata), moddata)
    open("bdist_wininst.py", "w").write(data)
    print "bdist_wininst.py recreated"

EXEDATA = """\
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAA8AAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAAAjSEomZykkdWcpJHVnKSR1HDUodWQpJHUINi51bCkkdeQ1KnVlKSR1CDYg
dWUpJHVnKSR1aCkkdWcpJXXuKSR1BTY3dWwpJHVhCi51ZSkkdaAvInVmKSR1UmljaGcpJHUAAAAA
AAAAAAAAAAAAAAAAUEUAAEwBAwCllso9AAAAAAAAAADgAA8BCwEGAABQAAAAEAAAALAAAAAHAQAA
wAAAABABAAAAQAAAEAAAAAIAAAQAAAAAAAAABAAAAAAAAAAAIAEAAAQAAAAAAAACAAAAAAAQAAAQ
AAAAABAAABAAAAAAAAAQAAAAAAAAAAAAAAAwEQEAoAEAAAAQAQAwAQAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABVUFgwAAAAAACwAAAAEAAAAAAAAAAEAAAA
AAAAAAAAAAAAAACAAADgVVBYMQAAAAAAUAAAAMAAAABKAAAABAAAAAAAAAAAAAAAAAAAQAAA4C5y
c3JjAAAAABAAAAAQAQAABAAAAE4AAAAAAAAAAAAAAAAAAEAAAMAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgAkSW5mbzogVGhpcyBmaWxlIGlz
IHBhY2tlZCB3aXRoIHRoZSBVUFggZXhlY3V0YWJsZSBwYWNrZXIgaHR0cDovL3VweC50c3gub3Jn
ICQKACRJZDogVVBYIDEuMDEgQ29weXJpZ2h0IChDKSAxOTk2LTIwMDAgdGhlIFVQWCBUZWFtLiBB
bGwgUmlnaHRzIFJlc2VydmVkLiAkCgBVUFghDAkCCuNX/7Q27V5F5+gAAPhGAAAA4AAAJgEAFf/b
//9TVVaLdCQUhfZXdH2LbCQci3wMgD4AdHBqXFb/5vZv/xVMcUAAi/BZHVl0X4AmAFcRzHD9v/n+
2IP7/3Unag/IhcB1E4XtdA9XaBCQ/d/+vw1qBf/Vg8QM6wdXagEJWVn2wxB1HGi3ABOyna0ALcQp
Dcb3/3/7BlxGdYssWF9eXVvDVYvsg+wMU1ZXiz2oLe/uf3cz9rs5wDl1CHUHx0UIAQxWaIBMsf9v
bxFWVlMFDP/Xg/j/iUX8D4WIY26+vZmsEQN1GyEg/3UQ6Bf/b7s31wBopw+EA0HrsR9QdAmPbduz
UI/rL1wgGOpTDGoCrM2W7f9VIPDALmcQZronYy91JS67aFTH6Xbf891TAes7B1kO8yR0Cq3QHvkT
A41F9G4GAgx7n4UYQrB9/BIDvO7NNEjQNBR1CQvYlgbTfTN/DlZqBFYQ1BD7GlyE2HyJfg9hOIKz
3drmPOsmpSsCUyq8+b5tW1OnCCWLBDvGdRcnEMKGNuEoco4KM8BsC+3/5FvJOIN9EAhTi10IaUOS
druwffI4k8jdUOjIVyJFsnzb3AwvUMgIFEBqAcz+c7ftGF4G2CVoqFEq8VCJXdS/sHDrLS0bfRw7
dGn/dChQaO72+b6QmBlLBC7sjnQTGnOd+5YNfIsEyYr2IR8byFn3LXw6Lh9kQ+2w0VoDxUUSPsgP
3ea+U5ccGY1e8KQUxuPO8GHOgewo4auLVRBExv/tf4tMAvqNXALqV5/gK0MMK8GD6BaLG//L7f/P
gTtQSwUGiX3o8GsCg2UUAGaDewoA/5v77g+OYA7rCYtN7D/M6ItEESqNNBEDttkubzP6gT4BAjA6
gT8Lv3Wf7QMEPC7BLpSJMQPKD79WbY/dth4I9AZOIAwcA1UV0W370rwITxyJwVcaA9CbEBYjNP72
6I1EAipF3I2F2P6baTShezdgCy7dgLwF1w9cMseY4VkYaLATHShPFz4bMyvtoGX4hoQFtrRhexFS
5PaDOMA+Cn5jL9vwDfzw/zBSUAp19J8ls9QN1kgPEMoAds79Dy38/0X4g8AILzU9dcixzs3eq0ca
UGU0aFgzwwbysgywBXitsO4bbpp0SqZmi0YMUAQOQ2uuseF2ueRQVCyrR/4a3EclIicbCBt2FFEw
z/bbDdxKAfqZGNLu7WPbmRjJFXlQKUMKUEPt9tzGagbBGLwPtRQ5Aqnguu4PjE1h6ZFw+7qmgLnH
fjTIjRzIlv9zBNSoZr+52yg3XxrwJvQDyCvYGZvkEBaz/HYQKt4ugXAAL1mNTwT72/aKCID5MwUE
L3UCQEtT9naGpSA3W+A6oQQsbjxel3jrA+quXCQE8X/fGgcRO4TJdAs6A8YAXEB17/D6QunIFEBo
cJNAZVg3ELl9iMtpAl3DVpOtbLt6CL+Fth2EjZABsF03AbW6Hyg4XIM9pBkACY8s4G40aMxE4Hh0
M2jZXte5tA7PVA+jJGj8vw/2AHRZQ3UVaJwdNaz3L5SxzGvJKes7M/++HJE0CJiFjXM2HURtx3a7
/ymDxghHgf5sGnLjHGiIOxTL3RCsIbQ1BP0Yds+CyyWfY/hqpaS292bBRhYSl99zd+xk38lPdIvr
ru4sAucsy/YtWFaJdfAC7Oj0frnBsvz4pzByO8Z9C1C22xFYld8IZe7oUAPsaJqmafTw4Nzkx8eE
XAYElSS/OuncBt2HI8h0rQFNuAeOICfZWjjglD45W9j9jVX4UmjYIIsIYxEfsJz9HAH4GVFQGpDI
yciE3BxsZmTsdjnXdBgf8CyauG2WCEjrchzsdCDoe05Gxh/sRCBSPDwZG2T09Bwk9JMKrrnUNQHU
/QTgfM4CDboe4HqGfGZse7dFjZUb7lI2GAYXMpY52AcnzsvcWAcNBiUIaAytazILIjQnJCZQE8CY
zWYfCBu9tuUgcFle9QAEU3ZbPSg7FnEQFwD88ppap+kVg3Qa3UhnZGcWlwEG6QCbuLkt23R7Al4h
D4X+oaTKkYMs8U3kmWSVcXeD9gmsD7deVDXsT8HgENlwOy6RfRR9aAFlqowI2Bbu1i7cAhDN4mzX
sjkKDpNQns92L343CjwpGWowG2hcbDO2c2Ug6HEG5Otziidw9h10EoZIIHCJavuQpQd2QWxHaEQf
/bnf15I6EHU2H2gT65bC3vqCfQ3QrdjrG1ctXCdkMIvDmku2LWG2CKrgjSQR8BzfdvfNHIkGoay2
bYlGBAM1Cl63cGEJfLBWOWK+CuFiYeF0NYJNy1HAQ2ge2jMAEZVcVphs4SK7CG3scAzvWCWDF/sG
iB0j2Un3OdidK/DaElaifAuvxbMjahBGuWmALV53ODGFXrwYhb7W7jk1qJ90PYwHAikTXMfGaiSw
ljuMMptvBT56MSoGgQQHdc0X74bYxwUq8+vBiT5WX07INeBRJkAMD3QXWrHJRlUUuawx9u0xGQv4
U9xFwFdQFPzfNDT0btgrXZ/4agqZWfe526z7+TPJaBiBUQAeaLxri8enzgnXMDw9RBrRMFtDbdca
wBQVQLZjYFhQuDiDWVDYDwwfLtQBqR08GtNoSq8d2LN1OHAjCgEVnOkevdOpt180nwutYc6e8JXX
5sKj+7bdEADauAAGAD1M4U71LZm2ZpSBCRAnD6zGdztOcOgIVyEM0qG4zG4Uw40xdPhyIv3/jJ17
v1UGxHGDDR7HBCTAjGw6HGTc8CSWKIFJWDIvP8jG8KzWsNT36ANB1m65n/0MNoxsDiDLAARfhOsn
a/TxLgaBeAg4QBnqfrI1e65wHdF0NwRbcAsv1fWwp/5CCFad42e4dTlWyCnY0C70tzehHUCLUAqN
SA6RUVJ7FhqNaqxGSKMwBsEs3SwM1PwQEZ5BGt7w4DXX1BoVTN+sUAVvTbVoLVEh9BEr0Ct/oW9b
rVIP+CtV8PJSmSvC0fhmG7VDehVDxw3lFpERhdwEg3zbFbr4An4GuOhPw64OCAIMPgh/H30FuLgT
uAwRnouEJBTQjQrgC7LGTlf9sLu5u+UCtCQgE4stfy3CAJvO0NX0K8YVRS4ov/4Q2Gx3C2Y7xxQA
wegDpCFy4enPEOzOEMcMLyQL1btwfzpTaOEgC3dmgFdW2/QQ+7xdGyCbFgFGSItri60zGdVYieIQ
ZG2iqVJe1EixaIbu3bWCGWR0NIA9ZbGS8WulU+PGfTyXJnAzTOdMFRwhaTK20MY3fFHPQ0ApBpCT
HCD4luu61+UWSsgXMLgeSeToZjwfwx3EA0lf07kgY8FDw4i7aykr3MQUdU7oKEGRjS3w4L+O9O+j
00XKHbII8KCb2SI4CNIKyYN4g92RwHZ2GGiZeLvbINZ6Pg41KlNfBZnsMAMpisHABEocq/RvLYKM
HVpQHonDOtcMT0qk6Ib+GxwF3pzZL+SL3QJ1Hw4hZsaeEDUikK5EcBboDhAwEKPwgbVlr0DrzZc7
KrSlp/Z9DY2Dj39IdbPSCihZFIB8BBcTUt1SIhETGD3/Fr5g3XcEExwOClnx8JBMx0vrS8ks/YQb
BeM79EpXV6dhw2l2aP4tA691BKvCmnBr6yADV2AfOu1qQWP5gcRUAoeWzv6B3AKpwseCg+0z23cZ
AGvUhqRGYAG9/FwcADr6De0HNrgSUVdqUF8DU40MNOQA+5ivMM4Cke2JffQnDDHydVE+tB88i0Xa
wyUKXMgfTTgYNBae2ZbEmFGhK6VEno3xjLZorP3WEUj/uQowaYASf8DabLZG7KAcf58zF3h0dV2c
UfD9m++dLJsa8AA5XhbwIghsTgExwe4kgm1j6C/eWU/OjA3s6GiaIkUQUVk7FXv08brz8K9erDuE
FoteESncecmtBlm1BBAMEOtnuQHU86h2LfECr1twuKZNKgjV1yQqdGbhkD/T6xAonB02wdkd7Bgg
Nrjf4yBmFnJ3nxSzJ7BDwSWWRevRwhJyWPMaJLB+xGBouahQmE8L2cuVCY2AeOOGH/mxi4SLQAg9
MRF0LT2sYEly29pPYe2dZpsljJI9GBgMiMWnvfRViZ94jLRQd7ifClzeDTNVHDtng1UcoBQWtDsl
mWNRhOGTWb+yLVfA0PhfzhP7bWISgWTDGCeAyQWgwS/35LdebgLJTTaXMBn1sj7EtqO0lWh16km7
9aGYdHDf7EL+QgwOkqSTwGTwU/cUauPA76xyFMBDYFU9GCKVLY2yO+KXSGAFaF2qhfYxPEZs6oDZ
VkMIXcQEEAPaK4Yy1pzbW+qFrAqZW1Qwpjhdewf3CSQXc9KcbAoQ+AD8qBFlZPCY9G3iu2rF3UeF
SjvedEMaLG+3fnQ+BPh0Ofy3FuqqAR0vsSvSObZGu6Y/4CsPlTNbVVO7MdoC0xr8sBVtTeCNRtvw
FPhHhoPI/51ybJ30blAQSKoZWjsSanIaR7na6sf/QARB6/ZjwVseGzwaDYequ8CfHoQxoFMX11a9
H1ZVFGK7ZBBBFIuxLf9EJG4BkQD6nvfYG8CDStBwi+BTwGOPGG8CSd7hBdUgaFyZorgf4T7/lCR0
LxN0BCYJu8UO9Co0aFQsGbhmwSxQe5OH0hLcsogswBo3wb19i3YElHWEi1Kz/dBsOKFTHLAgcjO1
BAUaL8lIV8xmgs7qgB9TPsyHTubOCSQlIuvbNlc28wTCHBT8QgRi8XgEvqiR3K4Wb9CqxfSKDwV1
HIB3gxL5C9Sak2Y7Aktd0BmNGcCzcJqwVhq+/4KkGVngVVPFn+Sa5kMc7NcNqJoQeHF7zy6ayvgX
nHGYuw/B3Nbs0wU0eC04Boh0hgUCKZR1RnuuZhfIRixQbiHYO2hsrxFrSCOzbSCV/drpYI/aC8wG
HRSZ02DWmg0hBlAAMCucbHCtwxQbsBVYSG7Aa0iaJxas8NMQoH3wATE95NUiId2RijAEMGExe5Oz
OiBrDYmBHvZhiWxuhN4QQBQjkvRINgeyGP78mdkZLNg3shSP3DC75NyZViWkaMyZzSV5IJ7ImXou
apv1wXzL8uzoF5DmXAvGg0D9c4tHfQD7XVZLnJnLIEC+jCbIBuwX0FbMVtjgFMDIT8Nb6ZIN8NyU
FBFW1pNtCHB+ApUMJOSAm80ABIWJvmC4rl33Pl7h/z0PKPYuU7fAqUU5ivycBBauxQMJ6OLf921u
4/DHMTATCfjp7LjPJhSjdLsQJJyC2ckgKFYMvlsL90Cy3PWNjRhRWbBAjBbGnEyN8dl4EFPmuA2o
7KAL7anwFcFFOlFJjgxCo4sYUKMUsDTcekPSm6GhNb0gUEgGGTNlKQzsWerE0vR/85YYdiCTGxwF
dfrWsaJBbprazvbsNAZVPDIdagJgNihSu9QcUxCVUBDiBsF47LybGkbsYBB+6xYfI6xBsNibJB+d
FTLXzECIO8u/e7jXQV6YssWQitGZwIVGdoDu0UaaTC6EjBSc92asmI9OcSiApDUmdEJdsC3+A6PX
9zSbsMeb0g7D08mx2FFoHjCCS+t06hatgwZjXBXi6zeIxVss5+e0zMDkKzUT4Ufs9GK2nbhWLwAq
pZADCQw1fwTkkGiMv5IgZ5PECWydclgK7HfSXRKe6BVA5Ao0J81JKPgQ8ADxkE2cCN/wnNyckTjp
Jr4KzODJy2aWChIFQwjsOrKMLCPoMeQoMMvIMvgf8BaBMrKV8xAO9K7M1tz+i03gO84b+gDGB/Ir
jhUuE0WJHYkVHXS7i10MNYkNAKPLdJvN4xvJgDsIwI4d2wldlFP9WUJZ+nUD6WOMHx9TZ6A1wt2w
RN3pxCpojBUrtUVKUU/8xxji/faFV/hK/nQ/aHTwdqKv4HxbCKM0DXJtoI7No3O+aGrBu4X6qVXH
Nifwg8YQekb82jOB/oNy5dFV7O+LRq/mTLLkCktduBiHKL4HI+tBk1SOAlJmDHFyGIzeVvI1/OS5
wczrBfJ8nfRiy+CFEezyrthEs0w2XnQbXsYAfndZyQp0agtZidqFN1eNfcTO86sGpWO/Wnrwq6vk
ZG0MqxqQuWixbROMG7+WEOKrHW7VwDCWLwHIKQqwNfcc3JBd6tZKmxvY5wcGzGvnTOF4KNZQFysS
S8buTGwgoh1AGfRyycmTbVgi+G7bOdp5lSmf13+MNFyurUHdfJh1BZRq22+3bAWsjH+QIJt1tAK8
5QO4LagPpAS1JEdJighcjR6v++IFrDWozIlwHr0Z45vgV4FVu4VQU74cnh/QkIGuFARWv2FqHNWd
MBqbPiBxZQtw9hNBVfwMKNmwU7QObCcjTbZTtD10KAC3dICDey1s1WqdFZDu1cnVoxcU4Bx5M5XZ
RQpocNCdZjv0lltSFciLhrkCVQQZRrDJt8XJuDKsw7MrA/0hZVCj2kMemN4DMAm58KE6nO0bjlAx
W3QHUBNoFKtvAtcPNCW2AUxubQAQxVYaqlSq9Vd0b4lauPtL9Z9xZoP/AnZhtnVOikgBQAh/eXl7
MHxKBDN+Hm50DHJ1O0DGBgb9LfYNRuszBgMKRk9P6yfa/n7S9GoIUay0PAp1BR9PiAbVwb8Z7Qbr
BYgORkBPqZkYibcru2uAJqjTKB+FU0na+NXBVtLxkRtE2APcJRtAGeSUAcfD4AN/yhAGrjNom9Vo
TcNqeAxzTJ7Yu1yqUJqtBY2yrCUIZmQ+Fuh3F7pQGTDEMQpIHMjY7zIMGPDUntVWc9WgR7Zv+FZq
ETErzZorLwQLLeKuutazRFgM8JkKECQkYGcGFHJ4zpLJBDqs8aQz20Ic1lBTrowUqUlzO1B+oP2o
Xop1MtgRpHhJexAr8RIv2SY7VbL2Pb+MEFfDEYK92Svl4BBFHbzwZITV/GpEJaheVoUwMEaOIhEl
CbrmSHbUAVAcUFcWbrPbt1NTRCpTZk3Y+LaThdOIQ4+ESPFBr+2GC9ZqDxiASLLEG8muuLQNsrhn
c9827CzYCNYsE3QJw0N4NSNRUR8xuaYYGxzoTMtb5WzwRQdB2N7bLYv9L+DeQ2pdUxLehf9ZdH2A
JwBHwD94LVZXXzp0A4Agy6pN6mnpuPCjXqx+rxhWz2VKOXu4Ay14IyUWkaxWcrCeBACwAw/O8Ia7
GSi55D1sBG0EQibI0Tr/soITzsx1Al7DoYQaKv13DkaDOAF+EA++BtHFvO/MRkfrEa2wFYsJigTA
ztr0QYPgCFPQVmReTyVfkYGQGBSWhTeEWd4028PZTShcEUBTaW8787rwY0WMJyKIHkYMoLUDnaz4
NvxWhEr56vW8VFPd8PSe9EMZGeRAhzW/V5AcpJxrh1NTySDfIwApDOy3SUUYe2OndDbK0MIVkzmZ
QuqhSV89FHVAAKkgFV9Hmq3BGggr+KLsV1KTVKRMRldEv9+TXAiInDUqOJ0FX3SScbreGlM6euBJ
zKAIQVTiIA88XPZHwBgBB250g9QDV3sLXelyN1X8GyeuCQYGYFM78KC+bYQGucIMIwninsvCM5Ye
2FQeCASwJAdsDdwLCbyH/F2yEWoQVmh8oOsoy2D0PXrfSlEITBaZrnDwUlVk3Fj0EWgOKttqKBuw
ex+jKnULaFQiGSlzMEdgFxcdOILo704iCnBBMyezBDN3DJIIKAYffMQojANZWejnFZADw72ENeca
7Ba9tYzZGvG1TuvEDBr2XMhLUnc1+EvBsokEj0E7Texrbc69CXweg3bsBujzIWe2WWY8TJS/toXW
DjCwhfcAk+JWTXAd+X3kOgzoCg9DoEVAjsKJG0AERKBKKBBbAVj/PNPcpw59M/hX345AD4aFngX3
5ALn+F/PBqS7Xx7/MFNkDTOLGFMLh2DMzPaf5fTh96dLO8d1RS4lLvMb/8DU4aEJj0eAjCygkCwA
ohNahNlaL9+6usAim9kIsTH8XgNrBOTcwXjL8zmQxBAk3qL0z4Ec2OtlKbAII31KDuxm60IhnCQi
6yCOHMiB5ogHflmcCxcWsPhtTfCK0f7vBMZw+6RotQWIJYSgvRQR1wXVOVMDUiBzYO99HrB4yatQ
aDSfoRTrG7CkO9kf7J0cEEmqg/HUQBD8ntwJhIN2KiJCJ2c4oXSFuwUMwFn44hQXxVe+ZaZWy0Sn
LjiMWQmqagge/Su3WaNsRg4oPZ82nKGGrHB7lNrxe4/wKcAsL3+jqFroXTB5EB+N62FMLK8ERPpG
1yXdDCqajrUwDn0nJiU/+x9CcF26H0B0HGoGaFhnfQLukbsWYgdoLBAw3sgSUGgIoTQxGgkOhmNR
SFOFZhSompCNNNYBXimo9IOOwsQWiCtvYU7wUZSUJVY8fNDKKFTwGxoI0lFJBEnXoa1Kpjyixwjs
AtwPAzeLVQgaf3uh2PlMYStBEAIMg+gigTkBGzcUW400EAjDbTLbd0k+elY0Egu3p4yvf6lWB55O
EH8Ni9YrVgQlYhPwK9GJFY4rRrqq39rYELtX/gyAiQErfgSjFm7J2LF0d+f8xyXcycZUnFLqI+yE
DWYWjZg/mBu05prqNgV2yCLyW6mCRXaLsXTBKBq4Lhf1RmyP4gHqRWPrGAvDba54RaBGt/+PN8wA
SzPSO8JWdDOLSE7KdNX/y/YsiVAUAggYi3EM994b9lKD5kHsbr+BiTGLQBwgFFFCM0xc0Wrhu7IE
12HRAA37CJAAfQgLDcUbjjqLRhMzGiQ+Rbc1LD0UDQpsQWy31JvbPwgeGihQUY0kDUcAc0vHAABU
5a35KtFWUU73igFfC5uKDaY6wYLnXnx7F4ftJBg4CtyFxTv3dQo/NX2LEVFkIIl+GNIKrfFdvGAg
8FI7fig5fiSHDrhWdOUkEFOBaqSE0rXNVUMniYY+/OkLv01MJYl4FItWF8+Jegz/vf37XbT32cdA
DAF4+Qh8WQQPf1SFrf3XH7gR0+CJShBS11E3uE/b/9ob0lD30oHiwFFlUoEzzBkq/tdVeEFPVjl6
FHUPCsVb9iNuDk/Ck83GC1YbyV+4+mk8T1gyEHFTVRBCCbaqiQR6djS33S0K+QOhPgAT8ANUb6rf
KCNVg/oEv/vBlcNLvd8e2r8FweP7iVwZiQjIDQ+HxBUe3hCZJI0QQxkEtjvaRrM9iEkeiQ3iQYvx
bXxrLwWLDooRHAQ1Fuf+hb8QBIPhD0KACokWdBXHAA1V3btL5gVsGPChdeuiIotQDuzG7RDB6SjB
CF12GCTcr7UdTBUvLhcFvdsVbp4EEUgzyY5mCEB29rg1fYteHIlOBom9HwMT/0u80Yl7QwTBhwPB
9/WF0nQhx+li7r0DVpTR3V/Em9v45Gj2wSAlgWMpB+WIDTsmHNhWwfWjdNo0fGKkZUMh2Pf9dRij
AlXzWm1taTAshV8CkiIBwaU2u09pAnOgM41IzblIS3NSHhJEVPLNto4M+QvYDDnjCF4wZ14tAmPk
7XONt+bhStzB4RhIC+S+LtnaSTQJ+E1WKIy1G4NIQokGOhwUAftbV5CBSDfiEAPKiUg5kovkkgq+
CGZLLhkLhDZlDjfmPzlINBI2YDZDhOvlM1lACMiB6aykpvoh22gCdQmLx1GDc27ZwginZ3JqY53J
LLSkFlBHbsclhAfYAQM5FkhPs2VpuDeKChtQ4dE+JAfIkVYCBA7YIYTJ0iCJKLOEkBJGIR/sNduF
eE4w8wa4+DthGpaRaSycy2azgnAAJWqW5NsKAP0MQwEp/WJuyf0GOAtHP9ksuwIwA7RB7i1C0yyb
Zmi0NUCi18ssm2URQUvsQvhbiweSwH/TV/OiAofbejyJQ3SN9lTbWAQP2Q6+628d2OBHKFKpV8p1
BnUNO7KBXT5XUepLDCjH8gS27cYBRjQCMA447hV8thZRCCB0DrZb68KitdAfYEcwwMOir0Cy3/xt
akWJzhWqZGMgwQs5KPFM9tvECk/XCEerKNpJwRqCocABX9mXehiDWelXKIyQ7cMGM0D3ckBTUygo
udM5aB+fK1EeDRLWwC6iNgKFty4B9QPYHoleLLyxGBKzOMgEQO5DL3uqAIPsmDhTb7YGWos4WPsp
Q7JrXMCtNRJILks0R+/aFt8QMFY7yLNUChVEcwUrL+Da8sFI6wUsBx6MA4P4DQ4+lwkZDIUsQH4Y
XJED+IP9A3M8ddC2b7ielg3G5EiKD8cUTK77+/+Ui9GLzdPig8UIYwvyRzGJOG/V3ruJL3LO6wQ3
r7oHi8jdN7XA0ei1AZeJSxh3kWNUb88CN4PtAxkBzRwHwe7oYNP7A9PuK+k/szS+QUi3hbYJPVKN
sISNDTBRXcMndg44Us5SHCRcITTi7Tp6+NxRDyxSEBXoPlDeEEMcFImuZuw587XvXFhxBjfkwGJh
FAP4Xbx1h/1YFM4gcyyp+i3QcG76oAY/TCxPm8E9l/Z8QCcA8tRXauJCjYvOguEHcrf/FnjqEDPR
r6I47YvBO8X6BInYQbq1bFxLJgGLiQPpdG5bYkzSF7wqxxy+a4dNBYWdFnwaRDvWdSNrvKsbv4t7
KLAZi9c7sRXbLhS+cwcrwkhXZCvyc4k1oULXHXVntExBSAT6YmvtcFM0KA4HRzDD26z7atajTDox
K8pJ/+/2HG1LLAcEPlV1IGLcfJCR99byTovOwovImHBnm6ResAveYKFhBcl2ncI7wQqtaegFwT4U
RDAk/oL7JYEC86WLyi2N4QMr0POktrc7PNpcJUQDUg1LM1270V0V8CsMFol4NteCoRwpAWhdZBjG
EEYIfwcqllfJgTwOczgyDtF9yBiS0iX/PyXIvmWLzSCYH4cdBtbQbu6OhTzgCIH6oAUT8gXfYGso
6wV9H0aNhAgClm7OUH93A0go+VCN57NxYQyNBQ5I3mcB8w7HQwhKA+sIrkY3msZxU5IIEQqDYvzO
04Utc2hZMr40BmlhMpUDLAhODjLREbGL/DsabD9a7cUEkWEICO5hrtADhmpncpgwbbL3w7gTochz
ITw0xzG6ucJ7aTWgNyBy31j60nZwGiRvQxCNU1FSnJozNDRX8eNyu0ZwaipR/PCFIbCQbTP7COYF
BcOHr09l0DTiHzc13068nQJdD4N70lk76HMz1t6PR+NKOwXr+vkIzb3XSpj29PkHLh1rbvou+c2L
yUi1ufYO/rkUI8bmVMEBjeY0drfVoHa0VRCXNHMbyVhwre0r6tEMRYQSit9th2txQKQ3OoAjErnN
dMTj8YUDAfKD6BLNWcP+krsrJPgLH8ALO+lzO5kD6xbI4AQfMJ3n2qOR6cnsfHdVtdfod4sMjakj
ziYOFKnxXqti1JAb1xVP5zLCHOGMCh4D5V7v1tA7KoepddMqCzVKLDkQ6ZnwguGbi7mTFQ3aHYr8
6wIAe/j2UqgMQUiZj/x19XeJXmXiQEJ6goWY0we67RVAJCZRUECN32sdmzEJLCRRElI8NriKv4Y7
P1FCBd4NRDYKa88UZQlZdphnB0AGD1CMJLmTpscfFUwkClybfTQZCCU0z3c9YPueBp88ICsceVAs
zx1DpE6EVwQEBuABtoUpSA9zbNFaWF5rPDCX2ASLr1td0CudOANWTOgNWs3Bzk3u52yNTn1RXEmx
e0DsSjTzdFZdtlQAeMBFm+cnTT7gzCBRDSMYsQTSxKEpzCEY2cB8rYnSACwAbRzMJaGdz4smaNd2
W6+altrplUxRd4VLAq252hewkKFt2O2QMwYww+BRvJk2Dlxh/cszGGyivz03az9VUfLk12r9K9FX
MtgPwwPqUE5LTA3L9mSNMYtpOVHQKwHtFmFzZpLqLxVSUTpbmyWQQ4UyasestbfsQRhMg0tGQEjC
EOZ+SFGJeQRGRBgR0SYcOEsg6LOsCLMIrvKEp4QVeySwN1LIxlTKz2fAxcQAzjlBt6DQiQSTitT3
AxDcM7jug1FP0VhDSzAluEUTED6EBZ/Pnmr8UJRKGgoheZAoSCi8Q4zPK457I4EUGJ39dQZbsodR
NqVPUahkHYMtOtciaJQIW0aEFHyerGtVxruRUt3NIBy4UAY1zwxJcC8h2v6BBEOskP1fJLCFdC5M
EOwoCyQcGFKEPm+ksEcJO1xIUFK9ZyslpgcMQOju8HqmZudBUFZT9x5ZTnRLU9F0N6F7v32ENugg
Ny6JVgR/UCvVi26VbEuVCONufT7GOEC+ZggYMUOtFr5HLovHTFZVxWMhTbYGQ0tWmR1C0ks7nZiE
MCEgoJcNIcgkMBiRU09rrH01sP5FQ0gq7cZT2EP/1EQUzUUDXC6Xy0BGa0caSAFJUEuWzXK5909e
UKI4RVaKGbBpuDlMG1JkgEvvDKIpigIc4FZXGEcFeApXWGndi1jO7zXKRigYDRgIVwI7wAFj6U8j
BqnGt7vv3Q3wGXB1CuzCDABbGI5fLgCdhu9VgfuwFZk/7uL3w3IFuAgr2IIPjKGt6MHEb9GK7dth
EIoWg8bI2bsoG6xW8QP5CPIhhxxy8/T1hxxyyPb3+Pkccsgh+vv8c8ghh/3+/1CCDbYDTbxkn9q2
zgVZFRYSRhNIjd1to5nBDbnx8vfxTG617b6/CIs19/fri/WHE+EFaOsxXRdbTF8LwQgEPybBn5UI
UOYWQtluWOBQHxvR8S67Gld8BMMPHxw1dkuqoTeFIopPwDvuCqNFiFAQWgyISBGAO+iNdQAAD0gY
w/CKh8PfFH8gdiyYMLTOA0aS8FZctCVoyNpuDMFNuFrYDDTBfsW8EPpg+zTCRiwHiTNNr7gBBTrf
/gZsWujQxuhaTz0cGp1y1nYEzhAKCpJsKFwtfzBGeiyJfjuMammBrSkrInut+aqlUtuFiQZl3FUN
O7qV2JRWUiJNEU9V3VPHgBB3U3zqyKN+M10rmRy4SJ0oDeQzFK5Aru6jMOj/Gg1ypXQTSffZG8n3
i63eGQKDwe9NYUOdZqVaiT5jELsStqu3xopFskVY+HNEQJ6LFQ9cBLoOtQV4xS7tMACyjnPuS+7P
0+DQAMcIC8g2eeBno7v2LEE/CixyvK6FjC3VffgjIAhWyEkY4dGvFM4U0+i4bgsu/RrBRSv4QIoB
xdNskXgWi0mPlQgG0V3oMa+oEHTV4A+ui0m6WCuvBSIfAhy07bZAr0XDqCAH4yekc0POHweC2kLY
e585Gq9I3HnQR/INC+fYCL5738zJiwRMuU0EA8jOrWa61lqRsNRyA9cMzW1t00AY9UXMIjkkGGVe
lgOYhCWMRGQMBcMBaUQEhfBSZYAQhJsMjQzBiEFDhgB52AIMCgzkkAwFb9iAQwF+A2sVk3o34NV1
A8IrN0AQ7TlT1h/tI/lRDa+WsVoB0IWX2T5V4iwtjnUhPjCp1KC0O8ERVC0piLA9OAz7COsPShtx
6n9nhhRShTMy0iRyYjwM5AaSIW1iXeyQG+xjYSJej2IYIW6JntsBkEI59/dJ8wmISv8RQUg7UAh0
PPdF1wdODGZJ0mBgc2HPKDewAFGBAhvjYO+T8eBNCogKQkhEvU/AFWH2zxSLK47RLQMK4sdDHyvN
kDUDBRMXEarNdCeJ9BTDSgkwwBsg8Bgwo0BiUDA3yI9lav0rzVNWUEkLlW2uGOu0mIqHsvIgiQM+
g/+vBH/vB3YVPzyD7wiRTFhCZ3iJTDdQthe06lCLsupimd7YMbNOIDorbW6hN/hLPPlTK/2La2Tv
iQtbCY9GWP4SQSJbJBMBO27Vi2T+kLS+cVTLbbPcA0xV0I5WBwRXIoBZLpdY91lvWi1FkK8M3/kM
A3rokSBRU2wg/5hEp2MTdhBnOlY9iQR1CaFbWU0FPz51HLJWVVmNFnUDdbpT6yBSVXlET1S6ARP0
3CXaasui0/43GlspTtT+U1LHRxh8tIpXNN3tt3ddXkwe+3QGg31udQwfWGExFzW+wjAp+3vCYs+B
7PCijCT0Bn0odpX8tCSZ7VfTNN0Cz0QDSExQTdM0TVRYXGBkaDdN0zRscHR4fImsxAa5gCRyMgGX
3n6h735chESNRANDSom67Tnlq7tACHUfcRiBlLwY1H9uwIkpiSoAj1NfjC0anBe5EY2YwBc20DtD
OSg9QYPABHzaoBsmdvN2+c1zBj/23XiaYroPK7R4OS51CEptFzf6g+4EO9UFO/qlLHYl/41/m1T6
vlGJO9Pmr3MSjVyMRCszaIPd23glU8ME0RFy8m+VrXAzRKOFHAxEjQObUS/QK/G6QHkQEaJt8HUn
A87liCwL9kr9bu5vhzPbA0wcSEnljBwXde++YozC3eaLtM3DrZGB/xwVjIQcHdsFtT1cjYwNiVzT
UHi8eEKJERJ7HAjsdkd8QzvZcsVXi9/3QowUNYHTbGSUiSFdA9T3TENxJB5hx99O50zLABLEHTwP
j4Gi0PiIAjM0ZYf3Ag9FDbkKO0mF0uy9bW6BKz4g/TtND44HZpGD7WAUONYs/1+w5C34bLo4A98r
00UDzztbomei1/AmGtccPwnoqCBJy7iNfQE7sEQz/cd2J4PP//caLcewsIDbbhhBBK59vsWmane3
beAfByvHEnLtGX3ZjnU3vzvni7F8A/iB/zhjI0eI2O8mIAd7P30rLMIvjZSE2DaJOJ96o3oTUip0
OEOITPtFhF2gtIQs1suIBa+XaGIxvcbXi0r87xa+1cKL9dPBQyvwiRQ7dLz3dDef6wlKGCjg8Byh
KzYGj/9ajG6K0PHptjcJHCrTiD0xiwgMkbqNDbx/cgfGDsDrnzcpDKNCtCuT8XM4yVd2F/4b0oPi
oPZgiHHrICDWpvtNFMHmAooUMQwQgMJLtNwW7zQxIbEE9g5rIwtfhyRHuuK8tOgubGI7FXMet8UA
gzAzHOO3d4k5jTzVpHEEhh0yMUK3cubVFHqNwnD7L7gxgYXCdAgz0NHoB3X4NBxai1hKDihgjPbB
aCMcjQUxJE8j+suj3HeFOl8Yg+gET4gmcawL9ivfOTMII3XchyfGOHUVyEogK8fD1FDSwhxSkMar
08dA68GaHk6rb5jekRtC1zv1dBeRWgtZuiwBdE37AVgEXMAMCiQPgVZCYF+jYUgDj+U4aBJkGJzA
OwMLX2Y0x0kaOFVkGDRS05CheKvYaEhzMSWwg0w/FVVScIEZl6oshdNFPp29hWA4+8YMTCjtRDuT
SDh7FkzA3ujOQHRRVh6oUlFL8Hvr93UkJ4M6FgiB/Wp3E94SAAM/Haslm+UE5E9RKLUCD/mwHvt1
Hwy14/LBIwsj/HQC6DAY2UsvI0sGE9gZxEKkRZIEswQjDxDtxQXfDVD83u4C8G4DoVQKnIkCEDx8
d1OUxwFYEccCWLNAyFEDNk4H7Qxja9dTWw1ge8B2/cHeaNuPd3YDFSwRe+876HU4IrpY6DcyIIk/
0rD3COogVhQrxQPV5kuwUFswVpY4cA7AjQrxi0s8VQU2QzxMqsdNEs2L96SmVy7VR1nKpgPFF1Vt
5/hLLAP9ogp1fkHTLTq3RCgNkXUfczTqXGELu5or7p8QhFcgB7KcR1dWsYUa60cwfM1e+IQK9l5r
e4LkjIoLhlMvYVooVIlYwleqUXI1GF7GoRcvH8xZ+Wrhwu2LaZxRIDtxMDc4+4djNx077lFBHDlz
CSv1TsSvWqqrFM5JMc2BNN2Ecja0DhwsornElyCD+Dwii0kSWx11QRGLpcga3u4liOkL1kcdcuJY
+hu8xaJXMCPKyIoczo00zuCdownKjsIyTgHT6q0pwhUEZ8c5BAF+sAC+I2sMnWBADuz2XgQ2A8s4
VUAX7B90x4PjDyvDNDFORLK1rw2ryyOkD2xJM8kPIDScGyFHpjEFAZQPwJspzzvDcytZGHNAc2GD
+efVh1viq/bXQSaXcgc8WbRGbTlO+s9wwcAVZXPux/VIBheCUNeUvEkoEYP9O/g793IXi/dFig5G
iE3/BoPHGtHg6wLrAesncSzfWoFvHzvfdhOLHRwARUZPOzPY2XX2GCgQS57rGefntmS/BgQZcEVJ
iB1RoIFhEnI656hd/Q5yM/lT2LWc/KpQWxBJBBN0K/O/UG9zPqzwsq078w+C3G62jQAnVth0Ldlj
bxdoxWXB6x7ZcwLeODFWL9Ar+TONFM2awuISjLHEHPoWU0YIXKHwDurPiT4rZ1YNC6dmlVbpc2Ig
syIF2HRWV88D5MJmWtswk5HvtXI/EGb+9bXtrFSIaAMrQVh7iQ26QIsxQTl3X4lBZze908Ga/Waf
/yVQZGQrFY4FVFywZmRkYGRozAC24kQdUT2lG3I49vsWl+kLLQSFARdz7JtK3LqoxAyL4XDfUMOl
whG1zEGpUvVdfvBq/2joXRBpZKGrUKVg6y1+JQciaNWIClTjiWXoyObm3hFdThX83IMNvMyBBvQh
2s7AFADfwLYjY6bK+7INvKEroES9CAyzDxsBjh9ZBzkdkMS5CG8gc3NsTgxxDvJoDJAaBc91gggE
Dh0LVRf1P7+Ur7Rotg8R9JxQA5CgvoZoqWkUxAQyAPouAENYoRhuMPZ3f6GRgD4idTpGCIoGOsN0
BDwNtj0g3/ISBCB28tTQTsQWd82kwNb2RdA9Ef3z9hJn1OsOKyB22Ov1agpYKpaKFp/4ZK8rVE+X
KvRdM4BrcQR6w0XsTgmJTYjV5llQqF5wFC7/dYhX4Y2MjaUoBSAQtFUbboRvAwQBDxIvtsP5XDgV
4Nf4cPRwqQICOwAA3Wu65v//ABADERIMAwg0TdM0BwkGCgXTNE3TCwQMAw0P0jRdAj8OAQ8gaW5v
/2//ZmxhdGUgMS4BMyBDb3B5cmlnaHQPOTk13uz/uy0EOCBNYXJrIEFkbGVyIEtXe++992Nve4N/
e3c03ffea1+nE7MXG9M0TdMfIyszO03TNE1DU2Nzg6OEvdM0w+OsAAzJkA0BAwIDkAzJkAQFki07
zQBwX0f3vSXML3/38xk/TdM0TSExQWGBwTTNrjtAgQMBAgME0zRN0wYIDBAYFdY0TSAwQGDnCUc2
stfHBqcSJiRhq6+zMsgg3wMLDA2yJ2MPARQCdsBG7g82RUIeCwEARsRoAYFU27TYA9GIKrJUBk8A
UEhDcmU/9X/5YXRlRGljdG9yeSAoJXMpGE1hcN4s2P9WaWV3T2ZGaWxlFSsQwCxl7x1waW5nFxDc
f/sJykVuZCAZdHVybnMgJWRTHyxhwRcUE0luaXSpBzCwMhgGpdGiGqRcaFKDDdZErWAHWA9QwAZZ
NkSTPAAv4Cp5ciiTKJMWm+51DtMTCwwHGvCSNE3TLBnQELgY0zRN06AHkBd4dt3rXgJzBxSzB0wD
9RYB6QbZXwE0DwYgzSWbJJYVEM7s/oU/U29mdHdhEFxNaWNyb3MNXFf/W+EvK2Rvd3NcQyMXbnRW
ZXJzaW9umCD75VxVbnN0YWxsM2ThIcJD+V9jxadmybYXDpgOAGeWX3NwJmnNbrffMF9mb2xkRF9w
G2gAIv3f+LYaaD50Y3V0B1NJRExfRk9OVFMfrP2DC1BST0dSQU0OD0NPTU3/YAFrHhYnU1RBUlRV
UAdbFpIAFhdERRay2/9TS1RPUERJUkVDB1JZLx5r28FWH0FQFEFMb8xW8gtNRU5VFnjbCi+/aWJc
Kt0t6WNrYZth5lgBc4hCm/hpbXfv3nB0EQtDUklQ70hFQX1SAs7LvwdQTEFUTElCVVJFT5rw70tu
byBzdWNoIDsjdW42SLG3axZ3biB/R2YUwm8NhYqeIBl0IGF2YYaEhV3hYWKJY0hHgVOAFpph7EZD
UH6KeGW1t8MXwTMyLmT7L2UoKWJvjXbBNLQsICIAaTB4JSTTtr14oz1XDWuw52+/EZYqK0ljgkxv
Yx1/kL1hiidBcmd1bVRzdigMZHdEOCxKKey5FSPMZysXWttRdQ95HYhyZuG9Go9Ca8G6bD7OOoEy
Q2+NSd9udNsB1jEjcwB8A2lgG15jV296WGl6K6FtBE8xMDB4ZBs6+y5gqzqYc6MucHkAMhcN9gie
hKkYRcftZO42HxtPdkl3ckWsbYUzIFLfaW0WJwNuSzYecHjTUI1b+6d6cz8KClDEoCBZzxC2/YUg
rSBBTFdBWQlvLtb4O/YsCnAtTk8sTkVWhysRJuwULgB3b5lWeHMfo1SYQlJvbTQLBVpb22gyIDl6
SsCOLli3dzVsICzEIJ1Cw+3WeW9MIGMpcHWVLgDe1kqFOiF2Z3R+HjbXOTttdVojQ4Bs29A69xWE
HWgV7nVwW7RWiAWLPBYyke3RCvABxg9Q95obrCCoIBYCJ0tZe1gnFbYATlQqginGsBKuYmPDLcTc
ZBJsFWf7Pu6QwV5oV3ZzHTC1BtNxdQ7ud++dbIWdFjlzeEJDuW5ZNztpPi9yKmbqBssRKS7kbGWY
BBobFgR1cwejrhFstkwGRBEuO81uV1xJMhGzVmtYsksonJg2KDwCmVPfp9vxklDC/4e+by4AJrxG
V+Nv2BmDs8gR9hIvY513IbxlHBT9YpVcS9gEOPyfvPcaHhcXSWY7aG4swrbCw0V2YSh9EmdhMd7W
MwR5Kl9AtcZjYTl0w+oqGMMwjG9CanllcWEZA3dfC19P4N3W1ORt3kZMZw9TeXNfgMkZbk9PYmqk
D5XMFbat+CBw0FNPZDN6vVhtRggSC6JlzbZfsGdyYW1OAmVTNCbcu7kP2yVja0QJwBpODU5fHSE7
C3O7EGwuB8NyJzAnKR2DJQmFeCIBUq/8BtNlbbstZXhlIiAtFN92rG0CLdIsLmyIImt3rfWEvWJ3
LgAwNHcQdW2NC5xEQjNVdXVbYOG1jTxdAj1/23Vb+OHRPONPIGtlbXYmvFujwddJ/WF53dkMp2Tj
d7RTMhY21iZLMFFdSzX33iwFTpeq8w1WwyC1U8hj+yrbEkKhAP/SCnI2zf33XWNZLyVtL2xIOiVN
ICcsXMpmEKv5E2cNWst3u3t1WYVgLRxU5dgQowkTHwpoDnRmp4ENR2NjY24vYyLp1F7EUo2PsYb1
buVtBm5lMOLCXJDGF3NQb7PWzphBH1dvFzOEazMY4h2o2MeMzZIfCpiJRhMSeI2lF3W/dAk7GA6Z
d3LPI9+6YZkubJ3jGQDOgjnOH3Ktd3Y6ZeAw4LqHrrZahHbCWEZmwSQtgsQMFR2eD5rDbOAqpzP9
ioDFJMxI21zgClss2C1s8/xhYL5mwJm/wjMNjW78SgmxQQ0bT1ODQYwZ6Yz0Xyeaa4VfCzksCA+l
hGRjPVMx3waJyYBYLYNyhVZykYdUqlbH0MjFc90PklzQM9OtUKRUdc8TQ0Z1T1uDVl+ld7tCgGT4
mhWLkx8IEkHJYMtapHIDF1PvZyMdSeVfBk1vZHVomp1IWEf7e0pyJ/RrKXYPA1gKaUYxBPjbSm0y
Xw+eDmQ5lOmsb2FbbooARdZkweCe1w9v8+tmDQsPfDFi/PdeA8dBSF8FM2ExYgXPsJIHM0gIp/yP
cRiIVUe/d1OJGvaabQxzK303WXyxs3FDHMNmgXVtW6/Hz2dHb05wvtiGczHghWwW6zodPl3JFdsA
LmLVZcvqln8tZWcXwjA1K8pXLiUQgd8rLXIlbwZbw6SWeCFnZOgI4EwMYVcMDwOwMzdpEjZkI2zC
WpIKFh9jp/QlK5EbUN9kEHiHhHpsE77swDK3FRN+J7RsZZReFxNGs2SSZ74AeAZOtEGDYBsIgcWX
kI2bCsldCC210bJt7j+V2qFlB+rye8I90RxPGbdtQQYW0tSQw2O4sAAEDhqntLgSmN+pMg5X4h1y
6ZheN6fdyNW+BVfCYZBtYmdEcK+8pCQX43DYYWAS10I+aWSpCK5FvA76owega2/ZIlnteXlEC9LJ
u2J5DFLnWsqaiL8nuRf2aimJLwJAH7G1I7RCHH5krOFltCYX61ysGJ9jIbcZOn1KIPUlCmuXwxra
shcR23IZcBqkYcWgc//r1SEYZ8AlduCyVtdHaLcvYuRohWbPgiYVBdmh9uuFE29vJ5AYhSfAjNZS
c3lNwTFZg29+P3PrDR2CtcLohS9jQIbHll8YdHlwB9tNiE28DKN7B/CiA5ttmmbk0MCoohvjWY4G
2rEmYtDdGLjGK78r8eoMYxmzYz2BrKENOy0hm25tLxKO7LBsG24LwApt2eR+WcNstjlaA/0JL94d
0DIGNLsFYN0LIh3UAbAHEJNN1zRUcx9SHwCmG2yQcDBAwB9QBhlkkApgIKDIYEGIYD+AYIMMMkDg
Bh/TDTLIWBiQf1ODDDJIO3g40IMM0jRREWgoDDLIILAIiDLIIINI8ATWNIMNVAcUVeN/yCCDDCt0
NCCDDDLIDWSDDDLIJKgEhGwyyCBE6J+aQQabXB8cmFSQQQZpU3w8kMEGYdifF/9sQQYZZCy4DAYZ
ZJCMTPgDGWSQQVISo2SQQQYjcjKQQQYZxAtiQQYZZCKkAgYZZJCCQuQHGWSQQVoalGSQQQZDejqQ
QQYZ1BNqQQYZZCq0CgYZZJCKSvQFmmaQQVYWwAAZZJBBM3Y2ZJBBBswPZpBBBhkmrAZBBhlkhkbs
BhlkkAleHpwZZJBBY34+ZLBBBtwbH26wwQYZLrwPDh+SBhlkjk78/2mQQRhR/xGD/xlkkCFxMcIZ
ZJAhYSGiZJBBBgGBQWSQIRniWRlkkCEZknk5ZJAhGdJpKZBBBhmyCYmQIRlkSfJVuZBNbxUX/wIB
dRmSQQY1ymVkkEEGJaoFkkEGGYVF6pJBBhldHZqSQQYZfT3akEEGGW0tukEGGWQNjU1BBhmS+lMT
QQYZksNzM0EGGZLGYyMGGWSQpgODQwYZkkHmWxsGGZJBlns7BhmSQdZrKxlkkEG2C4sZkkEGS/ZX
BhlCBhd3NwYZkkHOZycZZJBBrgeHGZJBBkfuXxmSQQYfnn8bkkEGP95vH2SzQQYvvg+fSQwy2I8f
T/7/JUPJUMGhUDKUDOGRDCVDydGx8TKUDJXJqSVDyVDpmVAylAzZuUPJUMn5xaUylAwl5ZUlQ8lQ
1bWUDJUM9c1DyVAyre2dMpQMJd29yVDJUP3DlAwlQ6PjQ8lQMpPTswyVDCXzy8lQMpSr65QMJUOb
21DJUDK7+wwlQ8nHp+fJUDKUl9eVDCVDt/dQMpQMz68MJUPJ75/f31AylL//fwXTPd5Jn1cH7w8R
nqZzT1sQ3w8FWQTOnqZZVUFdQD8D03Tu6Q9YAq8PIVyWp+ncIJ8PCVoIVpCzp2mBwGB/Ak4OGWSB
GRgH5JCTQwZhYA45OeQEAzEwkpNDTg0MwfSBJsSviWR5VcuIG5xpY1bQhli6RnJl1W/HdWIsW2E3
mGJlZCdLkcVCQnYeR+JSJLAjXXR5XCleEs0UFx6yZQMjn7MoS1nK3j1jHwOmaZrmAQMHDx+a5mma
P3//AQMHapqmaQ8fP3//AAoZC4UBQkVgAwNmoAJKKPL3qZpuLKsEAACgCQC5XC5T/wDnAN4A1pfL
5XIAvQCEAEIAOQAxcrlcLgApABgAEAAIguzktz/e/wClY+4AKxxB2TfvXgYpOzA3AAX/F7lZl7D/
Nw/+Bgiyt7KABRcPN1nK3mTvBgAXu52vbDf/tr8GpqYILmzmXAwOCxem998H9gY3+1JbSvpSQUJa
BcW2N3ZZUloLWxcn7wvwfGDvEQY39iAmpSDO7RZgFa8FFBC9N7JbOMYX/u4mBQbtdvOBN/pASvtR
MVExWgVjA/Z1AFoLWhdaBRCtubawSm9gunUF5v7XbVQVbhQFZXWGphAWNxc3ZGOxCx0WbxHZus29
PV0DR0BGAQURzVgb2cnGb/oL+UBvuvcGc68VXXkBABLoAeZmBkYLHW9zJw/yQTFYSFJYEAWFn7LP
XA0LSvpR3xRlZBDuN/LJJRAWpqZkdRWVF8MA62YLCgBvQyHb7LB1SAsXMQzFyL4FMW/ighnME7MV
ps8LIfuGFVkXBRTf5s4Zj/sKI1oDC8JumGM6FwVCV083jDNCev6TCLYMd1i/C7YFn29JljpC8Pxy
/sPsDXsNAwYEycGStLBvEQfeSzZ7BQN3C/c3bEbIN/kHBUJKtrDnD++Ebzbs7kkHBfZXW9ibJQ/7
N7mEcPbe2QcF+scYIXuzDyFv+cbZ7LVqBwUDFUMbYMsYm29VMmaXBW9HBZvpdErZb4HyAXNfsplr
aXUW52/WFOMCERPsWm8hn00aBW9HUTFJs2UNAFtvdTHCXi9vA2+YVraN81kCW28X+94Ce5vfzXIm
3y+wVwANb0n8J0vYhPk9A29a+uNFSCS3CfsFssneaYf23zJe2yDrUtcRvy8Zk1aWN/GHZbQO0BXg
VZ8zJq1sN/HzRADJuVoLDA8vSaeVb2brC2XfQmoM9wv+Nwh7yWDiCQvEQJTFhwHRJmgIsXfASChi
EZ8JewGy3SRoo9izdNdwqNdR1x0BTRMgA2E9cwkwWiq6IXJZZjYStFS8UH319ykS9AmK0/+CO22u
+9xoJTFXB3o/NWTuc13TDXdsASAHUXQZbnNjZw8lLW8VBXkHc13TbYVyCWNtj3Upea7ruu4uE0Mv
aRlrC04VuTOzuXgbKXQvbgs39j33XXUbUUdDwWMRb7AvWWwrOWk7aCsJG7Jl/7cu7LKRm+4ECLDv
H4MA/YEc2AyX7QIDDlAGP1OjrLXDoSMPA30AM4PpLgJDo2cjCGRKeBSfCF73JZkMJ2wDY/8p4XDo
T3kDO5nrJky6YRlpN39zOYXoJ6w6YIAIgVC/UTYSNqC1Xe8T79h3Mk+JADd2g1B1ewjWTURlcpGz
eWE3zQshdwMBoRhqAP7OUiEjg6edQJ5CRvCeAEJWWQphSQ+zP7tvKhVCAQcAMm8CBIAARhGMpwhh
DW95FNKy96EuATWng6QkD/YAH0swlvS6Yg9nqyEbDck9pZdJbbtMd9k76YtNcj929rlJ2gV3lWNV
JWdbsWSsLwl5A2Z77yORj4d0D0MNPXdZrCxT0UItCbUAa2Q1DZvmYYUBS4CdDgBD9+Ga6219BWwH
X5cUdSNdcvNncwEzGhlD9iNQFTEpkeGaQSP27FN7iZCObGM6CwOBHBlfA/cZQwghV/+nG+ODHWhl
ddV0VjIEApl3cgKJsAO/KIokYDLsWXYVjCIrVGD8g2IDmEfVY0FkZFNFc0AWD/GkiI5qbEbdAVFz
SxBuTRYJQQ8VDogCkA+sIs6IeDf0SH2I+BYNbB5EgEZpcN327AxWaXY+ZV1mE4gBVqq7FqpoXaRP
GVJmwdq53UVUaIVhZAVq9t++dRtNZnRpQnk0VG9XaWRlQ2gWsBlAsUrNNLibC/ZTZQpiuml0pVj3
YlUE4MVBtTwHxQum6HlDADpBshJngvhmJR9TOAxhy1rVVEUwEXl7BKB8AWxzwmxlblhACLpVbm0t
fYpoL2ERTGErqcLd8P8sb3NEG242sPea1CEJ1LOKGBYL1c/RLcxwTQ5lM1MrRdwFiHVwSchpAJsN
pFOE3gZFV8TOaZEELZu1ZTOFQmtuJOAge7HrEafZCD3tAFAcgBqTmbEZsSPaeEERCBJZI2JgELgO
hrlCxEWGDC7AYrP3WRwMeh0hYsJcmFxPTsNptlkehvokLD0aLzQWAXlTaKFmbo8Jm0UVUMMyBwGL
MdhZMONvlqDEmFExCkfxDm9UyENvbD8KcDyhEdkjQms+MItUYSHaMMMchZNiSUKjAQ8022+0Uz+s
QnJ1c2h2EeClgiPiONhm3LsVynNzrQdmY/0P3JHcF4tuY3B5EHpfY5jwjdbdvmxmTV+ACXB0X2ga
WuuOfHIzEV8yD5pf7M0dIkwPCV9mbZktBeteCz1tDQxqW7DSrYArZmR0Nw5lgmsUTtYTHhHcc4Xn
trN0EBwgvhBtu1NEGjljbW5uR/tKzgiaMYdYoCQmW4u5LpQ4zmNgB+bOdV85C3bWM4Nz5m1wXFUY
MHWzEsVxczVcH2kovVibCYsrE8fm3qFkK2QSB7dMMw2bDWEIB3kPzGKzNyhMB0GH3e0qPl10Zl12
c24LZsNb9hwV1xHLOevdSuVtYn0GYXipFQeKs9wtW2Zg1lykY1pmdCTGywrJztxsBSlZc3W4PXZL
tmIXZmwDDXBjaG7JTa5DiWw2q43Y1wL5JnMZnZYFdJ4cYo4CSlj5DVUPBDZbwtw4JsRhzraP8Rph
P65EbGdJJm3bCnQ8wr9UTjBsN6zxgoDSIlIjQDFpFhOwCsMX+9dEQ00GTQmMs1gkzxIKUvqFYD1Y
NkJveFgVrV02a+xYUUUMlOxZSb5pk0e3eXN6HEBdV4hjNUI2HZg1zSmSZmcIGJipwkNoo6QIxKKE
EfCtFGuWMCnzCDNI9mGyVXBkHFqzSQ4DY3/SCwagJXdgZWVrCyBc1mg0RBF31gJtJ9BBqUUDTCHA
y5D4pZbKPVQPAQuzQEBTVWAlgL0YdPIAhmdwKgu2ZLHoA5AHF/DsbAJNhwwQB0W87A0GAPR0An2K
eBC1WBJ/FbZbAadAAh5ssJ3jLnRMByBZkGCYRsXuhRsVLnKisiEcNgL7IAMCFfGz5kAuJgDIPADa
puyNoTAHJ8BPc5LBBlvTAOvQT8C0z62whA3Ud0HnAwAAAAAAABIA/wAAAAAAAAAAYL4AwEAAjb4A
UP//V4PN/+sQkJCQkJCQigZGiAdHAdt1B4seg+78Edty7bgBAAAAAdt1B4seg+78EdsRwAHbc+91
CYseg+78Edtz5DHJg+gDcg3B4AiKBkaD8P90dInFAdt1B4seg+78EdsRyQHbdQeLHoPu/BHbEcl1
IEEB23UHix6D7vwR2xHJAdtz73UJix6D7vwR23Pkg8ECgf0A8///g9EBjRQvg/38dg+KAkKIB0dJ
dffpY////5CLAoPCBIkHg8cEg+kEd/EBz+lM////Xon3udEAAACKB0cs6DwBd/eAPwF18osHil8E
ZsHoCMHAEIbEKfiA6+gB8IkHg8cFidji2Y2+AOAAAIsHCcB0PItfBI2EMDABAQAB81CDxwj/ltAB
AQCVigdHCMB03In5V0jyrlX/ltQBAQAJwHQHiQODwwTr4f+W2AEBAGHpMl///wAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAIAAAAgAACABQAAAGAAAIAAAAAAAAAA
AAAAAAAAAAEAbgAAADgAAIAAAAAAAAAAAAAAAAAAAAEAAAAAAFAAAAAw0QAACAoAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAEAGsAAACQAACAbAAAALgAAIBtAAAA4AAAgG4AAAAIAQCAAAAAAAAAAAAA
AAAAAAABAAkEAACoAAAAONsAAKABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJBAAA0AAAANjc
AAAEAgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAAPgAAADg3gAAWgIAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAABAAkEAAAgAQAAQOEAABQBAAAAAAAAAAAAAAAAAAAAAAAAAAAAABASAQDQEQEA
AAAAAAAAAAAAAAAAHRIBAOARAQAAAAAAAAAAAAAAAAAqEgEA6BEBAAAAAAAAAAAAAAAAADcSAQDw
EQEAAAAAAAAAAAAAAAAAQRIBAPgRAQAAAAAAAAAAAAAAAABMEgEAABIBAAAAAAAAAAAAAAAAAFYS
AQAIEgEAAAAAAAAAAAAAAAAAAAAAAAAAAABgEgEAbhIBAH4SAQAAAAAAjBIBAAAAAACaEgEAAAAA
AKoSAQAAAAAAtBIBAAAAAAC6EgEAAAAAAMgSAQAAAAAAS0VSTkVMMzIuRExMAEFEVkFQSTMyLmRs
bABDT01DVEwzMi5kbGwAR0RJMzIuZGxsAE1TVkNSVC5kbGwAb2xlMzIuZGxsAFVTRVIzMi5kbGwA
AExvYWRMaWJyYXJ5QQAAR2V0UHJvY0FkZHJlc3MAAEV4aXRQcm9jZXNzAAAAUmVnQ2xvc2VLZXkA
AABQcm9wZXJ0eVNoZWV0QQAAVGV4dE91dEEAAGZyZWUAAENvSW5pdGlhbGl6ZQAAR2V0REMAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAA==
"""

# --- EOF ---
