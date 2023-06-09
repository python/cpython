"""
File generation for catalog signing non-binary contents.
"""

import os

__author__ = "Jeremy Paige <jeremyp@activestate.com>"
__version__ = "2.7"


__all__ = ["PYTHON_CAT_NAME", "PYTHON_CDF_NAME"]


def public(f):
    __all__.append(f.__name__)
    return f


PYTHON_CAT_NAME = "python.cat"
PYTHON_CDF_NAME = "python.cdf"


CATALOG_TEMPLATE = r"""[CatalogHeader]
Name={target_stem}.cat
ResultDir={target_parent}
PublicVersion=1
CatalogVersion=2
HashAlgorithms=SHA256
PageHashes=false
EncodingType=

[CatalogFiles]
"""


def can_sign(file):
    return os.path.isfile(file) and os.stat(file).st_size


@public
def write_catalog(target, files):
    with target.open("w") as cat:
        cat.write(CATALOG_TEMPLATE.format(target_stem=os.path.splitext(target)[0],
                                          target_parent=os.path.dirname(target)))
        cat.writelines("<HASH>{}={}\n".format(n, f) for n, f in files if can_sign(f))
