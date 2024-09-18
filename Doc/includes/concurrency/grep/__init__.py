from collections import namedtuple
import os
import types

from . import _utils


# [start-high-level]
def grep(regex, opts, *filenames, impl=None):
    from ._implementations import resolve_search
    do_search = resolve_search(impl)

    # step 1:
    regex = _utils.resolve_regex(regex, opts)
    # step 2:
    filenames = _utils.resolve_filenames(filenames, opts)
    # step 3:
    with do_search(filenames, regex, opts) as matches:
        yield from matches
# [end-high-level]


def resolve_impl(impl, cf=None):
    from ._implementations import resolve_impl
    return resolve_impl(impl, cf)


# [start-search-lines]
def search_lines(lines, regex, opts, filename):
    try:
        if opts.filesonly == 'invert':
            for line in lines:
                m = regex.search(line)
                if m:
                    break
            else:
                yield Match(filename)
        elif opts.filesonly:
            for line in lines:
                m = regex.search(line)
                if m:
                    yield Match(filename)
                    break
        elif opts.invertmatch:
            for line in lines:
                m = regex.search(line)
                if m:
                    continue
                yield Match.from_inverted_match(line, filename)
        else:
            for line in lines:
                m = regex.search(line)
                if not m:
                    continue
                yield Match.from_re_match(m, line, filename)
    except UnicodeDecodeError:
        # It must be a binary file.
        return
# [end-search-lines]


class Match(namedtuple('Match', 'filename line match')):

    @classmethod
    def from_re_match(cls, m, line, filename):
        if line.endswith(os.linesep):
            line = line[:-len(os.linesep)]
        return cls(filename, line, m.group(0))

    @classmethod
    def from_inverted_match(cls, line, filename):
        if line.endswith(os.linesep):
            line = line[:-len(os.linesep)]
        return cls(filename, line, None)

    def __new__(cls, filename, line=None, match=None):
        return super().__new__(cls, filename or '???', line, match)


class Options(types.SimpleNamespace):

    CLI_BY_ATTR = {
        # file selection
        'recursive': [('-r', '--recursive', True)],
        # matching control
        'ignorecase': [('-i', '--ignore-case', True)],
        'invertmatch': [('-v', '--invert-match', True)],
        # output control
        'showfilename': [('-H', '--with-filename', True),
                          ('-h', '--no-filename', False)],
        'filesonly': [('-L', '--files-without-match', 'invert'),
                      ('-l', '--files-with-matches', 'match')],
        'showonlymatch': [('-o', '--only-matching', True)],
        'quiet': [('-q', '--quiet', '--silent', True)],
        'hideerrors': [('-s', '--no-messages', True)],
    }

    @classmethod
    def add_cli(cls, parser, dest='grepopts'):
        assert dest

        import argparse
        class GrepOptsAction(argparse.Action):
            DEST = dest
            def __init__(self,
                         option_strings,
                         dest,
                         default=cls(),
                         nargs=0,
                         **kwargs):
                kwargs['default'] = default
                assert nargs == 0, nargs
                kwargs['nargs'] = nargs
                super().__init__(option_strings, self.DEST, **kwargs)
                self.grepoptsattr = dest
            def __call__(self, parser, namespace, values, option_string=None):
                setattr(self.default, self.grepoptsattr, self.const)

        for attr, opts in cls.CLI_BY_ATTR.items():
            for opt in opts:
                parser.add_argument(*opt[:-1], dest=attr,
                                    action=GrepOptsAction, const=opt[-1])

    def __init__(self, *,
                 recursive=False,
                 ignorecase=False,
                 invertmatch=False,
                 showfilename=None,
                 filesonly=None,
                 showonlymatch=False,
                 quiet=False,
                 hideerrors=False,
                 ):
        self.__dict__.update(locals())
        del self.self

    def __str__(self):
        import shlex
        return shlex.join(self.as_argv())

    def as_argv(self, *, short=True):
        argv = []
        for attr, opts in self.CLI_BY_ATTR.items():
            value = getattr(self, attr)
            for opt in opts:
                if value == opt[-1]:
                    argv.append(opt[0] if short else opt[1])
                    break
        return argv
