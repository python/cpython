#!/usr/bin/env python3

"""Check if specified HTML files have dead and redirected (to-be dead) links.

Call this script on HTML files of the rendered documentation.

A full run through the whole rendered documentation takes about an hour.

Copyright © 2022 by Oleg Iarygin <oleg@arhadthedev.net>
Licensed to PSF under a Contributor Agreement.
"""

import sys
from argparse import ArgumentParser
from codecs import decode
from concurrent.futures import ThreadPoolExecutor
from enum import Enum, auto
from functools import cache
from html.parser import HTMLParser
from http import HTTPStatus
from io import StringIO, TextIOBase
from itertools import count
from pathlib import Path
from time import sleep
from urllib.error import HTTPError
from urllib.parse import urldefrag, urljoin, urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener
from urllib.response import addinfourl


class LinkAnalyzer(HTMLParser):
    """Scanner for hyperlink referers and targets.

    A referer is an <a> HTML element. A target is an id or name attribute of
    any HTML element.
    """
    def __init__(self):
        super().__init__()
        self.targets = set()
        self.referers = set()

    @staticmethod
    def _get_attribute(attribute_name: str, container: list[tuple[str, str]]):
        return (value for name, value in container if name == attribute_name)

    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            self.referers.update(self._get_attribute('href', attrs))
            self.targets.update(self._get_attribute('name', attrs))
        self.targets.update(self._get_attribute('id', attrs))


class _NoRedirectHandler(HTTPRedirectHandler):

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise HTTPError(req.full_url, code, msg, headers, fp)


UA = ('Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, ' +
      'like Gecko) Chrome/102.0.5005.63 Safari/537.36')


@cache
def get_link_targets_and_referers(url: str, allow_redirects: bool):
    """Load a page and extract output hyperlinks with potential input points.

    To speed scanning up the result is cached (memoized) so other pages linking
    to the URL are processed instantly.

    Returns:
      - None if a target cannot be accessed .
      - False if a redirection is met and allow_redirects is False.
      - a tuple of targets (hyperlink lannding points) and referers (links).
    """
    try:
        # Sites love to return HTTP 403 Forbidden to unknown user agents
        # (crawlers, rare browsers, etc.) so we have no choise but to pretend
        # to be Chrome.
        request = Request(url, headers={'User-Agent': UA})

        opener = build_opener(
            HTTPRedirectHandler if allow_redirects else _NoRedirectHandler,
        )
        page = opener.open(request)

    except HTTPError as e:
        if e.code == HTTPStatus.TOO_MANY_REQUESTS:
            sleep(2)
            return get_link_targets_and_referers(url, allow_redirects)
        elif 300 <= e.code <= 399:
            return False
        return None

    except Exception:
        # URLib, HTTP client, whatever can throw any network-related exception.
        return None

    with page:
        analyzer = LinkAnalyzer()
        try:
            page_source = decode(page.read())
            analyzer.feed(page_source)

        except Exception:
            # Whatever the document is, it cannot have link anchors
            analyzer.targets = []

        return analyzer.targets, analyzer.referers


def get_absolute_url(base_path: Path, url: str):
    return urljoin(base_path.resolve().as_uri(), url.strip())


def analyze_page(base_path: Path, url: str, allow_redirects: str):
    url_to_check = get_absolute_url(base_path, url)
    # Improve cache reuse by dropping URL parts not affecting content
    direct_url_to_check, _ = urldefrag(url_to_check)
    return get_link_targets_and_referers(direct_url_to_check, allow_redirects)


class LinkStatus(Enum):
    SKIPPED = auto()
    GOOD = auto()
    BAD_REFERRER = auto()
    BAD_TARGET = auto()
    REDIRECTED_TARGET = auto()


def skip_unsupported(target: str, target_parts) -> LinkStatus | None:
    # For now, skip absolute paths considering them always available.
    # Otherwise we would need to write a manual URL resolver for file://.
    if target_parts.path[:1] == '/' and not target_parts.netloc:
        print(f'   skipped {target} (absolute links are unsupported yet)')
        return LinkStatus.SKIPPED, target

    # Skip mailto links; we cannot verify them anyway
    if target_parts.scheme == 'mailto':
        return LinkStatus.SKIPPED, target

    return None


def determine_target_status(target_page: str, target_parts) -> LinkStatus:
    if target_page is False:
        return LinkStatus.REDIRECTED_TARGET

    elif target_page is None:
        return LinkStatus.BAD_TARGET

    elif target_parts.fragment:
        targets, _ = target_page
        is_correct_link = target_parts.fragment in targets
        return LinkStatus.GOOD if is_correct_link else LinkStatus.BAD_TARGET

    return LinkStatus.GOOD


