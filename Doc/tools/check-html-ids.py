from compression import gzip
import concurrent.futures
from pathlib import Path
import html.parser
import functools
import argparse
import json
import sys
import re


IGNORED_ID_RE = re.compile(
    r"""
    index-\d+
    | id\d+
    | [_a-z]+_\d+
""",
    re.VERBOSE,
)


class IDGatherer(html.parser.HTMLParser):
    def __init__(self, ids):
        super().__init__()
        self.__ids = ids

    def handle_starttag(self, tag, attrs):
        for name, value in attrs:
            if name == 'id':
                if not IGNORED_ID_RE.fullmatch(value):
                    self.__ids.add(value)


def get_ids_from_file(path):
    ids = set()
    gatherer = IDGatherer(ids)
    with path.open(encoding='utf-8') as file:
        while chunk := file.read(4096):
            gatherer.feed(chunk)
    return ids


def gather_ids(htmldir, *, verbose_print):
    if not htmldir.joinpath('objects.inv').exists():
        raise ValueError(f'{htmldir!r} is not a Sphinx HTML output directory')

    if sys._is_gil_enabled:
        pool = concurrent.futures.ProcessPoolExecutor()
    else:
        pool = concurrent.futures.ThreadPoolExecutor()
    tasks = {}
    for path in htmldir.glob('**/*.html'):
        relative_path = path.relative_to(htmldir)
        if '_static' in relative_path.parts:
            continue
        if 'whatsnew' in relative_path.parts:
            continue
        tasks[relative_path] = pool.submit(get_ids_from_file, path=path)

    ids_by_page = {}
    for relative_path, future in tasks.items():
        verbose_print(relative_path)
        ids = future.result()
        ids_by_page[str(relative_path)] = ids
        verbose_print(f'    - {len(ids)} ids found')

    common = set.intersection(*ids_by_page.values())
    verbose_print(f'Filtering out {len(common)} common ids')
    for key, page_ids in ids_by_page.items():
        ids_by_page[key] = sorted(page_ids - common)

    return ids_by_page


def do_check(baseline, checked, excluded, *, verbose_print):
    successful = True
    for name, baseline_ids in sorted(baseline.items()):
        try:
            checked_ids = checked[name]
        except KeyError:
            successful = False
            print(f'{name}: (page missing)')
            print()
        else:
            missing_ids = set(baseline_ids) - set(checked_ids)
            if missing_ids:
                missing_ids = {
                    a
                    for a in missing_ids
                    if not IGNORED_ID_RE.fullmatch(a)
                    and (name, a) not in excluded
                }
            if missing_ids:
                successful = False
                for missing_id in sorted(missing_ids):
                    print(f'{name}: {missing_id}')
                print()
    return successful


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        help='print out more information',
    )
    subparsers = parser.add_subparsers(dest='command', required=True)

    collect = subparsers.add_parser(
        'collect', help='collect IDs from a set of HTML files'
    )
    collect.add_argument(
        'htmldir', type=Path, help='directory with HTML documentation'
    )
    collect.add_argument(
        '-o',
        '--outfile',
        help='File to save the result in; default <htmldir>/html-ids.json.gz',
    )

    check = subparsers.add_parser('check', help='check two archives of IDs')
    check.add_argument(
        'baseline_file', type=Path, help='file with baseline IDs'
    )
    check.add_argument('checked_file', type=Path, help='file with checked IDs')
    check.add_argument(
        '-x',
        '--exclude-file',
        type=Path,
        help='file with IDs to exclude from the check',
    )

    args = parser.parse_args(argv[1:])

    if args.verbose:
        verbose_print = functools.partial(print, file=sys.stderr)
    else:

        def verbose_print(*args, **kwargs):
            """do nothing"""

    if args.command == 'collect':
        ids = gather_ids(args.htmldir, verbose_print=verbose_print)
        if args.outfile is None:
            args.outfile = args.htmldir / 'html-ids.json.gz'
        with gzip.open(args.outfile, 'wt', encoding='utf-8') as zfile:
            json.dump({'ids_by_page': ids}, zfile)

    if args.command == 'check':
        with gzip.open(args.baseline_file) as zfile:
            baseline = json.load(zfile)['ids_by_page']
        with gzip.open(args.checked_file) as zfile:
            checked = json.load(zfile)['ids_by_page']
        excluded = set()
        if args.exclude_file:
            with open(args.exclude_file, encoding='utf-8') as file:
                for line in file:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        name, sep, excluded_id = line.partition(':')
                        if sep:
                            excluded.add((name.strip(), excluded_id.strip()))
        if do_check(baseline, checked, excluded, verbose_print=verbose_print):
            verbose_print('All OK')
        else:
            sys.stdout.flush()
            print(
                'ERROR: Removed IDs found',
                'The above HTML IDs were removed from the documentation, '
                + 'resulting in broken links. Please add them back.',
                sep='\n',
                file=sys.stderr,
            )
            if args.exclude_file:
                print(f'Alternatively, add them to {args.exclude_file}.')


if __name__ == '__main__':
    main(sys.argv)
