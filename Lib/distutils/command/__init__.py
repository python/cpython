"""distutils.command

Package containing implementation of all the standard Distutils
commands.  Currently this means:

  build
  build_py
  build_ext
  install
  install_py
  install_ext
  dist

but this list will undoubtedly grow with time."""

__rcsid__ = "$Id$"

__all__ = ['build',
           'build_py',
           'build_ext',
           'install',
           'install_py',
           'install_ext',
           'sdist',
          ]
