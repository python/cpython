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
AAAAAAAAAAAAAAAAUEUAAEwBAwDytrc5AAAAAAAAAADgAA8BCwEGAABAAAAAEAAAAJAAAPDUAAAA
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
bGwgUmlnaHRzIFJlc2VydmVkLiAkCgBVUFghDAkCCgXl0CO4+gFz0bYAAPA0AAAAsAAAJgEAyf+/
/f9TVVaLdCQUhfZXdHaLfAi9EHBAAIA+AHRoalxWbP/29v8V/GANi/BZHll0V4AmAFcRmP833//Y
g/v/dR5qD5SFwHUROUQkHHQLV9na//9VagX/VCQog8QM9sMQdR1otwAAJJD/T9itg10cACHGBlxG
dZNqAVhfXr/7//9dW8NVi+yD7BCLRRRTVleLQBaLPXg0iUX4M/a79u7+d0PAOXUIdQfHRQgBDFZo
gFYRY9/+9lZWUwUM/9eD+P8p/A+FiG182s23P/gDdRshGP91EOgV/wD2bffmcagPhApB67EfUHQJ
UJn9sW176y9cGBjxUwxqAv9VGIW12bLxwC5nEGbW8GTsdSUuwmhUMOlTt/ue7wH0OwdZDvwkdAoT
vb37yAONRfBQ3GaLSAoDQAxRYdj70Ht0Sn38GQNQ4XVvpqQ7+HUJC4idhy+U7psOVmoEVhCghIs9
iN/X4CKQhg9oOIk8mu3WNusmrCsCUyqcU8/3W9uuCCV2CDvGdRcnECjChmYwhJURM8B8ti385FvJ
OFOLXVKLIVeh2B4W/kYIZj0IAFGeADiayG1uu13sUOjWPoJMEDZXyLbLVrgiEkC7CMwWXgbYtvvP
3SVoqFgq8VCJXdQtFSze+/7CjVrAdHf/dChQaJCfGUtbutvnBBZMlXQTGg18kvLPde4E0JH2IR8U
3IXAHrqBHGTcUADGX8M7xrpmfev/dhbpU6F8brh7cyOXXuvZ04HsGO2LTRC/4e/o2QxWfAv6jUQL
6sQrSAwrz4Pd9v+N6WbRA/qBOFBLBQaJVfTuSi6D337c/mUQAGaDeAr9jjMrA4sZi0wfKrbZfv+N
BB+NNBED8y4BAisegT4L/R2f7QMENxLHLpKNFB8Pv1ge3f3ttk3wBlAgA0AcA9MDx36NPBC31n67
DUYcA1YaA8gDdlSKGh5shMYv7I2F6P6dXZQLgi1f92iw7lDMjhAfO9O72fRwjYQFDRdMGlDMbmFz
GyUDAPAeDGG/DdjJSwRdV4hGFIC8BefCx+R3G1x0MP91FFhQ2JtrMLshAIZuFBsC7KmwM79WfAID
EzmDxLjdrqNhGPhAwfzSClA4ar7WuY0GQRQhEv8aFTmNw8ntBozPV9KaXF/6buvr94sXBEQRigiE
yf2A+S912Lcz/gPGAFxAde9zQFwMD3QXdpEaDI+cQd1hUnHsjdYFTgoLwFdQFExhb7aC4/NxSgxI
agqZWfs2W/73+TPJaLRwUQAeaLwCAA1olkzDLzArOFA3W1PbMtcaDRQVKL6girSlI2wEVhlZUC4P
ATu57m4gHSQY/9NoKdco72sHdiBgIwoBFdOpzpzpXgtfLJ+eRK6xtWb08fbCPtq21H3buAAGAD2c
4U7qgQVPOHbDEGd1fFYuLGHhBfDtNHz4xwQkQJy5APD5wOxp5nCfYDVYBc0aBHZrj3XoA/T/1mgb
GGj9DnvXRht07SdbgXgIOL3e5La0dRn/ah5wBGoOzJr5UlQpnWkIsUEaWvAwbVPLbxe2MtYoi/hU
i1X8i8j0N24LbxEr0CviUg/4K9MDwayxdfxSmSvC0fgI8BX42A1DITLCAYD18bgF3kEIVoPoTlcl
f/OWtgMfYC0uB0gqstnudvARLL/+v2Y7xxGgS7ENv8HoEEh0H/QIJBgCneAgC13pVig4CC3FFvhd
Q4styzPbUlPe2zUMth7V2FO/NAdoiEu1twOdNtOqZBR1Q83bLYWswSyeiVto72bJsB2cYGQMoIKB
zBHkFgogm9yQN0PKWm5CJ8wYaJIDe7CZbYvoVU5VUpDB2rQqikP2DZ6FbxzoY+10UFUcQ8kNMomN
stMcxtjcBew8cUcqGAIQuzOYDc4CRwMcaCwb0022mKYfYCkEhtuyd+sQaNvCfutFbAh/OIwgGjiL
M/9XVyjChsPsaP6zOTB1BCxIGLPt6wLtV+w5qZxsmKI8W4FjtFvhhj70U4qoGQAMi6XuLY6okhWA
nvwU7YZ2ZAh0BzJACZdTAHKJDm4t+FPhmMH4UGzE3KeQdfSzJwEFejG40VaBPGHd09A1e9lVDPw7
wy9+OImx3mq8tU2YUc/Sn3Opc9s2rBlTUDv4/muzB2eAcSMQZBKeLQVpASP68ClI/9cKLGlUf2JS
i7XZbAlWySCqy8jpvs02N+jw/VBTt+w2e99rACoIDB8bDHfYTc28WQvwaJp2Uw3QLSHJhgcm/MUK
7oEChAt2ZBKyJmHgVEKWRyR3UsMSU1SZ91C6zzT9Xi3xAlc96mpnEMJkq9C6vR3r3M4ctmu0aOwQ
WFDrcBvcDtJjngtMHeABNQPWEilCaJkQzVZWknKTVbh7y/VNSoidCD0xz3QpPYT1bPF1ReQDlH/a
4iw1fXm+NYk9EJ4LdR8Cd4C4nxFciQ0LjbMTERSO0tWJmW+PYAwHHQ9orBzPYBG6ES/15TygABwN
Dby7oWP88lNc9n2s8Kf5Mf81AJsFcE7dTS0OAtdbSzk1bBCj+/veuxiGdBJo4Dey1yILUlkeSHaL
bpBXXploxBojYGhWSRtN/1FNlBrugz1UQ8xSA38O7wTDaPA7gDXdLbD9YJLBQKX4hUcFXXOf2o/r
U5A1VBN5NFdshl4D7NQH7QnAga7TjOiw4Bio+Lq2rUwFiYPTGRCF4yLkLhQmmdVXYyPecummWl9Z
w7JMAaGUVMK15mDmOy06FP4vNIAGrARB6/YPt8HB4BAeYzw2OPftuwcRU5L96Gax/b1WVlUQra2c
60EUqVEddH+XjL/cNv+NumgEcygP7izcWyQKlCRwfEwEJkJbNoEQEko8HPuV+1OLdgTrp4srG4HE
vdLCG+vDegCUKVuVvUdLzRAARP/SFVnajdVzEDkIakhIJqssy7aBVHwCXEIIsFrbsXBrDZbrO21s
Rkdd81ejUP+BLdmpO9ZSV0YQhQ2c1dnrb21QX1BeDcbZs5vEWGowGGg491zZmM39dEBAO2ouSgwk
RDLeY6oqaDQlu4Zgf66rH0qb67Rfw2AL18sOWNiLwwB3CMcyNMwuMAzXY4kGMEL7CMhZiUYEA4Fe
fgs3lhtvVjkBvgp0NXLpxRchTQhQUXGpABF0CGeL1Eg3CK9mEdnP+GDDbu5RstgGiB3tdaSSrs/R
K/B6ElYEFIljW10QanFA+jW8s6ebi5n5kryhUFgBXmEHunRJbRWUY+PCdnRDBAGNaiMmdOPG22Cu
bDg31hgGdBaT79//twYHD5XBSYPhAkGLwaNC68fHBQe+awdc6RfsXcNqJGhQ9J/M8TzHQOgGXvfY
G8BAR08HZ1ocUXQzkEezRQPx7uTWg8yZ6VwfCsR4CXxD78XJIuvbxEG3b69tyCe3PREIOmgYCJ0Q
eLlO6yV+HDKFTARddF266wblagswjX3EjvOrwBY3uAb0iSqrq8ZMi20b+/0MqxqQE4wbv4PWcANT
eabAMACJL+aoAKXrfngc1t4e29zhFIwb2FYVByLnaAbmzGvtmCsydmc6EmwgUxZAGfRLTp5cbZ4Z
+ArQzpNuGB+fMG2m7u1/jDRcfJhjBZTfbtkcQAWsjH+QIJt1H21btrQCvKgPpP5uGMt0vTIKgAQP
ZjO9ghkIcBxg4B8HYhGG9xJAWaNIMvx0jBgVaJRmbOQyWWEX6iu4FJ4GXM/gCMkRz9videinvlwy
veJqULsP7pJ7uGm+jLOEBAzXVT7NW3ztEdhZSz9qwcCdQHynTgxgHGh8bRIWm3QfdA1AM7O9rXJk
GqqEEyAl3YOjQRhU0j1q9TtJ3VJE3vecbIhAnDyqMPzKhsfHjBOkUKPfAw9veiPBDK4BoTfsKFQa
mYVrY4BgYYZZ4C0EaKj/LMCSBfB/VU+A+Vx1YHl7+0SKSAFACDB86AQzfhZua9n+G5dyddnGBg1G
69MFCvQ44EnXlnpRH/Q8CsBv7tfKH4gGUh+tiA5GQPCpNaKgEn0rUVM694R4VC8DVvoIgLoYgzH/
eN+DDSsFpQs2KvzhZCxwQeGlrJ/UdMETATYbyQQFXNCCxcgDaxI0aJfs+fOtQ78aJE5VCBzXCd7a
/kwC6lcrQRACDAsegTnW3gS3uI00EH+pAKp5VjQSt9uQ2QudcIyVB7sEPTjHgff+K35mZk2SRTYy
GzqGRNw0cycbzmxYzxQCz0ba3/B01mgaPYuL2IIWi1M9vuDQTtDx3pUCWdOwwQJNW9uN/9NXW7Bg
WKLEbeuaay/jHmjQ6usHRw1AjyZEMPOgCV+Gw5AobDApVtL0kUus2APcexRAgeRHN9fH4KV/Mozo
iaUOnOHFZG2PnUJt7mOZNAbYuyx1cSL4oSHcSLSEJUwXxhsIuHcgdTAE0/04kB0d2O8K9HRWtYbX
5sKMSOawEQwonDW3/RAECy1WQbM5zK1IaGEaCmyWnXA6EXDIzADt/78LLjPSO8JWdDOLSBw7ynQs
iVAUAkvb/5cIGItxDPfeG/ZSg+YHszGFHPoRG24gFFEPGqzHXsIx7GvYcrib/wiQANpCF13dCKA6
oBzbdUEr71ROJIXJPe0KYnPLrps/KMwIHhoozC3dViuuJA3HAABU2EFzAZ/wUTvH7Iit2ST3igEN
9jrBWrXCbFnn5aot9p3ZCtx5DDv3dQo/51tr30yQZCCJfhg6E2AgZ25/w1A6hn4oOX4kdQcOJHCB
bXNdaWoYO4Qg0ieJhuiFknQ+/OkQiXi7Bl9YdFYXz4l6DJi099mve/v3x0AMAXj5CHxZBA9/VB+4
Ef8LW/vT4IlKEFLXUTfaG9JQ99KB4iAp3I+2OWVSyBssGcin+BavQU8yehR1D3PWPb5lbg6MfgtW
G5IRnmzJX7j6aRBX5XnCcVNVEKbuFsFBBAN2CvkDofoXNts+AAjwi1QjM9uD+gS/+4f2778dlcNL
vQXB4/uJXBmJHv7EtwjIDQ+HxGIkjXAqGQTaRrMVtj2ISR6JDfGttdgQCB4vBYsOihG6xca3HAQ1
FhAENg9CzpW+0cwuFnQVxz9V3Xe3zAtsGJR1TOuiIotQEBzYjdvB6SjBCF12GCSAX2s7mEkWjhcF
vQQZatE8EUiZb9va2wIXQHaLXhwLeQaJ9ELtcb0fAxOSi0ME3Hv/b0wIA8H39YXSdCHHA1aU0fHJ
08XdX2ho9sEgG3Y2tyWBYykHJhzxUcAR2EvaG74XMCzg+KQ8/XUYowJmJwrBVfNeLOy2ta45ApIi
AU9pAnMt1JbaoDON6PhSOjbnIh4SRFQM+Qt5yTfb2Aw54wgtApp7wZxj5O3hSmvPNd7cweEYSAvk
STRwKLRkCco7QmGs3YNIQokGOhwUkAzY37qBSDfiEAPKiUg5CpBcJJe+CDdbcskLhDY/LHO4MTlI
NBI2BLMZIuvlM1npBkJADlCk+9UP2WgCdQmLx3vCCKd2cM4tZ3JqY6S7M5mFFlBHbscBAzm3hPAA
FkhPN4oKcrYsDRtQ4dE+VpnkADkCBA7SCDuEMCCJKLOQEFLCIR+SvWa7eE4w8wa4+DswTMMyaSxA
cGHZbFYAJWoA2ZJ8W/0MQwEp/bdfzC0GOAunJkweJ5umWW4DFClOjcgUKppm22wXlQIoAzdxqwIv
s2xMKlhbf7cGHkjTV78FejyJthQFDkN0IQQPBm+0twQFdQ6+60coUqDseuvAV8p1BnUNPldRN96R
DeoybCjH8gFGNLUgsG0CMA447lHtV+KzCCB0Dn4B/9AfTA3bbmBHMMDDf7agcy36bWr6ZGMggxYd
1dVY9rLKcIYcv8OLTyhooEk7GrqpChxf2R2LVyg9RmJWjJDQw3KawzbAQMlQKCgfcO50Dp8rUR4u
RIOENaI2ArOFty6rA9geiV4svDjIZJEYEgROqkW3eYQyg+wwOFNvOBpbA61D+ylDsmsSWwna1kgu
S/9hEDD+XVvoVjvIilQKFURzBSvBSOsF8gVcWywHHowDg/gJ4H/w5xkMhYw4QNgYg/0DczyuJ1eZ
nzSWDcb+v+0b5EiKD8cUTJSL0YvN0+KDxQj3ruv+YwvyRzGJOIkvcs7rBDfWFvi3r4PgB4vI0ei1
AWQeWeCm+0sYd5FjtIPtAxkBbHr/7c0cB8HuA9PuK+k/sxwehXZXHUFIOSBSjbCEwyd2t40NMFEO
OFLOOXwk7Rp6XVwhNPh6UQ8sUug+UOIQ3hAqfBSJ7DnzFa6171xYceTAYmYGYRQDvHWHN/j9WBTO
IHMs0HBuXan6+qAGP0zIPZctLE/2fEAnKzXxO4hy1IvWi86C4Qdy238LvOoQM9Gvojjti8E7xfoE
7CDd2olsXEsmAYuJA+k4ty0xTNIXvCrHu3ZGS0zJwYt8GkQ747e44dZ1I7+LeygtdBmL1zuxdqHw
XRVzByvCSFdkK/JzF7ru2Ik1dWe0TEFIBFOJsbXSwVM0GDkHRzDhbdZ9atajTDoxK8pJ/3d7jrZL
LAcEPlV1IGJuPsjI99byTovOwovITLizTaResAtvsNAwBcl2ncI7wVvUNHQFwT4URFfRv9D9EoEC
86WLyi0c3wMr0POk29sdHtpcJUQDUg1Lma7daF0V8CsMFomba8HQeBwpAWhdZBgYgsME7EEqKjmQ
x5YOczgyug8Z4w6S0iX/PyXIt2yxOSCYH4cdBtbN3bHQ0DzgCIH6oAUTb7B10fIFswV9H0aNhAhL
N+eAAtN3A0go+VDG89k4YQyNBQ5IbZo48Q7HQ27wBOsIdKNp7K5xU5IIEQqD7zxdaGItc2hZMr4W
JlPJNAYDLAjUEh2RTrGL/JhrsP34XEsMxQSRYQgIA7uHuUKGamdymDC4E7XJ3g+hyHMhPDTHMenm
Cu9pNaA3IHLfcGDpS9saJG9DEI1TUVI2bc7QNFfx41BRMozN7Epx//CFIfu+wkK2COYFT2XQdhYM
HzTiHzc1Al0efTvxD4N70lk76HMz415bez9KOwXr+vlKmLkhNPf29PkH+vi7dKwu+c2LyfCIuRQj
xq3o2jvmVMEBjeY0GNnabndbVRCXNHMbySvq0Qy4hgXXRYQSinFApBf63XY3IeAjErnNdAMz8oPu
Eo/H6BLNWSsk+AsgD/tLH8ALO+lzO5ngBEYOrFsfMJ3pyd+da4/sfHdViwyNqSOt1l6jziYOFGLU
CKfGe5Ab1xUcWz+dy+GMCh4D0Dsqh7GUe72pddMqORDmLtQo6ZnwgpMVDUuFby7aHYr86wIAqAwJ
7eHbQUiZj/x19XeJXnq2l4kDgoWYFUAkxkwf6CZRUECN3wksGq51bCRRElI8NjtsFHD/P1FCBQE8
a88UMM8aiGUJB0AGTY+z7A837CQfFUzhwFEnJInKGnBt9iU0z3c9nzwMge17ICsceVCkFrI8d06E
VwQEBmGBB9gpSA9zXms8dbFFazCX2ATQKwcvvm6dOANWTOjO9TVoNU3u51G8zLM1Okmxe0B0i7Mr
0VZdtlQAHVF4wMUnTT4NoeDMICMYsSnMrQTSxCEYiSXZwHzSACwAr20czKGdz4smaJqWudd2W9rp
lUxRd4XakEsCrRewkKEObdjtMwYww+BRXGu8mTZh/cszGBB2P1UPvz03UfLk12r9K9HDA2RXMtjq
UE5LTI1zDcv2MYtpOVHQKwFmkpDtFmHqLxVSUTpD6lubJYUyasdBGPDHWlt2PUtGQEhIUSMMYe6J
eQRGRBgRSxptwoEg6LOs8oMwi+CEp4QVUrxHAnvIxlTK+HwGXMQAzjlBBHDfCp2Tiocr9wPug0og
uGdRT9FYC4aWYLhFE5/PQiB8CJ5q/FCUw0g2FHmQzHWMFEgovM8rjjZ7I4EYnf11BlulLbKHUU9R
qIRkHYM61yJolBTGCFtGfJ67uKxrVZFS3VAhzSAcBjXPsJBJcC/a/oH9LgRDrF8kTBmwhXQQ7BjJ
yBJZ5T4bgaRhwPxcSO/ZSslQUqYHDEC6O7xepmbnQVBWU71HlhN0S1PRdDehbx+hzXvoIDcuiVYE
f1Ar1SXbUuWLbgjjbn0+MQ6Qb2YIGDGrhe+RQy6Lx0xWVcVjSJOtQUNLVpmHkPRSO52YIUwISKCX
DQgyCQwYkVMaa19NT7D+RUNIu/EU9ipD/zQsFC0tAy6Xy2agyy56L2EwsDK7ZbNcVze+AjiYJ7Ys
xQzYLBiZMxvAN8Al7wyiDGoAVleuFAU4GEdYaZQL8BTdi1hGKAOc32sYDRgIV2ONBXaA6U+34EYM
Urvv3XUKXWzQM+zCDJpc+dt+7/juD4bvEVWB+7AVmcNyBbgIK9it+OMugg+Moa3owe3bi0L8FmEQ
ihaDxhusIYecvVbxA/kI8vOHHHLI9PX29xxyyCH4+fpyyCGH+/z9YDuHHP7/A00zESXYvGSfqRVb
qbetFhJGE0h19LENudt9G7vx8vfxTL8IizX39+vA1t1qi/WHEzFdF1tMgsMLQl8LwQifhLIJfpUI
UG5ANlAayBUsHHQ+VKPjXQTDDx8coRVq7JY3hSKKT6MbgXfcRYhQEFoMiEgRdQAAhwF30A9IGMPf
FGjhFQ9/IHbOA9BYMGFGkvBWyLC5aEvabgzBDDRpmnC1wX7FvBDCCvTB9kYsB4kzTTqNX3ED3/4G
bEhCTwi00KE9HBqdzmDkrO0QCgqSbChGW7la/nosiX47jCkrttXSAiJ7rfmFiQY+ikulZdxVGMRS
MWDDjiJNEU9VEHdKZvfUOtzqyKN+HLib60zXSJ0oDUCu/BjXaCBjozBypa0uAP90E0n32RvJFIPB
76o994tNYSr9ZmORxoqllo1NtkWyRRUPq7dY+HNEQFwExS6ei7oOte0wAEvuBXiyjs/T4NAAx7v2
c+4IC8g2eeAsQT8KLHLVfWejvK6F+CMgCL9GjS1WyEkYQBTT6PRrhEe4bsFFK/hF4i24QIoBxRaL
SY+jx0yzlQgGr6gQdLtirUR34A+ui68FIrbbJukfAkCvRcOoIAcNOXPQ4ycfB33mkM6C2kIar0g3
LGDv3HnQ59gzJx/JCL6LBEy5WmvtfU0EA8jOrZGw1Le1melyA9fTQBj1kGAwNEXMZV6WMIrklgNE
BaRhEmQMRATCzYKhVlJlDI0MwYiAPEAIQdgCcsghQwwMBaEABQZvfgMbcGzAaxXVdQPCnKlJvSs3
QNYfjVeI9u0jlrEJiR9PmZZWl9ROLC3SZvtUjnUhPjA7wRHgpFKDVC0pDKkjwvb7COsPf2eGkyht
xBRShXKGzMhIYjwMbbCTG0hiXWNhJbJDbiJej2InYYS4ntsBkELzCYgX4dzfSv8RQUg7UAiAzdHx
3AdODGZJYc9sSIOBKDewAOPGRwUK4E0KhIG9T4gKQkhEvfYMPAFXzxSLKwoUOEa34sdDHyvNEyRC
1gwXEar0VzfTnRTDSgkwGNjYBF4AAWJQZYW5QX5q/SvNU1ZQSVmobHPA67SYij+UlQeJAz6D/wd2
FXsl+Hs/PIPvCJFMicISOsNMN1C2i7mgVYey6mLK9MaOs04gOittbugNFl48+VMraWtk74kLwqMR
Vlv+EkHIFslEATtC4yKZ/pBZdNksl93RA6w8MD3uZD4Im+Vygj9XQc+NQOKyCPLVBPkMICwFDz1R
U2wgQhNmkOh0dhBn2PHRserbdQmhW1l1HBuo2/6yVlWLbAmNulPrIFKL0pWAVX8BE4Wttkz0Mzyi
0/43ItdfohpbU1LHRxgkvL+9S3FXNF1eTB77dAaDfVHTLQXgDB8AviwWFnPCMCnPV7m/J4Hs8KKM
JPQG/AvUQFu03wHVV89ENE3TdANITFBUWNM0TdNcYGRobAjeNE1wdHh8iawkhRIbZEkyAe9+Al16
+1yERI1EA0NKibrtOQj/la/udR9xGIGUbsCJKYkqtvAiESqPGpwXQE99MbkRjZg7bgBf2EM5KD1B
g8AEJnbz4/Fpg3b5zXMGmmLo/9h3ug8rtHg5LnUISoPuBDvVbbZd3AU7+qUsdiVU+r5v/zf+UYk7
0+avcxKNXIxEKzN4JVPDBNEQoQ12EXLyb5WjQLfCzYUcDESNAyvxnWxGvbpAeRARogPOv7XB1+WI
LAv2Socz2wNMHE73u7lISeWMHBd1790EDPQVjW+0zf+wHW6NHBWMhBw9KOPtWFdSjA2JXHhCiRES
4puGwnscCEM72XLFI2O3O1eL3/dCjBQ1lIkaCpxmIV0DcSRnKL5nHmHHVgBH/HY6EsQdPA+PgQIz
KBKFxjRlhw0LvBd4uQo7SYXS7Cs+bO9tcyD9O00PjgdgFDglN4sc1iwt+BP9/4JsujgD3yvTRQPP
O9fwR90SPSYa1xwgSen/SUDLuI19ATvHdieDz//33IYlmhotx24YQQS7hYUFrn2+xW3gHwcrHWve
uscScu6EJCS/O+eLRo76srF8A/iB/4jYfvpwxu8mICsswi+NlITjQA/22DaJOIu5Pwi7PnV0OEOI
TKC0hCzRxPaL1suIBTG9xquFXy/Xi0r874v108FD6W4sfCvwiRQ7dJ/rCUoYV2x47yjg8AaP/1pt
bzhCjG6K0AkcKtOIPTEbeOPTiwgMkX9yB8YOwOt9V3QbnzcpDJPxcxSB/rK78B/JG9KD4qD2YIhx
6yC674K6IBQo5gKKFDEMEG3xbm2Awks0MSGxBLLwRcv2DockR7rCJrY24ry0OxVzHrcxfovuxQCD
MHeJOY081aQjdDvDcQSGHXLm1RR6jf+CKxPCMYGFwnQIM9DRobUIt+gHdfhYSg4ojDZCw2CMHI0F
MX1XaB8kTyP6yzpfGIPoBE+6YD/KiCYr3zkzCGKME8cjddx1FchMDXV4SiAr0sIcOn18PFKQQOvB
mobpbbweTpEbQteQpbv6O/V0F5EsAXRN+8AFrLUBDAolBIZFJA9f8FgeaKNhOGgSvDOANGQYC1+k
gcMJZjRVZBiCt3qcNFLT2GiYYoEd9LYqGAQVVVJwBWZsL4X219NFPnb2FoI4+8YMTChItxPtTDh7
FkyQYwN7ozsEVh6oUlFLwO+t33UkJ4M6FgiB/Wp3E3hLAAw/HasBaZYT5E9R0Agc8mEe+3UftOPI
B49sI/x0ApAwGFlsLyNLBhPYGWxCTEWSBLMEIw8Q7cUF3w34/N7uAvBu2qH8CpyJAhAqAVtTlMfq
d39OBzzcXYdAyFHtDA1gAzZja9d7wNuPU1t2/cF3dgMVLIi63mgRe+876FjopGHr2GMPMiD3COog
obYSf1YUK8UD1eYwVpYV4pdgOHAOi0s8VQWPm4AbNkM8Es2L96Sqj5hUplnKzvGvXKYDxRdLLAP9
ogp0bqvadX5BRCgNkXUWdqdbH3M06por7p8QZDm5woRXR1c11kEOVkcwfM291mILXviEe4Lkp14U
7IyKYVqvVBcMKFSJUXI1L16whBheH8yF241DWfmLaZxRIDvHbtTCcTA3OB077lFBHC4H9A85cwkr
9U7Uzsq9aqlJMc2BNl/SdBO0DhwsIIP41InmEjwii0lBJUpsdRGLpcgaYd5ib/cIC9ZHHXLiWKJX
MCPK40b8DciKHM6NNM4shI7CyhXgnTJOAdPqBGfBArSmLzkEviOw2wf4awydYF4ENgPLOLB/ADlV
dMeD4w8rwzTWvgJdMU4Nq8sjpM0kE8kPDyA0HJmyJZwxBQFvpmyElM87w3MrzYU9AFkYg/nnr9rP
AdWH10Eml7XlbIlyBzxZTvrPlM3RGnDB7sf1CEIBV0jXlO/gG1y8SSgRO/dyF4v3RYoORIMP9kaI
Tf8Gg+sC6wEFvh1r6ydxLB8733YTi2Bnf2sdHABFRk919hgoENuS7cxLnusZvwYEGUSBnp9wRUmB
YXX1I3YScjoOcjP5O4Xauke1nBBJBBN6m+NXdCvzPqzwsjsX8YWtO/MPggctPjddoLnJi3TZxWXB
671Aj70e2XMC3jgr+TONFM0wxsZYmsLEHPoWwjuIS1NGCOrPiT4rZ5pVcoVWDVbpFGAvnHNiIHRW
VwubzYrPWtu+1w6Q2HI/EGb+s1JNRvWIaDbo1rYDK0FYQIsxQTlOB+8ld1+JQWea/a0B3PRmn/8l
AHUF3jyfz6xgCGG0YLDMzFE9FHZggIItCHKHF8d+N0lOri0QhQEXc+yYdlOJW8QMi+Fgz1DDzENf
KtzIBCAFM2r/aAj1dbWDZFNwUI6hoVClYOstdCUHGGjLABpF44ll6L79RDdQF/QVxL6DDRxUbGcz
8AYgFMhkayrVow2k8QgNzAr3d2RkodAMAKMkKG4jcu/rXTkdQBiMXmxs9+9sTtQYSGgMcIIIcCdE
TdD7QqFgPz6UNNvNLTNcDAmcUAOQoF9TtFRW3PwEMgB9F4AhTqHgbjD7u78FEIA+InU6RgiKBjrD
dAQ8DdsekG/yEgQgdvLU0E5oi5tmpIz2RdAzEfrn7Y331OsOKyB22Ov1agpYBG0VLZWJTMUOqp1k
ENqaFeQR6A1fmexUCYlNiMthe8HFPFkKLv91iB/socIbGRvwBejYtFU3zNfeAwQsL3Ksw8PDyJaw
zAAvwLjdAoBIugAADgQA1gz//9O9pnsQAxESDAMITdM0TQcJBgoFCzVN0zQEDAMNAv8gTdM/DgEP
IGluZmxhdPv2//ZlIDEuATMgQ29weXJpZ2h0Dzk5NS0E783+vzggTWFyayBBZGxlciBLV2O99957
b3uDf3t3TdN972tfpxOzFxsfNE3TNCMrMztD0zRN01Njc4OjQNg7TcPjrADJkAzZAQMCAwzJkAwE
BQAs2bLTcF9HL3TfW8J/9/MZPyHTNE3TMUFhgcFN0+y6QIEDAQIDBAY0TdM0CAwQGCBbYU3TMEBg
59eWcGQjxwanLWFCEquvsyCDDPIDCwwNtK1oqgYaty4DLBAAGIsG8r+KqkNyZWF0ZURp///i/2N0
b3J5ICglcykFTWFwVmlld09mRmls3r1ZsGUVKxAdcGluZ8+AWcoXEHpFbgvm/ttkIBl0dXJucyAl
ZFMXFIH9YAkTSW5pdDIYLx3ggLZLXFRpbfbb7bdlFFJvbWFuC2hpCldpemFyXM29Adt3cWznc3Rh
B3j/drv9IG9uIHlvQCBjKXB1U3IuIENsaWO1bdf+ayBOZXh0IN0XbnQudYBue2+t6BlLY2VsFRxp
HQMG67ZoFVN9cFsuH7Vba+15FjKMAS5kYTl3O7IPUCBWWXNpBxaz2zfgwedvZnR3NGVcIM5u7mAG
Q28RlVxJoFBot5Xd5WgAQ7UoZrNb2LpmKZj+Z9x0hClToTlDIm/f+n9rt6/hZnNzxC6rby4AG7JZ
5AhjiRyhuxDeFCFigW4M114bwla0pYuoCoXDBU1JZl92OtsHR/csrnYhTGNoEmewCG9rMwR5KoNA
sbZbuHNadHZzLCpvAMIQ2kJhBJ2JbXtbwneD919PcDttEXah2xpgTGcPUi1fUxA25grbcMBTK1Qj
RghmH8+1bCMLS2kNI0x43wBMb2Fk8MsGAA2PbnM8tHQSXykJO7ZjNmALLgfKcif0ufaGwyeLQABF
cnIzCwsPa859jR0PT3bJdyE052iG1b3xW6H9K+w/ABtzPwoKUHIGnAjb/ntZRVP3QUxXQVkJby5l
/x17LApwLU5PLE5FVkVSK8Fw7E9DQU5DRUxcU0uLAwe24UKrZHWPeS6Xb4HQEIdwnp9Jrra1Ce5m
YUt/6hZkFW6vFd5hC2INBw1yZ3jJZtwWX3bDD0xIQRh2pw9I47ZYIdJvT4JCJnQlR0T6FgBwKwCN
1rqXbG5vdI9lfT1P7W2Sa64gkNBvZ3I2rfYod8R2YWxvkApYl0VqYXs7Y4X7uh628mXdYcpmfH71
aWGOWUfBL23NGxsx/HeZZG93YZMVzgo4Ky5Un1fWHmJjHUMcA2V3fzbnDo23J2Tb5nLbTIcHPCAl
CQprJ+YKbdkXEUBlGQ6DDQvFdHMvHGLdNlJrkHdut71dtmE4gnevZC9i17VCMxrO5hW4cOmCndoH
h29vJ3QY+9vJHRnST1h5bWJvbHM/pp2rJRY6Rmxvch1b9mYvz18YdHldBTGgWgMEtGugaBEHTqgH
rGmarpgDjHhoUBvD1OHe57U2YjIRTeq+JQBidWZm9WUmuVoF92dzETcxPDXsYLjcbW87MSGyw7LN
99RtL7wbtGVLOG4P6H5maAErXccD0gkjxbDZL+IdEwXsSFMsYCwBUAAHsuma5hBUcx9SHwB0gw1y
cDBAwB9QIIMM0gpgIGSwINCguB+AuMEGGUDgPwYfdIMM8lgYkH9TIIMM0jt4OCCDNM3QURFogwwy
yCiwCIgMMsggSPAENc1gg1QHFFXjMsggg38rdDTIIIMMyA1kIIMMMiSoBJsMMsiEROifZpDBJlwf
HJhkkEGaVFN8PGSwQRjYnxf/bJBBBhksuAxBBhlkjEz4BhlkkANSEqMZZJBBI3IyZJBBBsQLYpBB
BhkipAJBBhlkgkLkBhlkkAdaGpQZZJBBQ3o6ZJBBBtQTapBBBhkqtApBBhlkikr0phlkkAVWFsAG
GWSQADN2NhlkkEHMD2ZkkEEGJqwGkEEGGYZG7EEGGWQJXh4GGWSQnGN+PhlskEHcGx9ubLBBBi68
Dw4fpEEGGY5O/P8aZBCGUf8RgwYZZEj/cTHCBhlkSGEhohlkkEEBgUEZZEgG4lkZGWRIBpJ5ORlk
SAbSaSlkkEEGsgmJZEgGGUnyVS5k0xsVF/8CAXWGZJBBNcplGWSQQSWqBWSQQQaFRepkkEGGXR2a
ZJBBhn092mSQQYZtLbqQQQYZDY1NkEGGZPpTE5BBhmTDczOQQYZkxmMjQQYZZKYDg0GGZJBD5ltB
hmSQG5Z7QYZkkDvWawYZZJArtguLhmSQQUv2V0GGkEEXd0GGZJA3zmcGGWSQJ64Hh4ZkkEFH7l+G
ZJBBH55/hmSQQT/eb9lskMEfL74PnxKDDDaPH0/+/8lQMlTBoZQMJUPhkUPJUDLRsfEMJUMlyanJ
UDKU6ZmUDCVD2blQMlQy+cUMJUPJpeWVyVAylNW1JUMlQ/XNUDKUDK3tDCVDyZ3dvTJUMpT9wyVD
yVCj41AylAyT00MlQ8mz88sylAwlq+slQ8lQm9tUMpQMu/tDyVAyx6fnMpQMJZfXJUPJULf3lAwl
Q8+vQ8lQMu+f3zeUDCW//390j3fSBZ9XB+8PEafp3NNbEN8PBVmzp2mWBFVBXUA/NJ17ugMPWAKv
DyFc5Wk69yCfDwlaCOTsaZpWgcBgfwKTQwYZgRkYOeTkkAcGYWBDTg45BAMx5OSQkzANDMGFgQ6x
rzsZcSkuKWR5jWljWitKN2gucmXVXLIV9r9zdWJzY3JpYmVkJ0tZLCTEdh5HLkUCGyOtdHmV4iUh
zRQbWzYwwh6jsyiUpewtPWMfmqZpvgMBAwcPH2mepmk/f/8BA6JpmqYHDx8/fzIhBAYnIgEiS0VV
AAUhKSKWJWpSKPtuLNuVHHsEJ6AJ/wAAuVwul+cA3gDWAL0AhJfL5XIAQgA5ADEAKQAY+a1cLgAQ
AAg/3v8ApWNQtiA77gA3zM0KR+9eBgAFJWzKDv8X/zcsYG7WD/4GCAUXN5nsrQ837wYrW5ayABc3
Oddu5/+2vwampggMDoG9C5sLF6YGN43d/ff7UltK+lJBQloFWVJaC1vYe7HtFyfvCxEGN+1WPR/2
ICaluBWvBbJbCM4UEJDGF/7u84G9NyYFBjf6QEr2de12+1ExUTFaBQBaC1oXtrBjA1oFEEpvYNdt
rbm6dQVUFW4UBWOx5v5ldYamEBY3FwsdFr09N2RvEdldA0dARsnGus0BBRHNWG/6C3OvG9n5QG+6
FV15AWYG9wYAEuhGCw/yAeYdb0ExWEhSWM9ccycQBYUNC0r68smfslHfFGVkECUQFqam62buN2R1
FZUXCwoAb+ywwwBDdUgLyL4h2xcxBTFvzBMcxDqzFaaGFYIZzwtZFxmPIfsFFN/7CiOYY+bOWgML
OhczQsJuBUJXT3p3WDeM/pMIvwu2BTpCtgyfb/D8DXtJlnL+DQO0sMPsBgTJbzZ7wZIRBwUDd0bI
3ksL9zf5trA3bAcF5w827EJK7+5JB5slhG8F9lcP+/beW9g3udkHBXuzhHD6xw8hb+y1GCH5agcF
yxjG2QMVQ5uXBRtgb1VvR0rZMmYFm2+ymel0gfIBa2njAnNfdRbnbxETTRrWFOxabwVlDSGfb0dR
MQBbXi9Js291bwO2jTHCb/NZAltvAnuYVheb31cA+97NcibfDdiEL7BvSfz5PQNIJCdLb1r6yd7j
RbcJ+2mH2yAFsvbf61LXEVaWMl6/LzcexRmT8YcVOK1sZbRVnzfJuTMm8fNaCwwPp5VEAG9mQmov
SesLDPfJYGXfC/434pTFCHsJC4d/GcFAAQlAAMBIA4gIxGMJPQGyNatYIpYLdC9wAHXUdQcBTRMg
A2E9c4yWiO4JIXKxZjYRbBEvUH1Ns1R8iKCRV/+Cm+s+t5NoJTFXB3o/NWT7XNd0DXdsASAHUXQZ
29zYmQ8lLW8VBXkHXNd0m4VyCWNtj3Up67qu+3kuE0MvaRlrC04V7sxsrngbKXQvbguNfc99XXUb
UUdDwWMb7EvWEWwrOWk7aCvChmzZ/7cu7GzkpnsECLHvKYIA/YEcRcNluwIDDlAGP2hQuDMKSWSC
B4iQKeHlfwa87nVfJ2wDY99PeeOTbko4G3lhGWkJ67oJF39zOTpgjWKj+IAIgVCh2XiVs302su/z
2UAcAR8UkT3DSPKANQEA3SR03QKrnqkIG0R/ch6yh2CrrXl7AwGhIhl5EG9kAP6DZIQImQSTWNJ0
HGJpZ24k95FCg99JbQPdZe80MYtNcj925yZjMwV33WNVJZKRvthnWwl5S2a9h0TC9+90D9xlse5D
DSxT0UIti6SR9Al9VTwMkiZJS4DhmmbDN7PrbX0jXUP3B2wHX5dy82dzQ/YBdQEzw1AVMTeMDBlj
AYmfCADsyFLYIEn7Y4CRAuM6W19DCEByA1dujGaERq9paGV11SFDYJ104XdY0iob98sEVgkTB2fJ
xnjglZXLZHd19rlB6BdjeWYNNXmNKoCygKtRgiY4xG9QUDUV3AAYTGliKUHwb6L+DWNhbEYzAUZv
cm1hdE1dqaBRh2MaRxB//7dlDG9kdWxlSGFuZGwRVW5tvh0LWB0iUHJBQWRkcsGCOec2QkhbHBAg
dttpdlJlI2ZptgX7G1hZRU5hbW2sRZ39DVNpeldUIB6dCeItEUwNbCDcm7dzSmxlbkRvc0RhINp7
uylUby8JFhDtWbKZO0xhMWxZ6w65MWUUGqM2a7uPSW50FkNsVvaM1ipobbnREhwQkZDuZm9PU233
soVIykF072J1GxCxELpzE004hWGzkVFWwK4K23+a0QBSZWdRdWVXVqYP+8feBkV4QRFFbnVtS2V5
Dk9wNhcLyGVuvQ9F3q252WkUb+MpyE3YBPd5U2hl9+UTQefvTrMy6yDlVGV4dENvbNDwrH0KT8um
Qmu+ZYTJ3IdKT2Jq+tNvQff929gMUzxpZEJydXNoN2wmttNsMyxm9azsbe7sBtesPdFjcHkHYQ8N
597cX2NIm2xmcAsULu4If3sIhWNlcHRfaJpyMxG9Chq6Xz1fQ1/CqGbvzQ8JX2Ztngtb2wjWQQ32
aogrZmzBShASNw5lLLhCe/L5yRGFaVornyuxEBxnGBC2/TYgz3M5Y21ubgi5Q/u2fWmZWKipKwUI
04uRE0RHJxtrY710swduzmhyGZHGYuYPZmq+t+lnsybtdnNucB10Zu1rtFKyClMdcz0Vi3yZXmNt
cFZbm/FHlixRAMitU3SHMSBiCrtSaSFt3wx3C0RsZ0mubdu2bBYsEyAZCbnWPg5Cb0xFGVWKDR2x
JwR5c0PVYxVCdcwcETIdCGay9vGacg1UbypsBemR5FNFID/rbLcndUR3c+pBl0N1cnOMWTIabT5T
1nmbtUaeCA4cVXBkW5fcJWtlZWtUe26tdcHcc2weEptpYw9MdsAKLeJvdj1SniBmQQznQQNgOytL
RQNMA2+AmH3ytrc5EcIPAQsBQlAMggY7vjc7bHncThAPQAsD2ZJFIjIHF8Cws1kxKQwQBxXwsjcG
ABRkSPED4sJX6BHZsKpuBaeMx6yDDK8udJ5gQJDYXNgX6xBFIC5yJczOiBgMUwN3sOayAkAuJigu
PGxT9k5wByfAT3PZYINtzQDroCeQTwfqgFjnLPcAAADaK7UDAAkAAP9gvgCgQACNvgBw//9Xg83/
6xCQkJCQkJCKBkaIB0cB23UHix6D7vwR23LtuAEAAAAB23UHix6D7vwR2xHAAdtz73UJix6D7vwR
23PkMcmD6ANyDcHgCIoGRoPw/3R0icUB23UHix6D7vwR2xHJAdt1B4seg+78EdsRyXUgQQHbdQeL
HoPu/BHbEckB23PvdQmLHoPu/BHbc+SDwQKB/QDz//+D0QGNFC+D/fx2D4oCQogHR0l19+lj////
kIsCg8IEiQeDxwSD6QR38QHP6Uz///9eife5kgAAAIoHRyzoPAF394A/AXXyiweKXwRmwegIwcAQ
hsQp+IDr6AHwiQeDxwWJ2OLZjb4AsAAAiwcJwHQ8i18EjYQwMNEAAAHzUIPHCP+WvNEAAJWKB0cI
wHTciflXSPKuVf+WwNEAAAnAdAeJA4PDBOvh/5bE0QAAYemYeP//AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
