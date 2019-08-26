from c_parser.info import Variable

from . import info, util


# XXX need tests:
# * known_from_file()

def known_from_file(infile, fmt=None):
    """Yield StaticVar for each ignored var in the file."""
    raise NotImplementedError
    return {
        'variables': {},
        'types': {},
        #'constants': {},
        #'macros': {},
        }
