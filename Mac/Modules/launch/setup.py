# This is a temporary setup script to allow distribution of
# MacPython 2.4 modules for MacPython 2.3.

from distutils.core import Extension, setup

setup(name="LaunchServices", version="0.2",
        ext_modules=[
                Extension('_Launch', ['_Launchmodule.c'],
                extra_link_args=['-framework', 'ApplicationServices'])
        ],
        py_modules=['LaunchServices.Launch', 'LaunchServices.LaunchServices'],
        package_dir={'LaunchServices':'../../../Lib/plat-mac/Carbon'}
        )
