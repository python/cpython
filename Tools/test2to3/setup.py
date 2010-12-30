# -*- coding: iso-8859-1 -*-
from distutils.core import setup

try:
    from distutils.command.build_py import build_py_2to3 as build_py
except ImportError:
    from distutils.command.build_py import build_py

try:
    from distutils.command.build_scripts import build_scripts_2to3 as build_scripts
except ImportError:
    from distutils.command.build_scripts import build_scripts

setup(
    name = "test2to3",
    version = "1.0",
    description = "2to3 distutils test package",
    author = "Martin v. Löwis",
    author_email = "python-dev@python.org",
    license = "PSF license",
    packages = ["test2to3"],
    scripts = ["maintest.py"],
    cmdclass = {'build_py': build_py,
                'build_scripts': build_scripts,
                }
)
