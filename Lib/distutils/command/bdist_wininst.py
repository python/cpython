"""distutils.command.bdist_wininst

Implements the Distutils 'bdist_wininst' command: create a windows installer
exe-program."""

# created 2000/06/02, Thomas Heller

__revision__ = "$Id$"

import sys, os, string
from distutils.core import Command
from distutils.util import get_platform, create_tree, remove_tree
from distutils.errors import *

class bdist_wininst (Command):

    description = "create an executable installer for MS Windows"

    user_options = [('bdist-dir=', 'd',
                     "temporary directory for creating the distribution"),
                    ('keep-tree', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('target-compile', 'c',
                     "compile to .pyc on the target system"),
                    ('target-optimize', 'o',
                     "compile to .pyo on the target system"),
                    ('target-version=', 'v',
                     "require a specific python version" +
                     " on the target system (1.5 or 1.6)"),
                   ]

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_tree = 0
        self.target_compile = 0
        self.target_optimize = 0
        self.target_version = None

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'wininst')
        if not self.target_version:
            self.target_version = ""
        else:
            if not self.target_version in ("1.5", "1.6"):
                raise DistutilsOptionError (
                    "target version must be 1.5 or 1.6")
        if self.distribution.has_ext_modules():
            short_version = sys.version[:3]
            if self.target_version and self.target_version != short_version:
                raise DistutilsOptionError ("target version can only be" +
                                            short_version)
            self.target_version = short_version

    # finalize_options()


    def run (self):

        self.run_command ('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir

        install_lib = self.reinitialize_command('install_lib')
        install_lib.compile = 0
        install_lib.optimize = 0

        # The packager can choose if .pyc and .pyo files should be created
        # on the TARGET system instead at the SOURCE system.

##        # The compilation can only be done on the SOURCE system
##        # for one python version (assuming 1.6 and 1.5 have incompatible
##        # byte-codes).
##        short_version = sys.version[:3]
##        if self.target_version == short_version:
##            if not self.target_compile:
##                install_lib.compile = 1
##            if not self.target_optimize:
##                install_lib.optimize = 1

        install_lib.ensure_finalized()

        self.announce ("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        archive_basename = "%s.win32" % self.distribution.get_fullname()
        # XXX hack! Our archive MUST be relative to sys.prefix
        # XXX What about .install_data, .install_scripts, ...?
        # [Perhaps require that all installation dirs be under sys.prefix
        # on Windows?  this will be acceptable until we start dealing
        # with Python applications, at which point we should zip up
        # the application directory -- and again everything can be
        # under one dir --GPW]
        root_dir = install.install_lib
        arcname = self.make_archive (archive_basename, "zip",
                                     root_dir=root_dir)
        self.create_exe (arcname)

        if not self.keep_tree:
            remove_tree (self.bdist_dir, self.verbose, self.dry_run)

    # run()

    def create_inifile (self):
        # Create an inifile containing data describing the installation.
        # This could be done without creating a real file, but
        # a file is (at least) useful for debugging bdist_wininst.

        metadata = self.distribution.metadata
        ini_name = "%s.ini" % metadata.get_fullname()

        self.announce ("creating %s" % ini_name)
        inifile = open (ini_name, "w")

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        inifile.write ("[metadata]\n")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = metadata.long_description + '\n'

        for name in dir (metadata):
            if (name != 'long_description'):
                data = getattr (metadata, name)
                if data:
                   info = info + ("\n    %s: %s" % \
                                  (string.capitalize (name), data))
                   inifile.write ("%s=%s\n" % (name, repr (data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        inifile.write ("\n[Setup]\n")
        inifile.write ("info=%s\n" % repr (info)[1:-1])
        inifile.write ("pthname=%s.%s\n" % (metadata.name, metadata.version))
        inifile.write ("pyc_compile=%d\n" % self.target_compile)
        inifile.write ("pyo_compile=%d\n" % self.target_optimize)
        if self.target_version:
            vers_minor = string.split (self.target_version, '.')[1]
            inifile.write ("vers_minor=%s\n" % vers_minor)

        title = self.distribution.get_fullname()
        inifile.write ("title=%s\n" % repr (title)[1:-1])
        inifile.close()
        return ini_name

    # create_inifile()

    def create_exe (self, arcname):
        import struct, zlib

        cfgdata = open (self.create_inifile()).read()

        comp_method = zlib.DEFLATED
        co = zlib.compressobj (zlib.Z_DEFAULT_COMPRESSION, comp_method, -15)
        zcfgdata = co.compress (cfgdata) + co.flush()

        installer_name = "%s.win32.exe" % self.distribution.get_fullname()
        self.announce ("creating %s" % installer_name)

        file = open (installer_name, "wb")
        file.write (self.get_exe_bytes ())
        file.write (zcfgdata)
        crc32 = zlib.crc32 (cfgdata)
        header = struct.pack ("<iiiiiiii",
                              0x12345678,       # tag
                              comp_method,      # compression method
                              crc32,            # checksum
                              len (cfgdata),    # uncompressed length
                              len (zcfgdata),   # compressed length
                              0, 0, 0)          # reserved fields
        file.write (header)
        file.write (open (arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        import zlib, base64
        return zlib.decompress (base64.decodestring (EXEDATA))

# class bdist_wininst

if __name__ == '__main__':
    import zlib, base64
    file = r"c:\wininst\wininst.exe"
    data = open (file, "rb").read()
    cdata = zlib.compress (data, 9)
    bdata = base64.encodestring (cdata)
    open ("EXEDATA", "w").write (bdata)
    print "%d %d %d" % (len (data), len (cdata), len (bdata))

EXEDATA = """
eNrtfQt4U9eV7pEl28IIJIJNlMRJlCBaN3JcE5uMiYDK2HJMsI1s2TIkDsbYMrLwK9IRj4akENkT
DhvlMU06aW/bSZp2pnMnM5OZ5jY0bYONSQzkwcMp0JJpIO1NZQyEJATMU/df+2z5BalnvszMN/f7
fPi29jl7r7X2WmuvvdbaW0em9P6nJK0kSTqUWEyStknq5ZDGv46iTL31tanSK5PeuW2bpuSd2yp9
TUFLe6BtVaCuxVJf19raJltWei2BUKulqdVSuMRtaWlr8GZNmZJiFTT66x9r/ceEE/Pj5eAtR+e/
hDrrlg/n/zPqitujvN57c2z+TzjMyfmfCdgXeP9HvPbrjnO8iqZ6H/XFeXQ5JalEo5M8rxTPHebb
qJmsSVKFdKltf1OCD1P82aHem1S9SNJwLT0v4HAl8E+TeDYNtZtG6Ci1UpK20s1TkuRLkP7zLvC5
VfPF3Vmyd51MkiwQDDnGMIbLIkkrsgINdXKdJL0/WchuGtaJNGwLjiwVTCrMwscKFZnXo+G6sgLB
QL0kZH1K0Nt4NT1p4pq4Jq6Ja+KauCauiWvimrgmrolr4vpvvarYCebUe9jpzi65aou+Y7e8ZOP5
mlDhlmKNY+P57A3RjedzHp2x8XyrPHnj+UDot28mTSka6HszKaVo4EykVO/Xl7tiaaddDim80zAv
JaSj/s1JIFk08I+bpxY5Bv5u41ek5Q/29HiYbNV7ooftkrSMTXllnUOKFGoS2JSXcde5+0VqCKV7
otvQ7wDJ50HS43qReqM/RNvAHa7yWNogWpfxxs6uZfKNPtMKh+SKrkO/JzaHwzbjPrzTtLwHgm06
YcIO2e2pVvtiab8COpvzY+LWO4hySdpOFKpdivNM7CAkYIc7j8jfjDkH3ZHO01diMV86DRBLc5SD
Xd7iilUZQB/aSow5Lw1kQ9Jzh2/qCh81hLvqXXG01KvQohfxGd6Z6o6FBn0JOkmKzcl+gniaB5hq
vxQ7uGzZwMZY6EwsTY8W5ry0rHb5A7tVMej0IafLM9epf51u5TkuqN5V7omF9C6fNA3E0p4ifUU6
pUuxGLXG0kzlfE7wEEvbis6BpM28V4UBwGY0RkpNttPlkY5EFc/gQftP+WSaVDi/yVfCZZmzM0Lc
FoJqTtfyOF9Qb8RpcAE2xsf1UcsoGjFOo+ZqGrt7XtiGSfGEBxPkRWwONYYHtXJBeFAjL/TP9KXX
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
g0P4QXgJcL4SKedqAR9U+RT0eYea7dFFP9ZOb3RI9MdDTKgllDNeh3QUZR/KNq/aR3K2YCfWROYi
LIvwV4YaGyG1V7hgwAXR0lTf5G2VIVwLfJbENSYgkCp64QSG4LnCx/S1jtDHkKD1PGUdam+GmwzU
NX9dqET0qmdPdD61IlkcZOnFfy+iV884K/XqeWBFsnpuSO3tos4WBLporSWr51fUvkLUFtH/fPyA
VNQbRe0Q/UcFfpJoXypqs+in/6bEnayeU1L7OlHnif59qMqT1TM/aveJOkP0vyye9aLeLOpi0X9a
jK8T7ZWipp/fVenV808a3ybaZVHnCvxdYvwpor1B1FbR/1PxHKffIepC0R8V48f5qxF1uuh/juYh
WT2fpfYNop4n+g+L+bletDeLOlP0bxPPBlFvFXWJ6B8U42tFe4WorxPyvyjknynaA6K+S+DvFPJP
Fu0rRX276P+xeE4Q9WOiXij6/yjGTxbt94v6RtH/jJC/QLR/U9T3iP73hPzTRbtf1HeI/lfEc9w+
t4j6PtF/RoyfKNo9oqZv5LqE3ZL8OaJ9jajvFvhvC/mNor1R1F8R/S+J57h9/6Woi0T/CTH+JNG+
XNS3iP7vC/nrRfujol4g+t8X8t8k2ltFnSX6fyWep4r6CVGXif5LY9Z/ueifJuY/PGb9PyTq2QJ/
x5j1Xyfq20T/j8SzRtSbRJ0v+j8cs/6XifoG0f9XY9b/elHPFf0Hxqz/JlF/TfT/q3iO61cR9SLR
/+mY9V8l6jRq16vffYxc/yFRzxH4e8asf6+oZ4n+/y2e4/bVKWqn6D8+Zv0/KOqbRf/3xqz/R0Q9
X/T/bsz6bxH1naL/NfEc5y8i6lLRf2HM+o+PM13Mf8+Y9R8UdY7Af3PM+o/b6UzR/7fiOU4/LOoC
0f/RmPX/gKhvEv3fGbP+Hxa1XfQfHLP+V4vaJvp/Lp7j/DFRLxb9Z8es/2pRX/8F63+tqP9C4L87
Zv2vEvVXRf8/iee4fI+L+l7Rf2rM+q8V9a2i/4dj1v+3RP0N0f/7Meu/TdRfF/2vi+c4f0+Keono
v/Il43/3OPH/hXHi/7Fx4v/T48T//ePE/38ZJ/5/8iXj/+5x4v/fjxP/+8eJ/98dJ/7/dpz4/4tx
4v/5Lxn/3xgn/v9knPj/f8eJ/8+OE/9/M078/z/jxP/Pv2T8f2ec+P+P48T/k+PE/x+ME///bZz4
/+tx4v/lLxn/e8aJ/y+OE///ME78//Y48b9vnPj/s3Hi/2dfMv6/NU78/4dx4v/AOPH/f40T/4+M
E/9/OU78v/gl43/vOPH/78aJ/38aJ/7/9Tjx/9A48f/VceL/uS8Z//eOE///eZz4//E48f9vxon/
H4wT/7ePE//phSZXoro+qxPVd2Pciep6eiBRfRemPFHVz7JE9V2ZqkR1/T6YqL4bQ/g0v0sT1Xdq
KhPV9V+TqL5DU5Goynd/ovqOjSdR9QNdiep7NYSfIMbfJMa/IT6+WR0/OT5+kjp+XXx8ga+Lj69V
x58dHz9bHX9KfHyDOn73iPHV8xo6mLM0rG+ta2mqH3G0FLTI9LUoHVYHQyuD9YGmlXTa9UVwI2iN
OXbh3WPpXBPG29Iurx8+zaFGy9omdI86oxrmeSQg9Y3ldWx//N0lrXgfySjeCyObIvsim4gBIEZ/
IxQAMQDEAEB/vCv2rfhbcMKnT1wT18Q1cU1cE9fE9V94aXgOZr7qf2zX8D1H9jXak3XqOceKp5Af
XeP/t+/S7+I4fO8mpWo+4PUi7CGDkowMqxn/6nDXhKyylaeNpdgfWFDqOAzdeaUA+hvFiBp+fFQp
7ZHuwo6zXxuLbQQM0QqoJycExWGeEDAJEsEUYKQ2UIynJxs5TLq0TfodYAa0LeirB4Vm1LVSO6i1
IdMOAD6Ifzn8NMI1pnW24CmB00qWXpKSkHyeAE+PCflIsnoxok7/LwIuGXAbpSjgNom+SeJYLFna
irw3RTrF5ari/FrQtR6UfFxHFoyape4NSQppE8e7Q/oepJ0qfXwNmksEzdPj0Lx7aNbi8vwAOZue
vxv+mFQISUhu0lEb7tar5yMj9D0IHtLAw6d8nCz+b+KauCauiWvimrgmrolr4pq4Jq6Ja+L6n3H9
Pzrvwu8=
"""
