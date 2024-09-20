import argparse
import sys

from . import Options, grep
from ._implementations import IMPLEMENTATIONS, impl_from_name, resolve_impl


def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    parser = argparse.ArgumentParser(
        prog=prog,
        usage=('[--help] [--concurrency MODEL] [--cf] [-rivHhLloqs] '
               '[-e] REGEX FILE [FILE ...]'),
        add_help=False,
    )

    parser.add_argument('--help', action='help', default=argparse.SUPPRESS,
                        help='show this help message and exit')

    concurrencyopts = parser.add_argument_group(title='concurrency')
    concurrencyopts.add_argument('--concurrency', dest='impl',
                                 metavar=f'{{{",".join(sorted(IMPLEMENTATIONS))}}}')
    parser.set_defaults(impl='sequential')
    concurrencyopts.add_argument('--cf', '--concurrent-futures', dest='cf',
                                 action='store_const', const=True)

    grepopts = parser.add_argument_group(title='grep')
    Options.add_cli(grepopts, 'opts')

    regexopts = grepopts.add_mutually_exclusive_group(required=True)
    regexopts.add_argument('-e', '--regexp', dest='regex', metavar='REGEX')
    regexopts.add_argument('regex', nargs='?', metavar='REGEX')
    grepopts.add_argument('files', nargs='+', metavar='FILE')

    args = parser.parse_args()
    ns = vars(args)

    impl = ns.pop('impl')
    cf = ns.pop('cf')
    opts = ns.pop('opts')
    pat = ns.pop('regex')
    files = ns.pop('files')
    assert not ns, ns

    try:
        impl = impl_from_name(impl, cf=cf)
    except ValueError as exc:
        parser.error(str(exc))

    return impl, opts, pat, files


def _render_matches(matches, opts):
    matches = iter(matches)

    # Handle the first match.
    blank = False
    for filename, line, match in matches:
        if opts.quiet:
            blank = True
        elif opts.filesonly:
            yield filename
        elif opts.showonlymatch:
            if opts.invertmatch:
                blank = True
            elif opts.showfilename is False:
                yield match
            elif opts.showfilename:
                yield f'{filename}: {match}'
            else:
                try:
                    second = next(matches)
                except StopIteration:
                    yield match
                else:
                    yield f'{filename}: {match}'
                    filename, _, match = second
                    yield f'{filename}: {match}'
        else:
            if opts.showfilename is False:
                yield line
            elif opts.showfilename:
                yield f'{filename}: {line}'
            else:
                try:
                    second = next(matches)
                except StopIteration:
                    yield line
                else:
                    yield f'{filename}: {line}'
                    filename, line, _ = second
                    yield f'{filename}: {line}'
        break

    if blank:
        return

    # Handle the remaining matches.
    if opts.filesonly:
        for filename, _, _ in matches:
            yield filename
    elif opts.showonlymatch:
        if opts.showfilename is False:
            for filename, _, match in matches:
                yield match
        else:
            for filename, _, match in matches:
                yield f'{filename}: {match}'
    else:
        if opts.showfilename is False:
            for filename, line, _ in matches:
                yield line
        else:
            for filename, line, _ in matches:
                yield f'{filename}: {line}'


async def _arender_matches(matches, opts):
    matches = type(matches).__aiter__(matches)

    # Handle the first match.
    blank = False
    async for filename, line, match in matches:
        if opts.quiet:
            blank = True
        elif opts.filesonly:
            yield filename
        elif opts.showonlymatch:
            if opts.invertmatch:
                blank = True
            elif opts.showfilename is False:
                yield match
            elif opts.showfilename:
                yield f'{filename}: {match}'
            else:
                try:
                    second = await type(matches).__anext__(matches)
                except StopAsyncIteration:
                    yield match
                else:
                    yield f'{filename}: {match}'
                    filename, _, match = second
                    yield f'{filename}: {match}'
        else:
            if opts.showfilename is False:
                yield line
            elif opts.showfilename:
                yield f'{filename}: {line}'
            else:
                try:
                    second = await type(matches).__anext__(matches)
                except StopAsyncIteration:
                    yield line
                else:
                    yield f'{filename}: {line}'
                    filename, line, _ = second
                    yield f'{filename}: {line}'
        break

    if blank:
        return

    # Handle the remaining matches.
    if opts.filesonly:
        async for filename, _, _ in matches:
            yield filename
    elif opts.showonlymatch:
        if opts.showfilename is False:
            async for filename, _, match in matches:
                yield match
        else:
            async for filename, _, match in matches:
                yield f'{filename}: {match}'
    else:
        if opts.showfilename is False:
            async for filename, line, _ in matches:
                yield line
        else:
            async for filename, line, _ in matches:
                yield f'{filename}: {line}'


def render_matches(matches, opts):
    if hasattr(type(matches), '__aiter__'):
        return _arender_matches(matches, opts)
    else:
        return _render_matches(matches, opts)


class MatchIterator:
    def __init__(self):
        self._matched = None
    def iter(self, matches):
        for match in matches:
            self._matched = True
            yield match
            break
        else:
            if self._matched is None:
                self._matched = False
        for match in matches:
            yield match
    async def aiter(self, matches):
        async for match in matches:
            self._matched = True
            yield match
            break
        else:
            if self._matched is None:
                self._matched = False
        async for match in matches:
            yield match
    @property
    def matched(self):
        return self._matched


def main(opts, pat, filenames, impl='sequential', kind=None, cf=None):
    impl = resolve_impl(impl, kind, cf)

    # steps 1-3:
    matches = grep(pat, opts, *filenames, impl=impl)

    results = MatchIterator()
    if hasattr(type(matches), '__aiter__'):
        import asyncio
        # step 4:
        async def search():
            it = results.aiter(matches)
            async for line in _arender_matches(it, opts):
                print(line)
        asyncio.run(search())
    else:
        # step 4:
        it = results.iter(matches)
        for line in _render_matches(it, opts):
            print(line)

    # step 5:
    return 0 if results.matched else 1


if __name__ == '__main__':
    impl, opts, pat, files = parse_args(prog='grep')
    rc = main(opts, pat, files, impl)
    sys.exit(rc)
