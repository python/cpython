from c_parser.info import Variable

from . import info, util


# XXX need tests:
# * from_file()

def from_file(infile, fmt=None):
    """Yield StaticVar for each ignored var in the file."""
    raise NotImplementedError
    return {
        'variables': {},
        'types': {},
        #'constants': {},
        #'macros': {},
        }
