"""distutils.command

Package containing implementation of all the standard Distutils
commands."""

__revision__ = "$Id$"

__all__ = ['build',
           'build_py',
           'build_ext',
           'build_clib',
           'build_scripts',
           'clean',
           'install',
           'install_lib',
           'install_headers',
           'install_scripts',
           'install_data',
           'sdist',
           'bdist',
           'bdist_dumb',
           'bdist_rpm',
           'bdist_wininst',
           'bdist_sdux',
           'bdist_pkgtool',
           # Note:
           # bdist_packager is not included because it only provides
           # an abstract base class
          ]
