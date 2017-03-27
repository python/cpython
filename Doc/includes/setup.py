from distutils.core import setup, Extension
setup(name="noddy", version="1.0",
      ext_modules=[
         Extension("noddy", ["noddy.c"]),
         Extension("noddy2", ["noddy2.c"]),
         Extension("noddy3", ["noddy3.c"]),
         Extension("noddy4", ["noddy4.c"]),
         Extension("shoddy", ["shoddy.c"]),
         ])
