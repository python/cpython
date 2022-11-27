"""
File generation for catalog signing non-binary contents.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"


__all__ = ["PYTHON_CAT_NAME", "PYTHON_CDF_NAME"]


def public(f):
    __all__.append(f.__name__)
    return f


PYTHON_CAT_NAME = "python.cat"
PYTHON_CDF_NAME = "python.cdf"


CATALOG_TEMPLATE = r"""[CatalogHeader]
Name={target.stem}.cat
ResultDir={target.parent}
PublicVersion=1
CatalogVersion=2
HashAlgorithms=SHA256
PageHashes=false
EncodingType=

[CatalogFiles]
"""


def can_sign(file):
    return file.is_file() and file.stat().st_size


@public
def write_catalog(target, files):
    with target.open("w", encoding="utf-8") as cat:
        cat.write(CATALOG_TEMPLATE.format(target=target))
        cat.writelines("<HASH>{}={}\n".format(n, f) for n, f in files if can_sign(f))
