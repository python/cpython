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
                   ]

    boolean_options = ['keep-temp']

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_temp = 0
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
                raise DistutilsOptionError, \
                      "target version can only be" + short_version
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

        self.run_command('build')

        install = self.reinitialize_command('install')
        install.root = self.bdist_dir
        if os.name != 'nt':
            # Must force install to use the 'nt' scheme; we set the
            # prefix too just because it looks silly to put stuff
            # in (eg.) ".../usr/Scripts", "usr/Include", etc.
            install.prefix = "Python"
            install.select_scheme('nt')

        install_lib = self.reinitialize_command('install_lib')
        # we do not want to include pyc or pyo files
        install_lib.compile = 0
        install_lib.optimize = 0

        install_lib.ensure_finalized()

        self.announce("installing to %s" % self.bdist_dir)
        install.ensure_finalized()
        install.run()

        # And make an archive relative to the root of the
        # pseudo-installation tree.
        fullname = self.distribution.get_fullname()
        archive_basename = os.path.join(self.bdist_dir,
                                        "%s.win32" % fullname)

        # Our archive MUST be relative to sys.prefix, which is the
        # same as install_purelib in the 'nt' scheme.
        root_dir = os.path.normpath(install.install_purelib)

        # Sanity check: Make sure everything is included
        for key in ('purelib', 'platlib', 'headers', 'scripts', 'data'):
            attrname = 'install_' + key
            install_x = getattr(install, attrname)
            # (Use normpath so that we can string.find to look for
            # subdirectories)
            install_x = os.path.normpath(install_x)
            if string.find(install_x, root_dir) != 0:
                raise DistutilsInternalError \
                      ("'%s' not included in install_lib" % key)
        arcname = self.make_archive(archive_basename, "zip",
                                    root_dir=root_dir)
        self.create_exe(arcname, fullname)

        if not self.keep_temp:
            remove_tree(self.bdist_dir, self.verbose, self.dry_run)

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

        for name in dir(metadata):
            if (name != 'long_description'):
                data = getattr(metadata, name)
                if data:
                   info = info + ("\n    %s: %s" % \
                                  (string.capitalize(name), data))
                   lines.append("%s=%s" % (name, repr(data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append("\n[Setup]")
        lines.append("info=%s" % repr(info)[1:-1])
        lines.append("target_compile=%d" % (not self.no_target_compile))
        lines.append("target_optimize=%d" % (not self.no_target_optimize))
        if self.target_version:
            lines.append("target_version=%s" % self.target_version)

        title = self.distribution.get_fullname()
        lines.append("title=%s" % repr(title)[1:-1])
        import time
        import distutils
        build_info = "Build %s with distutils-%s" % \
                     (time.ctime(time.time()), distutils.__version__)
        lines.append("build_info=%s" % build_info)
        return string.join(lines, "\n")

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
        self.announce("creating %s" % installer_name)

        file = open(installer_name, "wb")
        file.write(self.get_exe_bytes())
        file.write(cfgdata)
        header = struct.pack("<ii",
                             0x12345679,       # tag
                             len(cfgdata))     # length
        file.write(header)
        file.write(open(arcname, "rb").read())

    # create_exe()

    def get_exe_bytes (self):
        import base64
        return base64.decodestring(EXEDATA)

# class bdist_wininst

if __name__ == '__main__':
    # recreate EXEDATA from wininst.exe by rewriting this file
    import re, base64
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
ZGUuDQ0KJAAAAAAAAABwj7aMNO7Y3zTu2N807tjfT/LU3zXu2N+38tbfNu7Y39zx3N827tjfVvHL
3zzu2N807tnfae7Y3zTu2N857tjf3PHS3znu2N+M6N7fNe7Y31JpY2g07tjfAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAUEUAAEwBAwAu6+E5AAAAAAAAAADgAA8BCwEGAABAAAAAEAAAAJAAAPDUAAAA
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
bGwgUmlnaHRzIFJlc2VydmVkLiAkCgBVUFghDAkCCl/uS+s25ddS0bYAAO40AAAAsAAAJgEAkP+/
/f9TVVaLdCQUhfZXdHaLfAi9EHBAAIA+AHRoalxWbP/29v8V/GANi/BZHll0V4AmAFcRmP833//Y
g/v/dR5qD5SFwHUROUQkHHQLV9na//9VagX/VCQog8QM9sMQdR1otwAAJJD/T9itg10cACHGBlxG
dZNqAVhfXu/u/v9dW8NVi+yD7AxTVleLPXguM/a7OsA5dQjt7d39dQfHRQgBDFZogE0RVlZTBfsz
9v8M/9eD+P+JRfwPhYhkfPgDdRshb67dfCD/dRDoHv8AaJ8PhAO2Z992QeuxH1B0CVCQ6y9cIC3b
H9sY6lMMagL/VSDowC7GXlibZxBmdSUuu/luDU9oVCfpUwHkOwePfLvvWQ7sJHQKEwONRfT3Wdhu
bnUcAhg6dH38Et5Mw7ADQKQ0FHUJ3TfD6wuIlncOVmoEVhCgwUVoMBCIdIl2rW2+rw9hOII86yal
K9u2NdsCUyqcU6cIJYsEO81gnu/GdRcnECh0jv/JhQ0KM8BsW8k4g30QCFOLYfsW2l0IaUOS8jiT
yNW2ue12UOjIPpJFDC9QyAhu22X5FEBqAcwYXgbYJWioYdv951Eq8VCJXdQtFTzXHDvzfX9hdGn/
dChQaJCYGUsE9y3d7RZcjnQTGg18iwTJs+7nOor2IR8U7DouH6O1NpBkQwPFRs1922ESPshTl4wZ
jeHDHrpe8FAUxs6B7Bjhhr+ju4tNENAMRFQL+o1EC+q4u+3//ytIDCvKg+kWA9GBOFBLBQaJTfTu
ZSyDZf/bx/8MAGaDeAoAD45OHQY99ItVEItEGiqN22y3/zQajTwIA/uBPjEBAi42gT8LNX/HcgME
KosPv04gg8IuiW73bbswA9ME8BFWHgPKBRwDEW1fmrfRCE8cicFXGgPQE/QjNH65jRoe7I2F6P6U
YqRs+bpnC2iw81DbnhAf297NFt5wjYQFDTb4Rszum3tHGlAbJQOudfAeDGG/DdjJSwRhV5hGFIC8
BedY9x13D1x0Rkxmi0YMUAQOQV2hZDjwdk4J6S0AGs49lIa6Hie0Ps322+4bdhRRDexLAfMdGDnT
HUsbKhS8GBNAUItyOrexe79AClBCagZVFLQS/zi53dcaFTkGjLR51t9duHGacOv3USQERBGKCITJ
csb/WwGA+S91A8YAXEB174dAMExytzR0F4C2VTdaR2rdYV8FUhEmwFcKjtOxUBRMYQeM4mz5v9kM
SGoKmVn3+TPJaLRwUQAyDe/bHmi8AgANRTBNbaNZPzhQMtcaIRSOsN1sFSi+gIkEVi9ZULq70ZZC
DwE5HSQY/9NoHdjt5DbkKCBgIwqme72vARXTqRhfLNaaOXOfnkT08fbvvm33whAA2rgABgA9rOFO
dHCBd8R6bAUQPLCjfQhXPvwndEFh/QYExwQkIJvVAPC4z3Y6FcDyYDVYBVu7NHP4GiXoAz861mRz
sN1o4MJo/QygnAAE2r3pbV+EXusnuYF4CDjNdRnfe2bbI2odcKl0XFSGhlkzKa2IEGotQmyQ8DB3
YzJdl9qF1jiL+AUc/IsO9DRuC28RK9Ar8lIP+Cv5Aq3jb+NSmSvC0fgY8BWARmSE2ccNEYAF3rqD
jgEIaoPoTmqEwAGwW3hQa244Hi3CwVvYwAyCSBXtFzdbq9tGvv7RZjMR28HoENt9R4FIOQoIeYst
NDBo8OjQQMOYI2jmgCbVSLZn+D8IgiAbFgHQuDP/M+1VVWiLFdM7xYmCLdwy0qxIVUmGFOEF4dta
LOoDJDweHgTr3v0SiCUJIyss0KCrQw8ohxE+G09YaO+NFbC3SAYHR87pCgCawGKmIC7Ma/0G+5aV
JBhomW29PlBVZMs2iDVJVVopz0IpyIqUsBzog4y1hVlFdDIciY1s0FBysfhXUDFxLDHGrRwVGE0Q
LAIxd2fC/QMcaCwbpe+mm2wfYCkE6xBo2hgNt2XBi+tFpyBbYYR/cThBM/9XVydomWHDwcU62y91
BCsSxth26wLsV+tWeignGyb3fVuBsN2+Yc1/+FMz26gZAAxTMdKSa2aS6vyJCGg3tGNndAcwPQna
UwA6NfObR1NQjUWYxznfOEsx15CxJ2L1W66J18DKQDwGpBD6u2Yv/DvDL804GHQUjU2Yme3E1ybV
4vs2nNKzudRTUIRI/4Bx1no6aWsvEGQSAUPXWtI820noKfj+VJvNzlYGYuzHIKp9my1ZxjXs8P1O
U/a+19K18AB3CAwfG8Pumm0MvFk36GiadD2wLmD3GPzyhBsMvFjBoFISRoEaWNYkGoa/U+knuZOD
mZ55Xi3xAoC84JqmPWBqZ/ACBADtijC76wN0yvNo7Aa3M4cQWFAO9WOOeso63AtvHc0BNevZaCtr
csDSEHLM7ru+ZFVNbbeLQAg9Mc/Ws8XjdCk9Y5/EA8PpArMQNU5WvjLUfQj8iT0AnbaAuH8RXDXO
TizVDWYUnsA08D6DHKAAu4F8/PfCczTCUyyHaWjPHAaLg8F3NeCZBXArAsKRwG53Uxk5NWwQowhm
dBK2vwivhTedIgt1WR5whWS36Fde82jEGik+BoZmG4AeUYM9VNVEqeFDnCAifw7vhLK4cmA1vQ2w
/WCSj0DE+IVHBVxzn9pv61OzNVQTeTTZHL0H+tQHIAnAA12nGeiwrRi33TVtW0wFiYPT5xCFDAZZ
EXq1StN0SkJvuXRTfV9Zw4BMAaGUKuFac2DmOy1ZE/8XGroGjARB6/YPt8HB4BBM2QiHTWFWu+fx
U7G6ZD+6/b02VlUQQRR3ayvnqVFAdE02/43WJeMviGgEcwgPJAp1TQb3lCRwfFniBCYQSmjLZuBK
PBx6v3J/i3YE66eLKz6BxL0sLbwzCACUKVtnEPWd+NoA/KMMGXMQgVnajQkIakhIBquxLMu2XXwC
XEIIcHsZWttLDbnrO2Q285tRwISpQd+BS3bqDtZSVyYQhQNndXbrb21Qi1AscfbsZg3nWGowGGg4
91zcWD+duUAgO2ouGCQMSNZ7rH4qaDQlkxDsz5W6H0q+67RfbGF62cMOe9iLw+CWCFiGhgn8MAzX
NUZoH+GJBshZiUYEA4Feb+HGEhtvVjkBvgqhufrCdDUhTQhQUXIFIhDOFmUR80g3zSKy6Qiv+GDi
o2SxX6EGiB3tqk2fo92kK/CvElbkFMe2uiQQalFA+jX8NxczE5/5obyhULzCDk5YiHRJbRXGhe0C
wHRDBAGNaiMmt8Fcx3TjbDg31hgGdL//b40WkwYHD5XBSYPhAkGLwaNC68dde7nexwUHyffsXcP/
ZI73aiRoUDzHQOgGXvfYG8B6OjijQHkcIXQzmi0aOK+//eTWzkznOoMfCsR4CXx6L05mIuvb00G9
tkEe1id1tz0RdELgvQg6aBi5LuslzBQyIY0EXXTX5QkLdGoLMI19xIVucLuO86sG9Ikiq6s22zb2
gUzdDKsakBOMG7/mBqYWUHl2wDBHBSi1iS/ruJf29tg2HNzhFIwb2FoVB0dbM7cizGvkH/C3sTvT
uSsSbCBnFkAZcvLkkvRt4Rn4gHaeXG47H58zdW9XaH+MNFx8mGMFdstmYpQLBayMf5AgaNuy/Zt1
tAK8qA+k/qbrlfluGCmABJvpFVwPGQhwHGD4OBAzEYb3EsNPBxbKo4wYFWiYZpMVhiRsF+or1PUM
Ti4EnQipEev+6mvA2+JcMr0BalC7uYdbL91pvpCziAT42g8tDNd0EfRZSz9omYlInT9r/R9OvWWx
DSNGf+tqNukgdBWKZA89ODPbo4QTozpWD1LSGFQ0YS0h3aPCR0RdfOx9zyCbPKMw9aITu1CjMIOy
e9j8D5KnlYbeSBmhN2CFFjg7CjRcgGAmBLBhmGFhof7w3v4/C5dVT4D5XHVEikgBQAgwfOj/Blje
BDN+Fm6vcnXZxgYNRuu15Vq20wUK9DFzUfs1eNIY9DwKwx+IBlKNKPCbH62IDkZAmS4hHnxqfSRR
U01UYIzOPQNW+giA+HiIjSbG2IPmKyP8UHhBaQ1k0aVEACFcywC0IF3wLxv1BMU0aC0X9PyX5S82
AxYRBFUI9by1/VvQTALqVytBEAIMBB6BOd9scRcJjTQQWKSBPnlWNOs2ZLYSC5hJjJAHuz3nOPAe
/it+Zl9GkudCZgMzgb78/tGJyUaXbOQb6cKsuBam9HTQaBsDQ7E4N0rpsNBO0F4m23bOC7oCTXjb
gRZX2nELFgzXaOsetcTLKNbe6wfQdJoQwTj358cZDkMe3ShnK9NHLn0dVqDYA9x/FEB1R64dS+Tg
tgB/JrgznC02DexhZIoePpLpYEU2NPrYuzB1ms3U5nEi+HslhGZKbmEp4RfBJBebQm4g4ATH0d5z
HMjm2O/4dFa9jChBw2tI2qsRLQpnzTUFBAstVjxsDnMrX2hhFQpscCeczhFww8wAtv//Ljoz0jvC
VnQzi0gcO8p0LIlQFAIIW2z/XxiLcQz33hv2UoPmB6wxoxz6ERtuIBRRCBq8517CMexr2F+4jf8I
kABccBdd7QiNOotG9bVuK99UTiSFyT0UDQqUSmxu2T8o3AgeGigYgLml26ckDccAAFQbO2gun+k+
O8cY941NsTWKAQ0dOsFNW6tWmOf1ZwrcqcW+M3kMO/d1Cj/ghHhr7ZtkIIl+GDoTYCBgOmduC29/
fig5fmUHDiSAgZspXGlqGC+EuieJL5Ska4Y+/NYQiXg1+MJCYVYXz4l6DIXd27/ftPfZx0AMAXj5
CHxZBA9/VB+4wtZufRHTs0oQUtdRN/ej7f/aG9JQ99KB4jA5ZVK1GzwZvsVrCu9BTx96FHUPj2/Z
KZpuDpx3hCebdQtWG8lfuPppeZ6wZBBxU1UQRTDQVXME8HaFzba7CvkDoT4ACPCLVCPfP/VLO4P6
BL/76pXDS70FweOJbw/t+4lcGYkIyA0Ph8RPZis8/CSNgCoZBLY9iGuxtY1JHokNEAgLL41v41sF
iw6KERwENRYQfaN1iwQjD0LALhZ0FceZF5wrOFXdbBiYdRu3725A66Iii1AQwekowQhddnYwObAY
JIQ2Fp6ieb7WFwW9BBFI2tuu0IaOZghAdoteHAtC7XHbeQaJvR8DE397/2/0i0MEOQgDwff1hdJ0
IccDVpTJ08Xc0d1fbGh2Nrfx9sEgJYFjKQcmUcARGxzYPxcwLPDaG7T4pDD9dScKwb4YowJV80u2
ta5mLCYCkiIBT9SW2uxpAnOgM43oNuciLeVSHhJEVAzJN9s6+QvYDDnjCHvBnHktAmPk7eHPNd6a
StzB4RhIC+QotGRrSTQJt2Gs3XA7g0hCiQY6HBTY37pCkIFIN+IQA8qJSDlcJJcMCr4IW3LJkAuE
NnO4MTc/OUg0EjazGSIs6+UzWUJADgTpVKTVD9kGaAJ1CYvHcM4t+2jCCKdncmozmYV2Y6QWUEdu
hPAAu8cBAzkWSE8GGQ63N4oKnVMzB8iRsz5WAgQOIYTJJNIgkBJG2IkosyE124WEH3hOMPMGuBqW
kez4O2ksRGazgmFwACXk2wrLagD9DEMBYm7Jlin9BjjNcrv9C7cmTC4nAyQpXp3bZts02CQqF6US
KANHmWXTNIG7XCpo8EASeFt/01cocLg1vwV6PIlDdKO9tY3cBA8EBXUOvuvrwAZbQlKZV8p1kQ3s
egZ1DT5XUeoyfLBtN94ox/IBRjQCMA444rO1IO5RCCB0DnZrHbkFj9AfYEcwwNRnatjDf6NtaqgG
nSshZGMgzugdtOhx9qbDw4tPKAFnyNEXSS8aX6WbqsDZLYtXKIzcYyRmkMnDckCgOWwDtlAoKB8D
507nnytRHi6iQjRIWDYCeDFbeOsD2B6JXiy8OMhIFokhBEeqWnSbRzKD7DA4U284PK2xNdD7KUOy
axJIvpWgbS5L/04QMFY7yOXftYV+VAoVRHMFK8FI6wUuX8C1LAcejAOD+AkZDAn+B3+FnDhA2BiD
/QNzPIzhejKQ2JYNxu//277kSIoPxxRMlIvRi83T4oPFCGN777ruC/JHMYk4iS9yzusEN69vbYF/
g+AHi8jR6LUBZB5LGHeeBW66kWPEg+0DGQHNwab33xwHwe4D0+4r6T+zHC5baHfVQUgmIFKNsISN
DTV8YncwUQ44Us45jCRct1uo1yE0+BpRDyxSEKD7QIneECqMFImx58xXrrXvXFhxkAOLmQZhFAPx
1h3e+P1YFM4gcyxAw7l1qfr6oAY/TCD3XLYsT/Z8QCeu1MTvQnLUi9aLzoLhB3Jv/y3w6hAz0a+i
OO2LwTvF+gSJbLCDdGtcSyYBi4kD6ejctsRM0he8KsccfNcOmwWFnRZ8GkQ71nUja/wWN7+Leygt
dBmL1zuxFdsuFL5zByvCSFdkK/JziTX4QtcddWe0TEFIBFOJUzQvtlY6GDkHRzBqNrzNutajTDox
K8pJ/0ss+W7P0QcEPlV1IGL3yc0HGdbyTovOwovIhgl3tqResAsF7g0WGsl2ncI7wQVii5qGwT4U
RFPR4xe6X4EC86WLyi0c3wMr0POk2m17u8NcJUQDUg1LXTrTtRsV8CsMFol4HGBzLRgpAWhdZBgY
Q3CY2UEqllwlB/IOczgyDkf3IWOS0iX/PyXIIPqWLTaYH4cdBtbQurk7FjzgCIH6oAUT8gXwDbYu
qQV9H0aNhAgCZ+nmHMB3A0go+VDeeD4bYQyNBQ5IDsdDvU0TJ27wBOsIro1uNI1xU5IIEQqDYvmd
pwstc2hZMr40BtLCZCoDLAhOn1qiI7GL/JhVSwxoDbYfxQSRYQgIA2H3MFeGamdymDC4E6G9Ntn7
yHMhPDTHMWk73VzhNaA3IHLfcBoaLH1pJG9DEI1TUVI0zqbNGVfx41BRMpy2mV0p7PCFIfsI5sNX
WMgFT2XQNN7OguHiHzc1Al0Pg3vHo28n0lk76HMz40rea2vvOwXr+vlKmPY1N4Tm9PkH+i4Hf5eO
+c2LyfCIuRQjxuarFV17VMEBjeY0GMZVWtvtbhCXNHMbySvq0QxFDtew4IQSinFApDf4Qr/bIfAj
ErnNdAMz8oPoEsld4vHNWSsk+AsfC+Rhf8ALO+lzO5ngBNHIgXUfMJ3pyfS7c+3sfHdViwyNqSPO
r9XaayYOFGLUkBnh1Hgb1xUc4Xfrp3OMCh4D0Dsqh6l1JZZyr9MqORDpxdyFGpnwgpMVDXup8M3a
HYr86wIAqAxBSJkgoT18j/x19XeJXnrd9jJxgoWYFUAkJlHNmOkDUECN3wksJF/DtY5RElI8Njs/
UZGNAu5CBQE8a88UHeZZA2UJB0AGDzek6XGW/CQfFUw+HDjqJETKJTRPA67Nz3c9nzwgjiGwfSsc
eVCkTttClueEVwQEBilILSzwAA9zXms8MK0utmiX2ATQK53m4MXXOANWTOjOTae+Bq3u51HMSZp5
tkaxe0B0VnhxdiVdtlQAHSQKD7gnTT4NIzgUnBkYsSnMr5VAmiEYidK5JBuYACwAoeu1jYOdz4sm
aJqW2jX32m7plUxRd4XaFx1ySaCwkKEzxqENuwYww+BRXGFmjTfT/cszGBR2P1X74bfnUfLk12r9
K9HDA+pQTp7sSgZLTI0xi2lsrmHZOVHQKwFmkuoEst0iLxVSUTpDTn1rs4UyasdBGPQ9/Vhry0tG
QEhIUYl5BHCEIcxGRBgRSyBco0046LOs8oRvEGYRp4QVUsiL90hgxlTKxBOfz4AAzjlBBJMM7luh
iocr9wPug1FMCQT3T9FYuGHB0BJFE5/PnkIIhA9q/FCUeXcYyYaQ0HWMz5ACCYUrjhjKZm8knf11
BlulsEX2ME9RqDqIkKxj1yJolBTKGGHLfJ67A5d1rZFS3VAGJaQZhDXPtBUyCe7a/oH9zoVgiF8k
TCsDtpAQ7BjlPQZZIj4JKXkjhTtcSFBS1+s9W6YHDECmZnJCd4fnQVBWU3RLtLn3yFPRdDehe+gg
qvztIzcuiVYEf1Ar1YtuCPKtZFvjbn0+ZggrMsYBGDFDf0GrhW/HTFZVxWNSSJOtQ0tWmUiHkPQ7
nZigDCFMCJcNGE0IMgmRU0/2GmtfsP5FQ0gqQ2a78RT/RCwUPS0DsFwul8vbLoovcTDAMmc3LLtl
s84SOKgnxiwoJcUM2KkzG+9QCDbADKIMagwrRQEOGEdYaeUCPIXdi1hGKADn9xoYDRgIV2OBHeBj
6U+3uBGDVLvv3XUKFxv0DOzCDI5c+d87vnvbD4bvEVWB+7AVmcNyBbgIKyv+uIvYgg+Moa3owe3b
ohC/RWEQihaDxhvIIWfvrFbxA/kI8vMhhxxy9PX2hxxyyPf4+foccsgh+/z92M4hh/7/A01MRAk2
vGSf0FbqbesVFhJGE0h19LENdt/G7rnx8vfxTL8IizX397B1t9rri/WHEzFdF1uT4PACL18LwQif
oWyCH5UIUG5ARlAGcgULGHTV6HiXPgTDDx8coYUauyU3hSKKT6NG4B13RYhQEFoMiEgRdQBhwB30
AA9IGMPfWnjFwxR/IHbOAzQWTBhGkvBWyGwu2hLabgzBDDSaJlwtwX7FvBDCAn2wfUYsB4kzTTrj
V9yA3/4GbFhCgwRa6JAcGp3OMHLWdhAKCpJsKEatXC1/eiyJfjuMKdtqaYErInut+YWJBpWIpVJl
3FU2gA07upRWUiJNEU9VEJndU8d3OuzqyKN+HK4zXSu4SJ0oDUCuAxkrEXqjMAH4v0ZypXQTSffZ
G8kBuV9sdYPB701hKw1mLLVU7WORek22RVi9NVayRVj4c0RA8VyseFwEug61L8Ardu0wALKOz9Pg
n3NfctAAxwgLyDZ54Cw7G921QT8KLHK8roX4amyp7iMgCFbISRgjPPo1NBTT6LhuwW/BpV9FK/hA
igHFFotJZpotEo+VCAavJbobPagQdLvgD66LrzZJF2sFIh8CQK8HLRXdRcOoduMn6dyQMx8HgtpC
9t5nDhqvSNx50JF8wwLn2AjeN3PyvosETLlNBAPIzpmutdatkbDUcgPmSkWb19MF9UUcEgyGzGVe
lgPCEkaRRGS0gDRMDEQEVkG4WTBSZQyNDMGICJAHCEHYAkAOOWQMDAU4FKDAb34DdwOODWsV1XUD
wis3njM1qUDWH+2z8QrRI5axCZYq8eMpVpfUTiwtUNpsn451IT4wO8ERHpxUalQtKQz7OHVE2Ajr
D39nhpNoFI0US3KGzMhIYjwMbbCTG0hiXWNhJbJDbiJej2InYYS4ntsBkELzCYgX4dzfSv8RQUg7
UAg6zdHx3AdODGZJYc9sSIOBKDewAOPGRwUK4E0KhIG9T4gKQkhEvfYMPAFXzxSLKwoUOEa34sdD
HyvNEyRC1gwXEar0VzfTnRTDSgkwGNjYBF4AAWJQZYW5QX5q/SvNU1ZQSVmobHPA67SYij+UlQeJ
Az6D/wd2FXsl+Hs/PIPvCJFMicISOsNMN1C2i7mgVYey6mLK9MaOs04gOittbgq9wV88+VMr/Ytr
ZO+JC1tIeDTC/hJBE9kimQE7/nYLXySQJDt04QO8PMtls1xAPf50PpI/Z1cjbJZB351A4gT59Mgi
yAwgUVPTsRQ8bCAvE3YQqptBomfY23UJ+8dHx6FbWXUcslZVi2wJCG6gbo26U+sgUlXRL0rXSwET
hTNMibbaMqLT/jcaW8WJXH9TUsdHGCS8VxT89i40XV5MHvt0BoN9zEVNt80MHwC+wjCesFhYKc+B
7PBtXeX+oowk9Ab8tN8B0y1QA9VXz0QDSE3TNE1MUFRYXGA0TdM0ZGhscHSQIXjTeHyJrCQ97RdK
bDIB735chESNRAO6C3TpQ0qJuu05CHUfcRhE/Fe+gZRuwIkpiSrF2MKLF48anBe5YQM99RGNmDtD
OSg9DboBfEGDwAQmdvN2343Hp/nNcwaaYroPK7Rxo/9jeDkudQhKg+4EO9UFO/r4t9l2pSx2JVT6
vlGJO9Pm2L39369zEo1cjEQrM3glU8ME0RFy8jdDhDZvlaOFHAxE9QLdCo0DK/G6QHkQX3eyGRGi
A87liCwL5v7WBvZKhzPbA0wcSEnlNDrd74wcF3Xv3QRvNTLQV7TN/xxdwXa4FYyEHD0oP4wKj7dj
DYlceEKJERJ77ohvGhwIQzvZcsVXi9/3mo2M3UKMFDWUiSGeaShwXQNxJB7pnKH4YcdDABLEGh/x
2x08D4+BAjM0ZeChSBSHDbkKzS3wXjtJhdLsKz4g/XKwvbc7TQ+OB2AUONYLltwsLC34bLr0TPT/
OAPfK9NFA8871/AmAR11SxrXHCBJy2im/ye4jX0BO8d2J4PP//caFnAbli3HbhhBBK596+4WFr7F
beAfByvHEnLuy3aseYQkJL8754uxfAMZGznq+IH/iNjvJtj76cMgKyzCL42UhNg21I0DPYk4i7k/
dDgvIuz6Q4hMoLSELNa9RBPby4gFMb3G14tK/PCtFn7vi/XTwUMr8IkUvae7sTt0n+sJShgo4Ahd
seHwBo//WoxuT7e94YrQCRwq04g9MYsIDG1s4I2Rf3IHxg7A6583f/Rd0SkMk/FzFIH+yRvSg+rK
7sLioPZgiHHrICAUE+G+Cx7mAooUMQyCbot3a4DCSzQxIbEE9pGFL1oOhyRHuhY2sbXivLQ7FXMe
t8Xjt+iCgzB3iTmNPNWkQrczHHEEhh1y5tUUei+4MjGNwjGBhcJ0Wotw+wgz0NHoB3X4WEoOaCM0
HChgjByNBXeF9sExJE8j+ss6XxiD6Av2o9wET4gmK985M8Y4cawII3XcdRXI1FCHJ0ogK9LC08fH
wxxSkEDrwZqY3sarHk6RG0JZuqtv1zv1dBeRLAF0TftcwFoLAQwKQmBYBCQPX4/lgVajYThoEjsD
SANkGAtfGjicwGY0VWQYeKvHSTRS09homGLYQW8BoBgEFVVSYMb2EnCF9tfTRWdvIVg+OPvGDEwo
O9HOZEg4exZMsDe6c5BjBFYeqFJRS/ze+j11JCeDOhYIgf1qdxO3BMAAPx2rkGY5geRPUdDAIR8W
Hvt1H7R88MiG4yP8dAKQg5HFhi8jSzCBnQFsQkxJMEtgRSMP0V5cIN8N+PzeLgDvBs6h/AqciQIS
sDXlEJTH6nd/dMDDrV2HQMhR7QwANmDjY2vXe/04tdXAdv3Bd3YDqOuNthUsEXvvO+hY6Bq2jo0e
DzIg9wjqIGor8UdWFCvFA9XmMFYhfgkWljhwDotLPFUFuAm4UTZDPBLNi/f6iEn1pKZZyhz/yqWm
A8UXSywD/aLntqrtCnV+QUQoDZFhd7pFdR9zNOqaK+6flpMrbBCEV0dXYx3kQFZHMHzNay22UF74
hHuC5OpFwd6MimFKdcFwWihUiVFy4gVL+DUYXh+43Tj0zFn5i2mcUSDsRi1cO3EwNzgdO+5RckD/
cEEcOXMJK/VOwdyrlurOSTHNgTYlTTehtA4cLJ1oLvEgg/g8IotJQaLEVkcRi6XIGi32dl9hCAvW
Rx1y4liiV27E3+AwI8rIihzOjTTOLISOXAHeOcIyTgHT6gRnLEBrqj85BL4ju32AH2sMnWBeBDYD
yzj7B5ADVXTHg+MPK8M07SvQBTFODavLI0wykWykDw8gkSlb0jScMWbKRsgFAZTPXNgD8DvDcytZ
GIP5qv0c0OfVh9dBJlvOlviXcgc8WU76z9kcrVFwwe7H9SAUcEVI15QOvsGFvEkoETv3cheLNPhg
//dFig5GiE3/BoPrAusB4NuxRusncSwfO992E3b2t1aLHRwARUZPdfYYKBAt2c4MS57rGb8GFOj5
uQQZcEVJgWFXP2JHEnI6DnIz+dTWdUU76zycEEkE2xy/KhN0K/M+rPCyuYgv1K078w+CBwLNTd4t
PkeLdNnFZQV67O3B6x7ZcwLeOCv5MzE2xuqNFM2awsQc+t5BXIIWU0YI6s+JPiuskisUZ1YNVukA
e+HUc2IgdFZX2GxWpM9a2712gFzYcj8QlWoy8mb+9YhBt7adaAMrQVhAizE6eC+xQTl3X4lBZ5r9
DeCmd2af/yUAdeb5fG4FrGAIYbRgsLADA/TMzFE9jC0Icjj2u6GHSU6+LRCFARdz7JtK3LqYxAyL
4WDPUMNS4Ua2zEMEIAUsfZcd/Gr/aAhkU4BQZKGhUCnYekt0JQcYaMuARtF4iWXovvaNDdQNDRXE
fYMN21RsZ3M/BhAUyGRrKtWcDaTxCA3MCvd3ZGSh0AwAoxQobiNy7+ttOR1AGHlubGz372xO1BhY
aAxwgghwJ0RN0PtSoWA/K5Q0280tIFwMCZxQA5CgX1O0VE/c6QQyAH0XgCFOoeBuMPu7vwT9gD4i
dTpGCIoGOsN0BDwN2x6Qb/ISBCB28tTQTmiLm2akjPZF0DMR+uftjeTU6w4rIHbY6/VqClgEbRUt
lYJMRVeCnlEQh8kz5BHoDV+S7FQJiU2Iy2F7wcVMWQou/3WIH+yhwhsZG/AF6Ni0VTbM194DBCwv
gqzDRraEFZIAL8C4AETSHd0AAAeqyhYV//83SNM8EBESCANN0zRNBwkGCgULNU3TNAQMAw0C/yBN
0z8OAQ8gaW5mbGF0+/b/9mUgMS4BMyBDb3B5cmlnaHQPOTk1LQTvzf6/OCBNYXJrIEFkbGVyIEtX
Y7333ntve4N/e3dN033va1+nE7MXGx80TdM0IyszO0PTNE3TU2Nzg6MXITxNw+MBJQEkQzJkAwID
MyRDMgQFALNky05wX0cv031vCX/38xk/IU7TNE0xQWGBwTRNs+tAgQMBAgMEBtM0TdMIDBAYIGyF
NU0wQGDn11jCkY3HBqe3hAlJq6+zA4IMMsgLDA3RtqKj9iqnPgMfVFVIgUNyZfV/uwElRGkGY3Rv
cnkgKCVzKSzY/38QTWFwVmlld09mRmlsZRUrLGXv3hAdcGluZxcQ/+1nwHpFbmQgGXR1cm5zICVk
sIQFc1MXFBNwwMB+SW5pdDIYtvbblw5LXFRpbWUUUm9tYW4LgG377WhpCldpemFyXHdxbN3+5t7n
c3RhB3ggb24geW9AIGNr/3+7KXB1U3IuIENsaWNrIE5leHQg3Re31tq2bnQudYDoGUtjZXVbt71s
FRxpHWgVU31wtfYBg1suH3kWMh3Z2q2MAS5kYQ9QIFYb8Jy7WXNpBxbB5293sNntZnR3NGVcIAZD
bxHKbmc3lVxJoFDlaABDXTO027UoZrMpmP5nIZEtbNx0hClTb9fw0Jzf+n9mc3PEcoS12y6rby4A
G2MIb9ksiRwUIQ3h0F1igW4MVuGCa6+0pYuoTUlmX6N7hcJ2OiyudiG3te2DTGNoEmczBHkqLVxY
hINAc1p0CO1Y23ZzLCpvQmEEnS1hAGGJd4Ntjba9919PcDttEWBMZ4Vtu9APUi1fUxBwwFMr51ob
c1QjRghsIwu8b7OPS2kNAExvYWTwt7kRJssGADy0dBIbsIZHXykJOwsuB8Nh2zHKcif0J4tANedc
ewBFcnIzC32NHXO0hYcPT3bJd4bVvRX2EJrxWz8AG/+90P5zPwoKUHIGnFlFU/dBTFdBWY49hG0J
by4sCnAtTk/2p7L/LE5FVkVSK0NBTkNFTFxwoWA4U0uLA6tkdYjDA9uPeS6Xb3CeBPdAaJ9JrmZh
S38Kb9va6hZkFWELYjNut9cNBw1yZxZfdsMMO7xkD0ynD2q67SZIMWJ1BmRf6W+kr8VG729DfhoA
dHtZckQvBGxub3STudZorWWBQVOyIHLX3iaU1G9ncn3IdmFspNZqj3OUDmV/YYt1WTtjifZl4Zi1
r+thzmaAfvlHFJMW5sUvcazQvLEAd51kb3dhPCs2NlnhLlifVx1DHNBo7SEHZXd/u3hgc+4rZNPq
ckAglr3NdCkNCmsnF7BgrtARRGUZxW3jMNh0czNWa5CGwyHWd267wYZ7NNNlG7NkL2Iezn1wXSvq
Fbhw6Udvb9wh2KkneBgZ0lqyv51TWHltYm9scz8Wb2bauT5GbG92L88F7rFlXxh0eXD4oCgRTfS0
92marmsrqAeYA4x4aNTh3qxQG+OxNmLqPiTDMhHzZmbxZVoF900mY3MRMzHsYLi5PNhtbzctIcOy
zTX30G0vuGVLOLIbbgvkaAErtH5ZwwPFsNlmzgkv3h1IUywjEwVgLAHpmubsUAAHEFRzH1KDDXKy
HwBwMEDAgwzSdB9QCmAgsCDQIKC4H8EGGWSAQOA/jxlksAYfWBiQgwzSdH9TO3g4gzTNINBREWgM
MsggKLAIMsggg4hI8M1ggwwEVAcUVcgggzXjfyt0IIMMMjTIDYMMMshkJKgEDDLIIIRE6JDBJpuf
XB8ckEGaZphUU3ywQRhkPNifF/9BBhlkbCy4BhlkkAyMTPgZZJBBA1ISZJBBBqMjcpBBBhkyxAtB
BhlkYiKkBhlkkAKCQuQZZJBBB1oaZJBBBpRDepBBBhk61BNBBhlkaiq0BhlkkAqKSvQZZJBBBVYW
GWSQpsAAM3ZkkEEGNswPkEEGGWYmrEEGGWQGhkYGGWSQ7AleHhlkkEGcY35skEEGPtwbH7BBBhlu
LrwPQQYZbA4fjk5kEIak/P9R/xEZZEgag/9xMRlkSAbCYSFkkEEGogGBZEgGGUHiWWRIBhkZknlk
SAYZOdJpkEEGGSmyCUgGGWSJSfJk0xtkVRUX/wIBZJBBLnU1ymSQQYZlJaqQQQYZBYVFkEGGZOpd
HZBBhmSafT2QQYZk2m0tQQYZZLoNjUGGZJBN+lNBhmSQE8NzQYZkkDPGYwYZZJAjpgODhmSQQUPm
W4ZkkEEblnuGZJBBO9ZrGWSQQSu2C2SQQQaLS/aGkEGGVxd3hmSQQTfOZxlkkEEnrgdkkEEGh0fu
ZJBBhl8fnmSQQYZ/P95skMGGbx8vvg+DDDbZn48fT/5QMlQS/8EMJUPJoeGRyVAylNGxJUMlQ/HJ
UDKUDKnpDCVDyZnZuTJUMpT5xSVDyVCl5VAylAyV1UMlQ8m19c0ylAwlre0lQ8lQnd1UMpQMvf1D
yVAyw6PjMpQMJZPTJUPJULPzlAwlQ8urQ8lQMuub2zKUDCW7+8lQMlTHp5QMJUPnl0PJUDLXt/cM
JUMlz6/JUDKU75+UDCVD37+Pd9I3/38Fn1cH7+nc03QPEVsQ3w8Fp2mWp1kEVUFdnXu6s0A/Aw9Y
Aq8PIWk69zRcIJ8PCVrsaZrlCFaBwGB/QwYZ5AKBGeTkkJMYBwZhTg45OWAEAzHkkJNDMA0MgQ6x
5MGvO3EpLoUlZHnQaWNKN2gZVi5yZdUV9r8rXHN1YnNjcmliZWQnLCTEskt2HkUCG1lHI+IlIS6p
dHnNFDYwwpUXHp+zpewtWyg9Y6ZpvpQfAwEDB56maZoPHz9//wFpmqZpAwcPHz8hECeif52fqioE
Ii0EEQgNTTqACFkoHHuWJftuLAQnoAkul9uV/wAA5wDeANblcrlcAL0AhABCADlcLpfLADEAKQAY
ABAACCA7+a0/3v8ApWPuAApHULY3717KDszNBgAF/xdu1iVs/zcP/gYI7K0sYAUXDzeWsjeZ7wYA
F27nK1s3/7a/BqamC5s51wgMDgsX/feBvaYGN/tSW0r6UkFCWrHtjd0FWVJaC1sXJ+8LPR/YexEG
N/YgJqUIzu1WuBWvBRQQvTeyW5DGF/7uJgUG7XbzgTf6QEr7UTFRMVoFYwP2dQBaC1oXWgUQrbm2
sEpvYLp1Beb+121UFW4UBWV1hqYQFjcXN2RjsQsdFm8R2brNvT1dA0dARgEFEc1YG9nJxm/6C/lA
b7r3BnOvFV15AQAS6AHmZgZGCx1vcycP8kExWEhSWBAFhZ+yz1wNC0r6Ud8UZWQQ7jfyySUQFqam
ZHUVlRfDAOtmCwoAb0Mh2+ywdUgLFzEcxMi+BTFvOoIZzBOzFabPCyH7hhVZFwUU3+bOGY/7CiNa
AwvCbphjOhcFQldPN4wzQnr+kwi2DHdYvwu2BZ9vSZY6QvD8cv7D7A17DQMGBMnBkrSwbxEH3ks2
ewUDdwv3N2xGyDf5BwVCSraw5w/vhG827O5JBwX2V1vYmyUP+ze5hHD23tkHBfrHGCF7sw8hb/nG
2ey1agcFAxVDG2DLGJtvVTJmlwVvRwWb6XRK2W+B8gFzX7KZa2l1Fudv1hTjAhET7FpvIZ9NGgVv
R1ExSbNlDQBbb3Uxwl4vbwNvmFa2jfNZAltvF/veAnub381yJt8vsFcADW9J/CdL2IT5PQNvWvrj
RUgktwn7BbLJ3mmH9t8yXtsg61LXEb8vGZNWljfxh2W0HsUVOFWfMyatbDfx80QAyblaCwwPL0mn
lW9m6wtl30JqDPcL/jcIe8lg4gkLwUCUxYcBaCNqGQmRSBERiM8JPQGyNVaxRCwLdC9wAOuo6w4B
TRMgA2E9cwkYLRHdIXKxZjYj2CJeUH1Ns5Gp+BBBFP+CkzbXfW5oJTFXB3o/NWT3ua7pDXdsASAH
UXQZt7mxMw8lLW8VBXkHua7pNoVyCWNtj3Up13Vd93kuE0MvaRlrC04V3JnZXHgbKXQvbgsb+577
XXUbUUdDwWMRN9iXrGwrOWk7aCuEDdmy/7cu7NnITfcECLHvKXgA/YEciobLdgIDDlAGP2hYc2cU
SWSCB30AvQrTXQJDo2hCpoTTH4KfBbrXfSEnbANj/095bko4HAM7mWEZ67oJk2k3f3M5OmBio/gJ
gAiBUMP5bZONhI217xPvninsEMwAQhNJZ6w7zLpECXKdv506TUYehIcDAaGDZAD+gxEkJUIHmo6D
jKtigWduPlIIS3v3SeydhuRtG0mLTWRsprtyP3YFd/XSF/vcY1UlZ1sJeWOQSFgyZu8s1r3353QP
Qw0sU9E0kp67Qi0JlRXSKJltYdNsmIdLgE+z62voPlxtfQ1sB1+XcvM+oG6kZ3MBM9tQI6tgyBUx
k8hwI09ziexTg0QZR7ZjOl8OIQDJA1dGujGaEa9paGV11XSVDIF1+XekYYCs2ylngvBKAqvhjSB0
YzzjZHd1F2N5YVX73GYNNXmNuEAqFVXxoAmGCMRXUAU3QJQ4GExpYpuof00lQQ1jYWxGMwFGKmgU
/G9ybWF0TYPf/21XXxpHZQxvZHVsZUhhbmRsEVXHAhbEbm0dIlByYM65b0FBZGRyNkJIW4jddrAc
aXZSZSNmaf4GFgS2WUVOYW1RZ3/BbQ1TaXpXVIJ4C2sgHhH35m1nTA1sc0psZW5Eb3NEYfbeLggp
VG8vCRZ7liyImTtMYTHWugNEuTFlFBra7iNbo0ludBZDbFYKWpvN9oy50SSku7UOHGZvT1O9bCFE
SMpBdCyEbtvvYnUbcxNNONhsJERRVsD2n2ZhrtEAUmVnUXVlV/6xt8JWpgZFeEERRW51bUtlecUC
8sMOT3Blbr1udprND0XeFG/jNsF9aynIeVNoZfflE7vTbBNBMusg5VRlPGvf+Xh0Q29sCk/LpkJr
MvchNL5lSk9iavrT/zY2YW9BDFM8aWRCcnVzaDTbzH03bCYsZvWsu8G17extrD3RY3B5B7k3tzth
D19jSJtsZnALwt9ewxQuCIVjZXB0X2iagoauO3IzEV89X0Nf2Xtzr74PCV9mbZ4LNoI1qkEN9mqw
EsTWiCtmEjeu0B5bDmXy+ckRyucKC4VpsRAcZ78NiNYYEM9zOWNt0L6tbW5uCH1pmViowvRi7qkr
kRNExtpYAUO9dLMHbqSx2MnOaHLmD2Zq+tlsRr4m7XZzbnCtlOxtHXRm7QpTHSJf5hpzPV5jbXBm
/FHFVpYsUQDIDIjY1q1TdAq3h+3fYVJpDkRsZ0mtbYO9JQVrFwznFNrHwZYNCUJvTEUZI/Yk11WK
BHlzN9WZI6KhYxVCMh0+XrOOCGZyDVRvHsMG1ipggUXOdluQICd1RHdz6kElo/Gzl0N1cnNtPllr
xJhT1p4IDsldkrccVXBka2VlaxfMvXVUe25zbB4Smwes0FppYw8t4m92YhbEZD1SCCxP4AnjQSpQ
RUwQs28AAy7r4TkRikHwDcIPAQsBBjuHLU8IvuxOEA9AskjkZgsDGgc2KyZbF8ARDGXwBnYQBwYD
Amme5RRkjKASFVbVBaeMx1+wneEudJ4HkECQ6xAjYnNhRSAuchjLljA7DFMDAjvdwZpALiYYLTxw
B6+xTdknwE9zzQxiZd976/MnkE9oH6gD5yzfK7UDAAAAAAAAJAD/AABgvgCgQACNvgBw//9Xg83/
6xCQkJCQkJCKBkaIB0cB23UHix6D7vwR23LtuAEAAAAB23UHix6D7vwR2xHAAdtz73UJix6D7vwR
23PkMcmD6ANyDcHgCIoGRoPw/3R0icUB23UHix6D7vwR2xHJAdt1B4seg+78EdsRyXUgQQHbdQeL
HoPu/BHbEckB23PvdQmLHoPu/BHbc+SDwQKB/QDz//+D0QGNFC+D/fx2D4oCQogHR0l19+lj////
kIsCg8IEiQeDxwSD6QR38QHP6Uz///9eife5kQAAAIoHRyzoPAF394A/AXXyiweKXwRmwegIwcAQ
hsQp+IDr6AHwiQeDxwWJ2OLZjb4AsAAAiwcJwHQ8i18EjYQwMNEAAAHzUIPHCP+WvNEAAJWKB0cI
wHTciflXSPKuVf+WwNEAAAnAdAeJA4PDBOvh/5bE0QAAYemoeP//AAAAAAAAAAAAAAAAAAAAAAAA
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
AAAAAAAAAAAAAAAAAAABAAkEAACoAAAAOKsAAKABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAJ
BAAA0AAAANisAABiAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEACQQAAPgAAABArgAAWgIAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAABAAkEAAAgAQAAoLAAAFwBAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