def test_file(filename, allow_redirects, limit, print_intermediate_report):
    links = analyze_page(filename, '', allow_redirects)
    if not links:
        print('   not found')
        return LinkStatus.BAD_REFERRER

    targets, referers = links
    if limit is not None and len(referers) > limit:
        print(f'skipped; {len(referers)} links is above the --limit threshold')
        return []

    def test_target(map_args):
        iteration_id, target = map_args
        print(f'[{iteration_id}/{len(referers)}] link to {target}...')
        target_parts = urlsplit(target)

        early_abruption_status = skip_unsupported(target, target_parts)
        if early_abruption_status:
            return early_abruption_status

        target_page = analyze_page(filename, target, allow_redirects)
        status = determine_target_status(target_page, target_parts)
        print_intermediate_report(status, target)
        return status, target

    with ThreadPoolExecutor() as pool:
        return pool.map(test_target, zip(count(1), referers))


def check_if_error(status: tuple[LinkStatus, str]):
    return status[0] not in {LinkStatus.SKIPPED, LinkStatus.GOOD}


###################################################
#
# CUI (Console User Interface)
#
###################################################

def print_title(text: str, printer: TextIOBase = sys.stdout) -> None:
    border = '=' * len(text)
    print('', border, text, border, '', sep='\n', file=printer)


def match_report_message(issue: LinkStatus, target: str, printer: TextIOBase):
    match issue:
        case LinkStatus.BAD_REFERRER:
            print('  file not found', file=printer)
            return True

        case LinkStatus.BAD_TARGET:
            fragment = urlsplit(target).fragment
            entity = f'#{fragment}' if fragment else 'the file'
            print(
                f'  broken {target} link; check if {entity} exists',
                file=printer,
            )
            return True

        case LinkStatus.REDIRECTED_TARGET:
            print(
                f'  redirected {target} link; increased loading time',
                file=printer,
            )
            return True

        case _:
            return False


def get_end_report(log: list[tuple[Path, tuple[LinkStatus, str]]]) -> str:
    # print() is a wrapper around a syscalls, and syscalls are slow.
    # So we batch all reports first so a caller will do a single call.
    # Reallocation should not be a problem because expected amount of printer
    # errors is miniscule.
    non_empty = False
    with StringIO() as printer:
        print_title('Final report on problems', printer=printer)
        for file, broken_links in log:
            print(file, file=printer)
            for issue, target in broken_links:
                non_empty |= match_report_message(issue, target, printer)

        return printer.getvalue() if non_empty else None


intermediate_reports = {
    LinkStatus.SKIPPED: 'skipped',
    LinkStatus.BAD_REFERRER: 'failed',
    LinkStatus.BAD_TARGET: 'failed',
    LinkStatus.REDIRECTED_TARGET: 'redirected',
}


def print_intermediate_report(status: LinkStatus, target: str) -> None:
    status_text = intermediate_reports.get(status)
    if status_text:
        print(f'   {status_text} {target}')


def main(options) -> None:
    print('collecting filenames to check...', flush=True)
    all_errors = []
    try:
        input_files = list(Path('.').glob(options.path))
        file_count = len(input_files)
        for file_id, file_path in zip(count(1), input_files):
            print_title(f'[{file_id}/{file_count}] {file_path}')

            target_results = test_file(
                file_path,
                options.allow_redirects,
                options.limit,
                print_intermediate_report,
            )
            errors = [
                status for status in target_results if check_if_error(status)
            ]
            if errors:
                all_errors.append((file_path, errors))

    except KeyboardInterrupt:
        print('\naborted')

    readable_report = get_end_report(all_errors)
    if readable_report:
        sys.exit(readable_report)


HELP_PROLOG = 'Check if specified HTML files have dead or redirected links'


HELP_EPILOG = """
Call this script on HTML files of the rendered documentation.

Eventhough the script does is multithreaded and caches the findings for already
processed pages, a full run through the whole rendered documentation takes
about an hour.
"""


if __name__ == '__main__':
    parser = ArgumentParser(description=HELP_PROLOG, epilog=HELP_EPILOG)
    parser.add_argument('path', help='A glob pattern of file paths to scan')
    parser.add_argument(
        '-r',
        '--allow-redirects',
        action='store_true',
        help='Do not report HTTP 3xx links as kind-of-broken',
    )
    parser.add_argument(
        '-l',
        '--limit',
        type=int,
        help='Skip files that contain more hyperlinks than specified',
    )
    main(parser.parse_args())
