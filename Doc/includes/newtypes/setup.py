from setuptools import Extension, setup
setup(ext_modules=[
    Extension("custom", ["custom.c"]),
    Extension("custom2", ["custom2.c"]),
    Extension("custom3", ["custom3.c"]),
    Extension("custom4", ["custom4.c"]),
    Extension("sublist", ["sublist.c"]),
])
