from distutils.core import Extension, setup

setup(name="MacPython for Jaguar extensions", version="2.2",
	ext_modules=[
		Extension("OverrideFrom23._Res",
			["../Modules/res/_Resmodule.c"],
			include_dirs=["../Include"],
			extra_link_args=['-framework', 'Carbon']),
		])