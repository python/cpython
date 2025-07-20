# -*- encoding: utf-8 -*-

# This is a package that contains a number of modules that are used to
# test import from the source files that have different encodings.
# This file (the __init__ module of the package), is encoded in utf-8
# and contains a list of strings from various unicode planes that are
# encoded differently to compare them to the same strings encoded
# differently in submodules.  The following list, test_strings,
# contains a list of tuples. The first element of each tuple is the
# suffix that should be prepended with 'module_' to arrive at the
# encoded submodule name, the second item is the encoding and the last
# is the test string.  The same string is assigned to the variable
# named 'test' inside the submodule.  If the decoding of modules works
# correctly, from module_xyz import test should result in the same
# string as listed below in the 'xyz' entry.

# module, encoding, test string
test_strings = (
    ('iso_8859_1', 'iso-8859-1', "Les hommes ont oublié cette vérité, "
     "dit le renard. Mais tu ne dois pas l'oublier. Tu deviens "
     "responsable pour toujours de ce que tu as apprivoisé."),
    ('koi8_r', 'koi8-r', "Познание бесконечности требует бесконечного времени.")
)
