# This is a temporary setup script to allow distribution of
# MacPython 2.4 modules for MacPython 2.3.

from distutils.core import Extension, setup

setup(name="QuickTime", version="0.2",
        ext_modules=[
                Extension('QuickTime._Qt', ['_Qtmodule.c'],
                extra_link_args=['-framework', 'Carbon', '-framework', 'QuickTime'])
        ],
        py_modules=['QuickTime.Qt', 'QuickTime.QuickTime'],
        package_dir={'QuickTime':'../../../Lib/plat-mac/Carbon'}
        )
