from optparse import Option, OptionParser, _match_abbrev

# This case-insensitive option parser relies on having a
# case-insensitive dictionary type available.  Here's one
# for Python 2.2.  Note that a *real* case-insensitive
# dictionary type would also have to implement __new__(),
# update(), and setdefault() -- but that's not the point
# of this exercise.

class caseless_dict (dict):
    def __setitem__ (self, key, value):
        dict.__setitem__(self, key.lower(), value)

    def __getitem__ (self, key):
        return dict.__getitem__(self, key.lower())

    def get (self, key, default=None):
        return dict.get(self, key.lower())

    def has_key (self, key):
        return dict.has_key(self, key.lower())


class CaselessOptionParser (OptionParser):

    def _create_option_list (self):
        self.option_list = []
        self._short_opt = caseless_dict()
        self._long_opt = caseless_dict()
        self._long_opts = []
        self.defaults = {}

    def _match_long_opt (self, opt):
        return _match_abbrev(opt.lower(), self._long_opt.keys())


if __name__ == "__main__":
    from optik.errors import OptionConflictError

    # test 1: no options to start with
    parser = CaselessOptionParser()
    try:
        parser.add_option("-H", dest="blah")
    except OptionConflictError:
        print "ok: got OptionConflictError for -H"
    else:
        print "not ok: no conflict between -h and -H"

    parser.add_option("-f", "--file", dest="file")
    #print repr(parser.get_option("-f"))
    #print repr(parser.get_option("-F"))
    #print repr(parser.get_option("--file"))
    #print repr(parser.get_option("--fIlE"))
    (options, args) = parser.parse_args(["--FiLe", "foo"])
    assert options.file == "foo", options.file
    print "ok: case insensitive long options work"

    (options, args) = parser.parse_args(["-F", "bar"])
    assert options.file == "bar", options.file
    print "ok: case insensitive short options work"
