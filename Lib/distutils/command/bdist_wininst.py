"""distutils.command.bdist_wininst

Implements the Distutils 'bdist_wininst' command: create a windows installer
exe-program."""

# created 2000/06/02, Thomas Heller

__revision__ = "$Id$"

import sys, os, string
from distutils.core import Command
from distutils.util import get_platform
from distutils.dir_util import create_tree, remove_tree
from distutils.errors import *

class bdist_wininst (Command):

    description = "create an executable installer for MS Windows"

    user_options = [('bdist-dir=', 'd',
                     "temporary directory for creating the distribution"),
                    ('keep-tree', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('target-version=', 'v',
                     "require a specific python version" +
                     " on the target system"),
                    ('no-target-compile', 'c',
                     "do not compile .py to .pyc on the target system"),
                    ('no-target-optimize', 'o',
                     "do not compile .py to .pyo (optimized) on the target system"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in"),
                   ]

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_tree = 0
        self.no_target_compile = 0
        self.no_target_optimize = 0
        self.target_version = None
        self.dist_dir = None

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'wininst')
        if not self.target_version:
            self.target_version = ""
        if self.distribution.has_ext_modules():
            short_version = sys.version[:3]
            if self.target_version and self.target_version != short_version:
                raise DistutilsOptionError ("target version can only be" +
                                            short_version)
            self.target_version = short_version

        self.set_undefined_options('bdist', ('dist_dir', 'dist_dir'))

    # finalize_options()


    def run (self):
        if (sys.platform != "win32" and
            (self.distribution.has_ext_modules() or
             self.distribution.has_c_libraries())):
            raise DistutilsPlatformError \
                  ("distribution contains extensions and/or C libraries; "
                   "must be compiled on a Windows 32 platform")

        self.run_command ('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir

        install_lib = self.reinitialize_command('install_lib')
        # we do not want to include pyc or pyo files
        install_lib.compile = 0
        install_lib.optimize = 0

        install_lib.ensure_finalized()

        self.announce ("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

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
        self.create_exe (arcname, fullname)

        if not self.keep_tree:
            remove_tree (self.bdist_dir, self.verbose, self.dry_run)

    # run()

    def get_inidata (self):
        # Return data describing the installation.

        lines = []
        metadata = self.distribution.metadata

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        lines.append ("[metadata]")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = (metadata.long_description or '') + '\n'

        for name in dir (metadata):
            if (name != 'long_description'):
                data = getattr (metadata, name)
                if data:
                   info = info + ("\n    %s: %s" % \
                                  (string.capitalize (name), data))
                   lines.append ("%s=%s" % (name, repr (data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append ("\n[Setup]")
        lines.append ("info=%s" % repr (info)[1:-1])
        lines.append ("pthname=%s.%s" % (metadata.name, metadata.version))
        lines.append ("target_compile=%d" % (not self.no_target_compile))
        lines.append ("target_optimize=%d" % (not self.no_target_optimize))
        if self.target_version:
            lines.append ("target_version=%s" % self.target_version)

        title = self.distribution.get_fullname()
        lines.append ("title=%s" % repr (title)[1:-1])
        return string.join (lines, "\n")

    # get_inidata()

    def create_exe (self, arcname, fullname):
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
        self.announce ("creating %s" % installer_name)

        file = open (installer_name, "wb")
        file.write (self.get_exe_bytes ())
        file.write (cfgdata)
        header = struct.pack ("<ii",
                              0x12345679,       # tag
                              len (cfgdata))    # length
        file.write (header)
        file.write (open (arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        import base64
        return base64.decodestring (EXEDATA)

# class bdist_wininst

if __name__ == '__main__':
    # recreate EXEDATA from wininst.exe by rewriting this file
    import re, base64
    moddata = open ("bdist_wininst.py", "r").read()
    exedata = open ("../../misc/wininst.exe", "rb").read()
    print "wininst.exe length is %d bytes" % len (exedata)
    print "wininst.exe encoded length is %d bytes" % len (base64.encodestring (exedata))
    exp = re.compile ('EXE'+'DATA = """\\\\(\n.*)*\n"""', re.M)
    data = exp.sub ('EXE' + 'DATA = """\\\\\n%s"""' %
                    base64.encodestring (exedata), moddata)
    open ("bdist_wininst.py", "w").write (data)
    print "bdist_wininst.py recreated"

EXEDATA = """\
TVqQAAMAAAAEAAAA//8AALgAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAA8AAAAA4fug4AtAnNIbgBTM0hVGhpcyBwcm9ncmFtIGNhbm5vdCBiZSBydW4gaW4gRE9TIG1v
ZGUuDQ0KJAAAAAAAAADqs5WMrtL7367S+9+u0vvf1c7336/S+98tzvXfrNL731Hy/9+s0vvfzM3o
36bS+9+u0vrf89L7367S+9+j0vvfUfLx36PS+99p1P3fr9L731JpY2iu0vvfAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAUEUAAEwBAwBmkbo5AAAAAAAAAADgAA8BCwEGAABAAAAAEAAAAJAAABDVAAAA
oAAAAOAAAAAAQAAAEAAAAAIAAAQAAAAAAAAABAAAAAAAAAAA8AAAAAQAAAAAAAACAAAAAAAQAAAQ
AAAAABAAABAAAAAAAAAQAAAAAAAAAAAAAAAw4QAAcAEAAADgAAAwAQAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABVUFgwAAAAAACQAAAAEAAAAAAAAAAEAAAA
AAAAAAAAAAAAAACAAADgVVBYMQAAAAAAQAAAAKAAAAA4AAAABAAAAAAAAAAAAAAAAAAAQAAA4C5y
c3JjAAAAABAAAADgAAAABAAAADwAAAAAAAAAAAAAAAAAAEAAAMAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgAkSW5mbzogVGhpcyBmaWxlIGlz
IHBhY2tlZCB3aXRoIHRoZSBVUFggZXhlY3V0YWJsZSBwYWNrZXIgaHR0cDovL3VweC50c3gub3Jn
ICQKACRJZDogVVBYIDEuMDEgQ29weXJpZ2h0IChDKSAxOTk2LTIwMDAgdGhlIFVQWCBUZWFtLiBB
bGwgUmlnaHRzIFJlc2VydmVkLiAkCgBVUFghDAkCCjUEmknnJr9G0bYAAAU1AAAAsAAAJgEABf+/
/f9TVVaLdCQUhfZXdHaLfAi9EHBAAIA+AHRoalxWbP/29v8V/GANi/BZHll0V4AmAFcRmP833//Y
g/v/dR5qD5SFwHUROUQkHHQLV9na//9VagX/VCQog8QM9sMQdR1otwAAJJD/T9itg10cACHGBlxG
dZNqAVhfXr/7//9dW8NVi+yD7BCLRRRTVleLQBaLPXg0iUX4M/a79u7+d0PAOXUIdQfHRQgBDFZo
gFYRY9/+9lZWUwUM/9eD+P8p/A+FiG182s23P/gDdRshGP91EOgV/wD2bffmcagPhApB67EfUHQJ
UJn9sW176y9cGBjxUwxqAv9VGIW12bLxwC5nEGbW8GTsdSUuwmhUMOlTt/ue7wH0OwdZDvwkdAoT
vb37yAONRfBQ3GaLSAoDQAxRYdj70Ht0Sn38GQNQ4XVvpqQ7+HUJC4idhy+U7psOVmoEVhCghIs9
iN/X4CKQhg9oOIk8mu3WNusmrCsCUyqcU8/3W9uuCCV2CDvGdRcnECjChmYwhJURM8B8ti385FvJ
OFOLXVKLIVeh2B4W/kYIZj0IAFGeADiayG1uu13sUOjWPrJMEDZXyLbLVrgiEkC7CMwWXgbYtvvP
3SVoqFgq8VCJXdQtFVze+/7CjVrAdHf/dChQaJCfGUtbutvnBBZ8lXQTGg18kobPde4E0JH2IR8V
rqEbyGEfZNxQADW8Y+zGumZ96/92FulThrs396GsI5de69nTgewY7Rv+ju6LTRDZDFZ8C/qNRAvq
xCtIbf/f+Awrz4PpZtED+oE4UEsFBolV9O5K7cft3y6DZRAAZoN4Cv2OMysDixmb7ff/i0wfKo0E
H400EQPzLgECKx6BPt/x2W4LAwQ3Escuko0UHw+/3d9u21geTfAGUCADQBwD0wPHfo1r7bfbPBAN
RhwDVhoDyAN2VIpGaPxyGh7sjYXo/p1dxNjydc8LaLDuUMy+EB8zvZst9HCNhAUNF0zsFja3GlAb
JQMA8B4MYduAncxLBF1XuEYUfEx+94C8BecbXHQw/3UUWFC5BrMr2CEAhm4UGwo787sC7FZ8AgMT
OYPE7ToalrgY+EDB/NIKUGud29g4agZBFCES/xoVOJzc7jkGjM9X0pql77beXOv3ixcERBGKCITJ
/X074/+A+S91A8YAXEB173NAXAwPdBepwfCIdpxB3WFSx95oHQVOCgvAV1AUTGFmKzge83FKDEhv
s+X/agqZWff5M8lotHBRAB5ovAJmyTS8AA0vMCs4szW1jVAy1xoNFBUoWzrCdr6gigRWGVlQLg8B
k+vuRiAdJBj/02gp1752YLcoIGAjCgEV08yZ7vWpC18sn54aW2vmRPTx9sI+S9237dq4AAYAPczh
TuqBhGM3bAUQZ3V8Vi4sYeFOw4f/BfDHBCRAnLkA8PnAZg732exgNVgFzRq39liXBOgD9P/WaBsY
aP13bbRhDnTtJ1uBeAhNbku7OL11Gf9qHnAEwKyZ72pSVCmdEBuk4Wla8DBt/HZhi1My1iiL+FSL
VfyLyOO28Lb0ESvQK+JSD/gr0xpbx38DwVKZK8LR+AjwFfjYFCIjzA0BgPVb4B008QhWg+hOVyXB
84Fu4YFXXjgeLTSDt7TBbkgw7hcyb7ZWt77+xWYIEdzB6BBIcm8ZCir6fYstNDBAw2dYCZ2tI2jW
gLZn+NAC1TUIbxYbFgG4M/9IM+1VVWiLFdM7xS3cMtB5tYFIVT+GBeHbghRaLOoDJATr3uE8Hv0S
iCWUFd4O7KAUdUMPKMGA2/G+WIkbaO+XlKZukQzbDKDEwwogm4HFTEEezGvAHvYtlSQYaJmT6FVO
Bmuw5FU6Kop4FkpBhbEc6DfI2Del7XRQVRyJjbLN3QwtFVjuPHFHKoRZYowYRBAQAtli7s7qAxxo
LBumH2DL3k03KQTrEGjbwuIwGm5+60WoIFw4i8MI/y4z/1dXKGiaJ+3ChoPcMHUELOsC7UwkjLFX
7FZ75MNQTjZ+W4HOgNdgu334UzPbqBkADFM8ksdipCXb/HoIVHQH4tZuaDA+CcdTACv0U3Uqt2gh
mAH0UNAvNmKugbEnO/UUyzXxGsHKHTwGlUHX7GUQ/DvDL744Tvx4q/z1TZhRJz8C35tLndk2nFNQ
e0j/gHG29iI91nEQZBIBzbOtk0TXOugp+P5U7GylJQdi7MfZkrXZIKrGNex7Ld238P1OU7XwAGgI
rtlm7wwfGwy8WTfrAjbs6GiadPcY/PKLFdwDhBugUhJGZU3CwHLGdyR30oUCDFOEmZ56uM80/V4t
8QKBPdpqZzDZKizwAqtN6zOH7ZrK5GjsEFhQOtwGtw8CY44LfB3OAYC1RMo1cmjJs5WV5BByw1Xu
3nJ9TXq4zQg9Mc90KT1hPVs8Y3XkA8SftjgLNX2pviOJPULdh8AQnqeAuJ8RXLlC4+zEDUEUvsDF
820Ip633HQ9o3BwZLEY3Ef/0FTygo6GB9wC7oZP84lNMvo8VnqeJMf81AJsFcLupxcFOAteLOTk1
bBAvwnu3oxiGdBJo4De9Itktuu0LglkekFdeyWjEGoGhWSFJG31RariPL1GDPVRDvEC8Eww3M39o
8DuANd32g0k6La9A1fiFR819asMFj+tTwDVUE3k5eh10NFcc1Acduk4zsgnA6LDOGOvatgbYTAWJ
g9MHEIX0apW6DBNZ4JV3cummIlOKX1nDoEwBwrXm3qGUYOY7Lf4vNFRqsAasBEHr9g+3wcHgEDEe
myZdYVa7BxHJfnSzU7H9vVZWVRDWVs51QRSpUU10bUvGX+42/42oaARzKA93Fu6tJAqUJHB8fAQm
oS2bQBAASjwc/cr9KYt2BOuniytLgcRp4Y31vcNoAJQpW4feqaVmEABE/wIVLO3G6nMQKQhqSEgm
q5Zl28B+fAJcQggMre1YcGsNxus7ZGZ01L1W81fTUP+SnbrDgdZSV0YQwFmd3YXrb21Qj1CcPbvZ
TA30WGowGGg491w31k9n2UBAO2ouOCQMSMZ7DKoqaDQluxDsz5XbH0rL67RfbOF62cMOiNiLwwCn
CFiGhhkcMAzXVUZoH+GJBshZiUYEA4Feb+HGEhtvVjkBvgouvfjCdDUhTQhQUaGpABEO4SxUBEg3
CNUsIpvP+GDzPUoW+54GiB3tpdL1OdqkK/CqElYEFHFsq0sQanFA+jW8dHMxM8D5wryhUMAr7OBY
qHRJbRVsXNguxHRDBAGNaiMmdHgbzHXjbDg31hgGdBb9+//WkwYHD5XBSYPhAkGLwaNC68fHBXft
gOsH6RfsXcNqJP6TOd5oUDzHQOgGXvfYG8BA6OngjIocQXQzaBZq4MDfHuTWOTOd64MfCsR4CXzo
vTiZIuvb9EHttQ155ye3PREIoRMC7zpoGLlO6yWuQ6aQCQRdS3ddgnQVagswjX3EjvNY6Aa3qwb0
iUKrq1dMsW1jH/0MqxqQE4wbv2tuYGpxeZbAMIkvc1SAUuuuqBxrb49t3OEUjBvYVhUHInu0NXPM
a+Qf8MgrGbsznRJsIFMWQBn0JSdPLm3OGfgFaOfJbkgfn2A2U/d2f4w0XHyYYwWUb7dsJisFrIx/
kCCbj7Yt23W0AryoD6T+bmW6XpkYOoAEs5lewQ8ZCHAcYPCPAzERhvcScFmjJBl+OowYFWiUZmxy
mawwF+or6BSeA65ncAjJEf/b4nrxV19cMr0SalC7/XfJPdxpvoyzhAQM11U+zVt87QhZSz9qr8Cd
QHynTgxgHGh8nRIWm3QfdA1AM7O9raJkGqqEEyAl3YOjQRhU0j1q9Tt56lJE3vecbIhAnDyqMPzK
hsfHjBPUUKPfAw+feiPBDK4xoTfsKFQah4VbY4BgYYZZ4C0EaKj/LMCSBfCvVU+A+Vx1YHl7+0SK
SAFACDB86AQzfhZua9n+G8dyddnGBg1G69MFCvQ44EnXlnpRH/Q8CsBv7tfKH4gGUh+tiA5GQPCp
NaKgQn0rUVM694R4VF8DVvoIgLoYgzH/eN+DDSsFpSE2KvwRZIRwQeHVrM8EdMETATYb+QQFXNCC
xfgDaxY0aJfsKfOtQ78aJHhVCBzXC97a/kwC6lcrQRACDAsegTkG3gS3uI00EH+pANp5VjQSt9uQ
2QudcIyVB7sEPTjHgff+K35mZk2SOhcyGzqG5dz+aeZONr5sWM8U3wWejbTwdNZoGj2Li7EFLRZT
Pe7goJ2g496VAonTYIMFmovbjf/TV7dgwbDS9G3rHqO5X8Zo0OrrB2isDUDz+WhCBNAJKPRlOAxs
MClWLE0fuazYA9x7FECB5OCxOXJ9pX8y5A3sDpzhbG1kj51Cbe5jmTQG2LssdXEi+DWE2UynJYR8
F4GAe13wdTAE0wPZ0bH92O8K9HRreG2OVsKMSOawEQzCWXNb/RAECy1WQZvD3Ip4aGEaCmzZCacz
EXDIzAD+/7tgXjPSO8JWdDOLSBw7ynQsiVAUtP1/2QIIGItxDPfeG/ZSg+YHszEfseG2hRwgFFEP
Gtz3XsO+hq3Ccrib/wiQAC100UANCKA6oBxDtPKu21ROJIXJPR02t+xaCps/KPwIHhoo3NJtJSuu
JA3HAAAdNBfAVJ/wUTum2JqNxyT3igENJlWBzMY6wVnnFWLfma2aCtx5DDv3dQo/tfbN1OeQZCCJ
fhg6E+b2N7xgIIA6hn4oOX4kdQcOJKA215V2gWoYO4Qg0ieJXihJ14Y+/OkQiXhr8IWFdFYXz4l6
DJi0urd/v/fZx0AMAXj5CHxZBA9/VB+4v7C1/xHT4IlKEFLXUTfaG9JQ98L9aPvSgeJQOWVSyBtc
GYpv8Zr4QU8yehR1D+6xW3ajbg4UvH4LjPBks1YbyV+4+mkQKs8TlnFTVRC3CA66pgQDdgr5A7+w
2XahPgAI8ItUIzPbg/oEtH//1b/7HZXDS70FweP7iVwZ8Ce+PYkIyA0Ph8RiJI2gKjaarfAZBLY9
iEkeiW+txdYNEAgeLwWLDootNr6NERwENRYQBDau9I3WD0LMLhZ0Fcc/VbtlXnDdbBiUdUzroiLA
bty+i1AQwekowQhddhgkWtvB5IBJFr4XBVCL5vm9BBFImdbeFshvR0B2i14cC3kXao/bBom9HwMT
kotD3vt/owRMCAPB9/WF0nQhxwNWlE+eLubR3V9oaPbBsLO5jSAlgWMpByaPAo7YHNhL2vcChoUc
4vikPP11GKPsRCHYAlXzXizdttbVOQKSIgFPaQKF2lKbc6Azjej4x+ZcpFIeEkRUDPkv+WZbC9gM
OeMILQJzL5gzY+Tt4UrtucZb3MHhGEgL5EkOhZZsNAnKOyiMtRuDSEKJBjocFAH7W1eQgUg34hAD
yolIOZKL5JIKvghmSy4ZC4Q2ZQ435j85SDQSNmA2Q4Tr5TNZQAjIgelQpL/6IdtoAnUJi8d7wggO
zrllp2dyamN3JrPQpBZQR27HAQOWEB5gORZITzfOlqXhigobUOHRPlaTHCBHAgQO0mGHECYgiSiz
EkJKGCEfstdsF3hOMPMGuPg7hmlYRmksQHAsm80KACVqW5JvKwD9DEMBKf32i7klBjgL1yZM0zTL
7U4nA0Qpfr3402ybbUQqF8UyKANnoeBllk3bfCqIW9bAA0l/01e/BXqWosDhPIlDdA8E4I321g8E
BXUOvutHKFJdbx3YoFfKdQZ1DT5XxjuygVHqMpwox/IBFgS27UY0AjAOOO5RCVd8tgggdA45ww3b
bq3QH2BHMMDDf3OF+ky2bWoqZGMWHdWgINWI9rKGHL+DysOLTyhooEk7qQoccBpf2U1GYla6i1co
jJDQw8M2wD1yQMlQKO50DpooH58rUR6DhDVwLqI2AoW3LkSrA9geiV4svJEYErM4yAROt3mEZKoy
g+wwOFNvWwOtRThD+ylDsmsJ2tYaEkguS/9hXVvoWxAwVjvIilQKFUQFXFv+cwUrwUjrBSwHHox/
8OfyA4P4CRkMhbw4QNgYg/0nB5ngA3M8nwyWv+0brg3G5EiKD8cUTJSL0a7r/v6LzdPig8UIYwvy
RzGJOIkvFvi393LO6wQ3r4PgB4vI0ei1AeCm+9ZkHksYd5Fj5IPtA3r/7VkZAc0cB8HuA9PuK+k/
dlcdbLMcTkFIOSBSjbAndreFhI0NMFEOOFLOGnpdwzmsJFwhNPh6UT5Q4u0PLFIQ3hAqrDnzFegU
ia6171zAYmbsWHEGYRR1hzfkA/j9WBRwbl28ziBzLKn6+qAGPZct0D9MLE/2fEA18TvIJ3Zy1IvW
i86C4X8LvCsHcuoQM9Gvojjti8Eg3drbO8X6BIlsXEsmAYu3LTHsiQPpTNIXvCp2Rks4x0zJwYt8
t7jhuxpEO9Z1I7+LeygtdBmh8F3ji9c7sRVzByvCSFdkuu7Ydivyc4k1dWe0TEFItdLBFwRTiVM0
GDkHbdZ9sUcwatajTDoxe4624SvKSf9LLAcEPlU+yMh3dSBi99byTovOuLNNbsKLyKResLDQMEwL
Bcl21DR0b53CO8EFwT4URND9EluH0YEC86WLyi0c2x0ev98DK9DzpNpcJUQDUq7daNsNS10V8CsM
FmvB0JmJeBwpAWhdgsMEm2QY7EEqOZDHGJYOczgyDxnjKg6S0iX/bLE5uj8lyCCYH4cd3bHQtwbW
0DzgCIH6oLB10c0FE/IF3QV9HzfngG9GjYQIAtN3A0go89k4S/lQYQyNBZo48cYOSA7HQ27wBKNp
7G3rCK5xU5IIETxdaHQKg2Itc2hZMiZTye++NAYDEh2RFiwITrGL/LD9+NSYXEsMxQSRYQiHuUJr
CAOGamdyyd4Pu5gwuBOhyHMhPDTmCu+1xzFpNaA3IOlL2+ly33AaJG9DEI1TUW3O0GBSNFfx41Ds
SnE2UTK8//CFIcJCts37COYFTxYMH75l0DTiHzc1An078XZdD4N70lk76HMzW3s/HuNKOwXr+vlK
ITT3Xpj29PkHu3Ssufou+c2LyfCIuejaO/gUI8bmVMEBjeY0GG53W63ZVRCXNHMbySvqhgXX2tEM
RYQSinG/HXS4QKQ3IjkSuc10A+Lx+EIz8oPoEs1ZYX/JXSsk+AsfwAs76XM7mYF1C+TgBB8wnXPt
0cjpyex8d1Xaa/S7iwyNqSPOJg4U1Hiv1WLUkBvXp3MZ4RUc4YwKHnKvd+sD0Dsqh6l10yqFGiWW
ORDpmfDwzcXcgpMVDdodivzrAj18e6kAqAxBSJmP/HX1d4kycSChXnqChZjpA932FUAkJlFQQI3f
tY7NmAksJFESUjwC7l/DNjs/UUIFATxrWQORjc8UZQkHcZYd5kAGDzgcJDjqpOkfFUwkd67NPhzK
JTTPdz2wfU8DnzwgKxx5UJbnjiGkToRXBAQG8ADbQilID3O2aC0sXms8MJfYBMXXrS7QK504A1ZM
Bq3m4OjOTe7ntkanvlHsSbF7QHYlmnl0Vl22VAAPuHhxHSdNPpwZJAoNIxixQJo4FCnMIRgbmK+V
idIALACNg7kkoZ3Piybabuu1aJqW2umVTFF3SaA194XaF7CQDbsdcqEzBjDD4DfTxqFRXGH9yzMY
ELfnZo12P1VR8uTXakoG++H9K9HDA+pQTkth2Z7sTI0xi2k5UdAr3SJsrgFmkuovFVJRa7MEsjpD
hTJqa8tOfcdBGPA9S0ZAIcz9WEhIUYl5BEZETThwhBgRSyDos2YRXKOs8oSnhEhgbxAVUsjGz4CL
91TKxADOW6ETnzlBBJOKhysE9wzu9wPug1FP0VjQEkwJuEWED2HBE5/Pnmr8UJTJhkIIeZDMdQmF
dxiMzyuObySQAhid/XUG9jDKZlulT1GorGOwRTrXImiUYcuIkBR8nnWtyhi7kVIZhAOX3VAGNc8J
7iWksNr+gWCIFTL9X7aQzoUkTBDsZYGEAxhShD6NFPYICTtc92yl5EhQUqYHDEDdHV6vpmbnQVBW
U94jywl0S1PRdDeht4/Q5nvoIDcuiVYEf1Arkm2p8tWLbgjjbn0+GAfIt2YIGDHVwvfIQy6Lx0xW
VcWkydagY0NLVplDSHopO52YECYEpKCXDQSZBIYYkVONta8mT7D+RUNI3XgKeypD/2QsFF0tl8tl
swPQ+y6qL5Ew4DLdslkuhzfuMjjIJ+ZiBmyWLEjJMxsEG+CS7wyiDGqiAAcoPxhHAZ7ClVhp3YtY
83uNckYoGA0YCFfADnCAY+lPiEGqsbe77w16BtzddQrswgyaHd+9i1z52w+G7xFVgfuwFZnDf9zF
73IFuAgr2IIPjKGt6MHtiN+iFdthEIoWg8aQs3dRG6xW8QP5CPJDDjnk8/T1DjnkkPb3+Pk55JBD
+vv855BDDv3+/6IEG2wDTbxkn/W2dSbZFRYSRhNIdfRvY3crsQ258fL38Uy/CLpbbbuLNff364v1
hxMxXXB4AdgXW0JfC8EINsGPSZ+VCFBuQGa5goVQUEx0vEsDdD4Eww8fHI3dkmqhN4Uiik/wjrtC
o0WIUBBaDIhIEeAOeiN1AAAPSBjDvOLhMN8UfyB2CyYMLc4DRpLwVhdtCRrI2m4MwROuFjYMNMF+
xbw+2D5NEMJGLAeJM00rbkCBOt/+BmwWOrTxeEJPPRwanZy1HYHOEAoKkmwoV8sfjEZ6LIl+O4xa
WmArKSsie635canUtoWJBmXcVRhs2NFH9FIiTRFPVRDsnjoGdzsM6sijfhyd6VrJuEidKA1ADWRs
c678GKMwBeD/GnKldBNJ99kbyRTnfrHVg8HvTWErLWZjsdRStZGNTbZFYfXWWLJFWPhzREDFc7Hi
XAS6DrW9AK/Y7TAAso7P0+B+zn3J0ADHCAvINnngLEHvbHTXPwoscryuhfiosaW6IyAIVshJGI3w
6NdAFNPouG7BvAWXfkUr+ECKAcUWi0mYabZIj5UIBq+V6G70qBB0GOAProuv2yRdrAUiHwJAr0Vn
Dtp2w6ggB+MnHwcc0rkhgtpCGgXsvc+vSNx50OQj+Ybn2Ai+iwStvW/mTLlNBAPIzq2RNjNda7DU
cgPX0wyG5rZAGPVFzGVGkRwSXpYDNEzCEkRkDEQEWTC0gFZSZQwHCEG4jQzBiEHYOWQIkAIMDKDA
QA4Fb44NOBR+A2sV1TWpdwN1A8IrN0AK0Z4z1h/tI5bjKbPxsQmWVpfUbJ8q8U4sLY51IT4wVGpQ
2jvBEVQtRNgenCkM+wjrD6WNOHV/Z4YUUoUZGWkScmI8DHIDyZBtYl12yA12Y2EiXo9ijBC3RJ7b
AZBCnPv7JPMJiEr/EUFIO1AIOp77Im4HTgxmSWkwsDlhzyg3sACoQIEN47D3yfjgTQqICkJIRL0n
4Iow9s8Ui8foloErCuLHQx8rzciagQITFxGqZrqTRPQUw0oJMAsg4OoY2NhiUDfIj8Blav0rzVNW
UJVtrjBJwOu0mLLyIAuKiQM+BH/vh4P/B3YVPzyD7whCZ3ivkUyJTDdQtrTqUFiLsure2DEXYrNO
IDorbTf4S5luPPlTK/2La2TviY9GWKELW/4SWyQTCUEBO+GLZCL+kEQ7dCyXzXYBPAPcYD0ePpSE
zXLZsj+HQf+9QOJZBPlqBPkMIJaChx5RU2wgQhMzSHQ6dhBn2PjoWHXbdQmhW1l1HA3Ubf+yVlWL
bAmNulPrIEXpSsBSVX8BE1ZbJvqFM2yi0/43kesv0RpbU1LHRxgkvN/epThXNF1eTB77dAaDqOmW
gn3gDB8AvhYLi7nCMCnPq9zfE4Hs8KKMJPQG/AVqoK203wHVV8+apmm6RANITFBUWGmapmlcYGRo
bARvmqZwdHh8iawkQokNMkkyAe9+gS69/VyERI1EA0NKibrtOQj/yld3dR9xGIGUbsCJKYlbeJGI
KiqPGpwXoKe+GLkRjZg7N4AvbEM5KD1Bg8AEJnbz8fi0QXb5zXMGmvR/7Ltiug8rtHg5LnUISoPu
BDvVNtsubgU7+qUsdiVU+r63/xv/UYk70+avcxKNXIxEKzN4JVPDBIjQBrvREXLyb5WjoFvhZoUc
DESNAyvxTjajXrpAeRARogPO39rg6+WILAv2Socz2wNMp/vd3BxISeWMHBd1790EBvqKRm+0zf/Y
DrdGHBWMhBw9KPF2rCtSjA2JXHhCiRHxTUPhEnscCEM72XLFkbHbHVeL3/dCjBQ1lA0FTrOJIV0D
cSQzFN8zHmHHVgAjfjudEsQdPA+PgQIzFIlC4zRlhw0F3gs8uQo7SYXS7Cu297a5PiD9O00Pjgdg
FDiSm0UO1iwt+In+f8FsujgD3yvTRQPPO9fwo26JniYa1xwgSfT/JKDLuI19ATvHdieDz/9uwxLN
9xotx24YQQTdwsICrn2+xW3gHweONW/dK8cScu6EJCS/O+eLI0d92bF8A/iB/4jYP304Y+8mICss
wi+NlIRxoAd72DaJOIu5hF2fuj90OEOITKC0hCxoYvtF1suIBTG9xtXCr5fXi0r874v108F0Nxa+
QyvwiRQ7dJ/rCUoYKza89yjg8AaP/1q2NxyhjG6K0AkcKtOIPTENvPHpiwgMkX9yB8YOwL4ruo3r
nzcpDJPxcxSB/tld+I/JG9KD4qD2YIhx6yDcd0FdIBRS5gKKFDEM8W5tIpWAwks0MSHwRcttsQT2
DockJrY2ske64ry0OxUWXdDCcx63xbcwd3aGY/yJOY081aRxBIYdcuZXJkbo1RR6jcIxEW7/BYGF
wnQIM9DR6Ad1+FiEhkNrSg4oYIwc0D4YbY0FMSRPI/rLfpT7rjpfGIPoBE+IJivfJ451wTkzCCN1
3HXq8MQYFchKICv4eJga0sIcUpBA23h1+uvBmh5OkRt39Q3TQtc79XQXkSwBdFhrIUtN+wEMDIuA
CwokDzzQSghfo2E4AGngsWgSZBiHE3hnC19mNFX1OEkDZBg0UtPobQVv2GiYYioYBNheAjsVVVJw
hfbX0y0EC8xFPjj7xtqZ7OwMTChIOHtGd24nFkyQYwRWHlu/B/aoUlFLdSQngzoWCAAYgN+B/Wp3
Ez8sJ/CWHavkT1HkwwLS0B77dR8e2RA4tOMj/HSy2JAPApAvI7AzYDBLbEJmCQwmTEUjiwskCQ/f
Dfj84N0g2t7aofwKtqbcBZyJAhCUx+p3eLhVAn9dh0DIUQZsnA7tDGNr16e2GsB7wHb9wb3Rth93
dgMVLBF77zvo1rERdVjoUQ8yIPcl/kjDCOogVhQrxQPV5i/BQm0wVpY4cA6LSwE3KsQ8VQU2Qzwx
qR43Es2L96SmX7lUH1nKpgPFF0tWtZ3jLAP9ogp1fkFETrfo3CgNkXUfczTqcoUt7Jor7p8QhFeD
HMhyR1dWR8UWaqwwfM1e+IQo2Hute4LkjIouGE69YVooVIlRYAlfqXI1GF4bh168H8xZ+YuohQu3
aZxRIDtxMDc46B+O3R077lFBHDlzCSv1TtVSXQ7UzkkxzekmlHuBNrQOHM0lvqQsIIP4PCKLSdjq
qBNBEYulyN7uS5QaYQgL1kcdcuJY+Bu8xaJXMCPKyIoczo00zsA7x40shI7CMk4B0+poTZUrBGdf
OQQP8IMFviNrDJ1gXgByYLcENgPLOFUFumD/dMeD4w8rwzQxTg0mkq19q8sjpA8PZUuaSSA0nNkI
OTIxBQGUewDeTM87w3MrWRiDnwOaC/nn1YfX2RJftUEml3IHPFmjNWrLTvrPcMHuAq4om8f1SNc3
uBCElLxJKBE79x/s38FyF4v3RYoORohN/waD6wLrO9aIBgHrJ3EsH/7WCnw733YTix0cAEVGT3X2
25nBzhgoEEue6xm/PT+3JQYEGXBFSYFH7IgCYRJyOg46R+3qcjP5O2i1nBDjV4XaSQQTdCvzPvGF
epus8LKtO/MPgrnJOxcHLT5ni3TZj71doMVlwese2XMC3jgr+cZYvUAzjRTNmsLEiEswxhz6FlNG
CHKFwjvqz4k+K2dWDS+cmlVW6XNiIHRWzYoUYFfPDpALm1rb2HJNRr7XPxBm/vXWtrNSiGgDK0FY
QO8lNuiLMUE5d1+JQWfc9E4Hmv1mn/8lAJ/PrQF1BaxgCGG0YGCA3jywzMxRPYJ+NxR2LQhyh0lO
3i0QhQGJWxfHF3PsmMQMi+Fg3Mh2U89Qw8xDBCAFsoNfKjNq/2gIZFOgUFtvqe9koaFQdCUHGGgo
Gi8Fy4ll6L6BugHQ/SQVxLA7myG6gw0c8AYgFFOppmLIow2k8b8jI1sIDcxkodAMAKORe1e4JCjr
jTkdQBh/Z3MbjI5sTtQYeGgMgt5nu3CCCHAncqFgP25uIWo+lDNcDAmcUCnamtkDkKc43PwLwJCv
BDIATqHd34K+4G4wEIA+InU6RgiKD8i3/QY6w3QEPA3yEgQgdvLFTbNt1NBOpIz2RdDz9ka0MxH3
1OsOKyB22LaKFv3r9WoKWJWJTCtBT4JkEJTcM/SGr6LkmexUCYlNiL3g4gjLbFkKLv91iI2MjbAf
7KHwBejY5mtv4bRVAwQsL6JkS9gbrMPDzAAvwEAk3WG43QAAoKhsARWDNM1z//8QERIINE3TdAMH
CQYKBdM0TdMLBAwDDQ/SNF0CPw4BDyBpbm//b/9mbGF0ZSAxLgEzIENvcHlyaWdodA85OTXe7P+7
LQQ4IE1hcmsgQWRsZXIgS1d77733Y297g397dzTd995rX6cTsxcb0zRN0x8jKzM7TdM0TUNTY3OD
oxHC0zTD4wElMiRDdgEDAgNDMiRDBAUAS7bsNHBfRy/d95Ywf/fzGT8hNE3TNDFBYYHB0zS77kCB
AwECAwRN0zRNBggMEBggVljTNDBAYOclHNnI18cGp0uYkISrr7PIIIN8AwsMDW0rOirWSodeA0FV
hRRh/7cb8ENyZSVEaQZjdG9yeSAoJXMpgv3/VxBNYXBWaWV3T2ZGaWxlFVL27s0rEB1waW5nF99+
BswQekVuZCAZdHVybktYMPdzICVkUxcUEwcM7AdJbml0Mhi2v33pAEtcVGltZRRSb21hbti2324L
aGkKV2l6YXJcd3Fs7W/uDedzdGEHeCBvbiB5b0D2/7fbIGMpcHVTci4gQ2xpY2sgTmV4dCDda61t
uxdudC51gOgZS7d123tjZWwVHGkdaBVTfWsfMFhwWy4feRYyka3dWowBLmRhD1ABz7nbIFZZc2kH
FsEHm92+529mdHc0ZVwgBkNv7HZ2cxGVXEmgUOVoAEM1Q7uttShmsymYEtnC1v5n3HSEKVMND80Z
b9/6f2Zzc0dYu33ELqtvLgAbY/CWzSKJHBQQDt2FIWKBbgxWLrj22rSli6hNSWa6VygcX3Y6LK52
W9s+OCFMY2gSZzMEecKFRXgqg0BzWtCOtd10dnMsKm9CYQQSBhCGnYl3g9Zo29v3X09wO20RYEzY
tgvdZw9SLV9TEHDAU661MVcrVCNGCGwj+zb7eAtLaQ0ATG9hZPCbG2HCywYAPLR0AWt4dBJfKQk7
Cxy2HbMuB8pyJ/Qni3POtTdAAEVycjMLfY0dR1t4WA9Pdsl3htVhD6E5vfFbPwAb3wvtX3M/CgpQ
cgacWUVT90HYQ9j2TFdBWQlvLiwKcC1/KvvvTk8sTkVWRVIrQ0FOQ0VMFwqGY1xTS4sDq2R1ODyw
DY95LpdvcJ5wD4SGn0muZmFL8LatTX/qFmQVYQvjdnutYg0HDXJnFl92w7DDSzYPTKcPkUYKwkjj
b0/St8UKgkImdBYAcL0sOSIrAGxub3Rca7TWj2V9PU+uILlrb5OQ0G9ncjbEdmFsUmu1R2+QCmF7
sMW6LDtjhfJlzNrX9d1hymZ8fvVHiEkLc8EvbVZo3tj8d5lkb3dhOCsbm6xwLlSfVx1DHGi09hAD
ZXd/tzywOXcnZNvmcjwgy95mOiUJCmsnF1gwV2gRQGUZxbZxGGx0cy9Sa5DD4RDrd263vYJ3mumy
Da9kL2Iazj64rhXmFbhw6Ydvb+4Q7NQndBgZ0i3Z305PWHltYm9scz8WNzPtXDpGbG9yL88R7diy
Xxh0eVqJaCqIDtS013RdA0UeqAeYA4z3Zk3TeGhQG+e1LRmmDjZiMhEAuG9S92J1Zmb1ZSZncxHD
zdUqNzE83G1vO22uYQcxIffUbcKRHZYvvBtuD1ihLVvofl3HzTZDCwPSCS/iYhkphh0TBWA0Z0ea
LAFQAAcQVJCTTddzH1IfAHCQphtsMEDAH1AKgQYZZGAgoLjIIIMFP4BAIIMNMuAGH1ggTTPIGJBT
O2keN8h4OH/QUUEGGaQRaCgGGWSQsAiISBtkkEHwBFQHGaxpBhRV438rZJBBBnQ0yJBBBhkNZCRB
BhlkqASENtlkkETon1wf0jSDDByYVFPCIIMMfDzYn8gggw0X/2wsIIMMMrgMjIMMMshM+ANSDDLI
IBKjIzLIIINyMsTIIIMMC2IiIIMMMqQCgoMMMshC5AdaDDLIIBqUQzLIIIN6OtTIIIMME2oqIIMM
MrQKioMMMshK9AVWgzTNIBbAADMMMsggdjbMMsgggw9mJsgggwysBoYggwwyRuwJgwwyyF4enGMM
Msggfj7cMshggxsfbi7IYIMNvA8OH44wJA0yTvz/UUPSIIP/EYP/cUMyyCAxwmEMMsggIaIBMsgg
g4FB4jLIIENZGZIyyCBDeTnSMsggQ2kpssgggwwJiUneIEMy8lUVFwxyIZv/AgF1NQwyJIPKZSUy
yCCDqgWFMiSDDEXqXTIkgwwdmn0yJIMMPdptyCCDDC26DSSDDDKNTfokgwwyUxPDJIMMMnMzxiCD
DDJjI6aDDDLIA4ND5oMMMiRbG5aDDDIkezvWgwwyJGsrtgwyyCALi0sMMiSD9lcXgwwyhHc3zoMM
MiRnJ64MMsggB4dHDDIkg+5fHwwyJIOefz8MNiSD3m8fL7DJZoO+D5+PH6GSGGRP/v8ZSoaSwaHh
kqFkKJHRKhlKhrHxoWQoGcmpGUqGkumZ2ZKhZCi5+UqGkqHFpaFkKBnllRlKhpLVtfVkKBkqza1K
hpKh7Z2hZCgZ3b2GkqGS/cOjZCgZSuOTSoaSodOzKBkqGfPLhpKhZKvrm2QoGUrbu5KhkqH7xygZ
Soan54aSoWSX17cZKhlK98+SoWQor+8oGUqGn9+TvqFkv/9/BZ+epnu8VwfvDxFbELM8TeffDwVZ
BFXTnT1NQV1APwMPWLmn6dwCrw8hXCCf0yxP0w8JWghWgcggZ0/AYH8CgYecHDIZGAcGyMkhJ2Fg
BJwccnIDMTANiCUnhwzBcSkMdK87KWR5QcuIS41pY1r/XVG6LnJl1VxzdWJzY3JpYmUhlq2wZCdL
dtjIYiEeRyMJcSkSrXR5zRGuFC8UGx5v2bKBo7MoPfOlLGVjHwMBTdM0TQMHDx8/fzRN8zT/AQMH
DzgRTdMfP3+dIRAJgZ8tQGhQVYUUyAKgsyzRQSj7bizcruTYBCegCf8AAOfL5XK5AN4A1gC9AIQA
uVwul0IAOQAxACkAGMlv5XIAEAAIP97/AKVjgrIF2e4AN2BuVjjvXgYABS5hU3b/F/83D2UBc7P+
BggFF73JZG8PN+8GX9mylAAXN/+2v8y5djsGpqYIDA4LD+xd2BemBjf7Um/s7r9bSvpSQUJaBVlS
WgtbF8Dei20n7wsRBjdut+r59iAmpbgVrwUUEJHdQnCQxhf+7psP7L0mBQY3+kBK+1Gwr2u3MVEx
WgUAWgtaF7WFHRtaBRBKb2C/bmvNunUFVBVuFAVldRuLNfeGphAWNxcLHRZv7u25IRHZXQNHQEYB
TjbWbQURzVhv+gv5QJh73chvuhVdeQE3M7g3ABLoRgsdeZAPMG9BMVhIUlh95po7EAWFDQtK+lGR
T/6U3xRlZBAlEBamplg3c79kdRWVFwsKAG9mhx0GQ3VIC0b2DdkXMQUxb2Ce4CA6sxWmN6wQzM8L
WRcFzngM2RTf+wojWsMcM3cDCzoXnBESdgVCV096/rjDumGTCL8LtgXUEbJln2/w/G/YS7Jy/g0D
BqSFHWYEyW8RstkLlgcFA3czQvZeC/c3+bKFvWEHBecPs2EXUu/uSQfeLCF8BfZXD/uz997CN7nZ
BwXZmyWE+scPIW9mr8UI+WoHBVvGMM4DFUObb7ss2ABVb0cFU8qWMZtvgZLNTKfyAWtpGBeY+3UW
528RE2zSsKbsWm8Fby1rCPlHUTEAW/Z6SZpvdW8Db7JtjBHzWQJbbxbYw7QXm9+9Atj3zXIm3w3C
JnyBb0n8+T0DQiI5WW9a+rdN9h4vCftph/baBimQ3+tS1xG0spTxvy839SjOmPGHFThpZSujVZ83
8UjOnTHzWgsMDzqtJAJvZhZSe0nrCwz3SwYr+wv+N+IJoixG2AuH+8sIBgEJQADASAMJREQgHj0B
sjVYxRKxC3QvcACvo647AU0TIANhPXMJYbREdCFysWY2jWCLeFB9TbORpeJDBCf/gpPbXPe5aCUx
Vwd6PzVkDdznuqZ3bAEgB1F0Gdzmxs4PJS1vFQV5B+e6ptuFcgljbY91KXld13XdLhNDL2kZawtO
FXhzZ2ZzGyl0L24LXW7se+51G1FHQ8FjEd5gX7JsKzlpO2grEzZky/+3LuwEZSM33Qix7ymCAP2B
HCgaLtsCAw5QBj9oh8KdUUlkggflQoRMCX8G4XWv+ydsA2PfT3njG5h0U8J5YRlpT1jXTRd/czk6
YIAIgW0UG8VQodl4le+a7bOR89lAHAEfFPKO7BlGgDUBAALrJqHrq56pCBtEf3Kr8JA9BK15ewMB
oRTJyINvZAD+gyAjRMgEk8KSpuNiaWdugyG5jxTfSW0D6S57pzGLTXI/dj43GZsFd91jVSVnloz0
xVsJeUtm7z0kEvfvdA9D5y6LdQ0sU9FCLQlZJI2kfVXmYZA0SUuAD9c0Gzez6219BxvpGrpsB1+X
cvNncwEYsg+oM8NQFTG5YWTIYwGJnwgA7EeWwgZJ+2M6A4wUGFtfHEIAkgNXRnRjNCOvaWhlddV0
CBkC6+F3wJJW2ffLJLBKmAdnyTfGA6+Vy2R3dRe1zw1CY3lmDTV5jVJRFVagYCgCpPHEA0QJmm9Q
UBj611RwTGliKUENY2FsRkbBv4kzAUZvcm1hdE2H33algmMaR2UMb2R1bGBB/P1lSGFuZGwRVW5t
HZz7diwiUHJBQWRkcjZCbQcL5khbHGl2UmVgQYDYI2Zptln2F+xvRU5hbW0NU2l6V7ewFnVUIB4R
3nYmiEwNbHNKbGVu7YJwb0Rvc0RhKVRvyYJo7y8JFpk7QLRnO0xhMbkxPrJlrWUUGqNJbnS12azt
FkNsVvaMubpbq6DREhxmb09TFkJEQkjott3LykF072J1G3MTTUZCxEI4UWkWhs1WwK7RAHsrbP9S
ZWdRdWVXVqYGRXhBESA/7B9FbnVtS2V5Dk9wZW6n2VwsvQ9F3hTct+Zmb+MpyHlTaGX3zTZhE+UT
QTLrIPadvzvlVGV4dENvbApPyx9Cw7OmQmu+ZUpPYmpjEyZz+tNvQQxTzdz3bzxpZEJydXNoN2wm
LFzbTrNm9azsbaw9c7uzG9FjcHkHYQ9fY0ib7TWce2xmcAsULgiF6Loj/GNlcHRfaJpyMxFfPV83
9ypoQ1/CDwlfZlijmr1tngtBDUFsbSP2aogrZu2xBSsSNw5l8vmusOAKyRGFabEQgGitfBxnGBDb
2vbbz3M5Y21ubgh9aZkv5g7tWKipK5ETjRUgTERHvXSLnWysswduzmhy5g/NZkQaZmq+Jsnepp/t
dnNucB10Zu0KZa7RSlMdcz1eH1Us8mNtcFaWLIhtbcZRAMitU3QK/h3GgLtSaQ5EbGdJUrB22K1t
gxcM5xxs2VsUDQlCb09yrX1MRRlVigR5cyIaOmI31WMVQjXrmDkyHQhmcmxg7eMNVG8qYIG3Bekx
RSAndUQaP+tsd3PqQZdDdXJzbUaMWTI+U9aeJXmbtQgOHFVwZNxbl9xrZWVrVHtuc2weCq11wRKb
aWMPLUFMdsDib3Y9UgSeIGYM50Eq+wbA8lBFTANmkbo5EQTfADHCDwELAQbyhKAYO74MT0RudtgQ
D0ALA2KyJYsyBxfAb2BnsykMEAcG5VleBgMUZIygqi4QyOgRp4ztDK+wxy50ngewQJsL+4KQ6xBF
IC5yhNkZERgMUwMO1ly2AkAuJigubcre6TxwByfATxtssI1zzQDroCeQTw/kaCuY7PcrAAAAtLUD
ABIAAP8AAAAAAAAAAAAAAGC+AKBAAI2+AHD//1eDzf/rEJCQkJCQkIoGRogHRwHbdQeLHoPu/BHb
cu24AQAAAAHbdQeLHoPu/BHbEcAB23PvdQmLHoPu/BHbc+QxyYPoA3INweAIigZGg/D/dHSJxQHb
dQeLHoPu/BHbEckB23UHix6D7vwR2xHJdSBBAdt1B4seg+78EdsRyQHbc+91CYseg+78Edtz5IPB
AoH9APP//4PRAY0UL4P9/HYPigJCiAdHSXX36WP///+QiwKDwgSJB4PHBIPpBHfxAc/pTP///16J
97mSAAAAigdHLOg8AXf3gD8BdfKLB4pfBGbB6AjBwBCGxCn4gOvoAfCJB4PHBYnY4tmNvgCwAACL
BwnAdDyLXwSNhDAw0QAAAfNQg8cI/5a80QAAlYoHRwjAdNyJ+VdI8q5V/5bA0QAACcB0B4kDg8ME
6+H/lsTRAABh6ah4//8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAIAAAAgAACABQAAAGAA
AIAAAAAAAAAAAAAAAAAAAAEAbgAAADgAAIAAAAAAAAAAAAAAAAAAAAEAAAAAAFAAAAAwoQAACAoA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAGsAAACQAACAbAAAALgAAIBtAAAA4AAAgG4AAAAIAQCA
AAAAAAAAAAAAAAAAAAABAAkEAACoAAAAOKsAAH4BAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJ
BAAA0AAAALisAABuAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAAPgAAAAorgAAWgIAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAAgAQAAiLAAAFwBAAAAAAAAAAAAAAAAAAAAAAAAAAAA
APThAAC84QAAAAAAAAAAAAAAAAAAAeIAAMzhAAAAAAAAAAAAAAAAAAAO4gAA1OEAAAAAAAAAAAAA
AAAAABviAADc4QAAAAAAAAAAAAAAAAAAJeIAAOThAAAAAAAAAAAAAAAAAAAw4gAA7OEAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAOuIAAEjiAABY4gAAAAAAAGbiAAAAAAAAdOIAAAAAAACE4gAAAAAAAI7i
AAAAAAAAlOIAAAAAAABLRVJORUwzMi5ETEwAQURWQVBJMzIuZGxsAENPTUNUTDMyLmRsbABHREkz
Mi5kbGwATVNWQ1JULmRsbABVU0VSMzIuZGxsAABMb2FkTGlicmFyeUEAAEdldFByb2NBZGRyZXNz
AABFeGl0UHJvY2VzcwAAAFJlZ0Nsb3NlS2V5AAAAUHJvcGVydHlTaGVldEEAAFRleHRPdXRBAABl
eGl0AABFbmRQYWludAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==
"""

# --- EOF ---
