""" robotparser.py

    Copyright (C) 2000  Bastian Kleineidam

    You can choose between two licenses when using this package:
    1) GNU GPLv2
    2) PSF license for Python 2.2

    The robots.txt Exclusion Protocol is implemented as specified in
    http://www.robotstxt.org/norobots-rfc.txt
"""

import collections
import re
import urllib.error
import urllib.parse
import urllib.request

__all__ = ["RobotFileParser"]

RequestRate = collections.namedtuple("RequestRate", "requests seconds")


class RobotFileParser:
    """ This class provides a set of methods to read, parse and answer
    questions about a single robots.txt file.

    """

    def __init__(self, url=''):
        self.entries = []
        self.groups = {}
        self.sitemaps = []
        self.default_entry = None
        self.disallow_all = False
        self.allow_all = False
        self.set_url(url)
        self.last_checked = 0

    def mtime(self):
        """Returns the time the robots.txt file was last fetched.

        This is useful for long-running web spiders that need to
        check for new robots.txt files periodically.

        """
        return self.last_checked

    def modified(self):
        """Sets the time the robots.txt file was last fetched to the
        current time.

        """
        import time
        self.last_checked = time.time()

    def set_url(self, url):
        """Sets the URL referring to a robots.txt file."""
        self.url = url
        self.host, self.path = urllib.parse.urlsplit(url)[1:3]

    def read(self):
        """Reads the robots.txt URL and feeds it to the parser."""
        try:
            f = urllib.request.urlopen(self.url)
        except urllib.error.HTTPError as err:
            if err.code in (401, 403):
                self.disallow_all = True
            elif err.code >= 400 and err.code < 500:
                self.allow_all = True
            err.close()
        else:
            raw = f.read()
            self.parse(raw.decode("utf-8", "surrogateescape").splitlines())

    def _add_entry(self, entry):
        self.entries.append(entry)
        for agent in entry.useragents:
            if agent not in self.groups:
                self.groups[agent] = entry
            else:
                self.groups[agent] = merge_entries(self.groups[agent], entry)
            sort_rulelines(self.groups[agent].rulelines)

    def parse(self, lines):
        """Parse the input lines from a robots.txt file.

        We allow that a user-agent: line is not preceded by
        one or more blank lines.
        """
        entries = []
        # states:
        #   0: start state
        #   1: saw user-agent line
        #   2: saw an allow or disallow line
        state = 0
        entry = Entry()

        self.modified()
        for line in lines:
            # remove optional comment and strip line
            i = line.find('#')
            if i >= 0:
                line = line[:i]
            line = line.strip()
            if not line:
                continue
            line = line.split(':', 1)
            if len(line) == 2:
                line[0] = line[0].strip().lower()
                line[1] = line[1].strip()
                if line[0] == "user-agent":
                    if state == 2:
                        self._add_entry(entry)
                        entry = Entry()
                    product_token = line[1]
                    entry.useragents.append(product_token)
                    state = 1
                elif line[0] == "disallow":
                    if state != 0:
                        state = 2
                        try:
                            entry.rulelines.append(RuleLine(line[1], False))
                        except ValueError:
                            pass
                elif line[0] == "allow":
                    if state != 0:
                        state = 2
                        try:
                            entry.rulelines.append(RuleLine(line[1], True))
                        except ValueError:
                            pass
                elif line[0] == "crawl-delay":
                    if state != 0:
                        # before trying to convert to int we need to make
                        # sure that robots.txt has valid syntax otherwise
                        # it will crash
                        if line[1].strip().isdigit():
                            entry.delay = int(line[1])
                        state = 2
                elif line[0] == "request-rate":
                    if state != 0:
                        numbers = line[1].split('/')
                        # check if all values are sane
                        if (len(numbers) == 2 and numbers[0].strip().isdigit()
                            and numbers[1].strip().isdigit()):
                            entry.req_rate = RequestRate(int(numbers[0]), int(numbers[1]))
                        state = 2
                elif line[0] == "sitemap":
                    # According to http://www.sitemaps.org/protocol.html
                    # "This directive is independent of the user-agent line,
                    #  so it doesn't matter where you place it in your file."
                    # Therefore we do not change the state of the parser.
                    self.sitemaps.append(line[1])
        if state != 0:
            self._add_entry(entry)

    def _find_entry(self, useragent):
        for entry in self.groups.values():
            if entry.applies_to(useragent):
                return entry
        return self.groups.get('*')

    def can_fetch(self, useragent, url):
        """using the parsed robots.txt decide if useragent can fetch url"""
        if self.disallow_all:
            return False
        if self.allow_all:
            return True
        # Until the robots.txt file has been read or found not
        # to exist, we must assume that no url is allowable.
        # This prevents false positives when a user erroneously
        # calls can_fetch() before calling read().
        if not self.last_checked:
            return False
        # TODO: The private API is used in order to preserve an empty query.
        # This is temporary until the public API starts supporting this feature.
        parsed_url = urllib.parse._urlsplit(url, '')
        url = urllib.parse._urlunsplit(None, None, *parsed_url[2:])
        url = normalize_uri(url)
        if not url:
            url = "/"
        entry = self._find_entry(useragent)
        if entry is None:
            return True
        return entry.allowance(url)

    def crawl_delay(self, useragent):
        if not self.mtime():
            return None
        entry = self._find_entry(useragent)
        if entry is None:
            return None
        return entry.delay

    def request_rate(self, useragent):
        if not self.mtime():
            return None
        entry = self._find_entry(useragent)
        if entry is None:
            return None
        return entry.req_rate

    def site_maps(self):
        if not self.sitemaps:
            return None
        return self.sitemaps

    def __str__(self):
        entries = self.entries
        if self.default_entry is not None:
            entries = entries + [self.default_entry]
        return '\n\n'.join(filter(None, map(str, entries)))

class RuleLine:
    """A rule line is a single "Allow:" (allowance==True) or "Disallow:"
       (allowance==False) followed by a path."""
    def __init__(self, path, allowance):
        if path == '' and not allowance:
            # an empty value means allow all
            allowance = True
        path = re.sub(r'[*]{2,}', '*', path)
        path = re.sub(r'[$][$*]+', '$', path)
        path = normalize_pattern(path)
        self.fullmatch = path.endswith('$')
        path = path.rstrip('$')
        if '$' in path:
            raise ValueError('$ not at the end of path')
        self.matcher = None
        if '*' in path:
            pattern = re.compile(translite_pattern(path), re.DOTALL)
            if self.fullmatch:
                self.matcher = pattern.fullmatch
            else:
                self.matcher = pattern.match
        self.path = path
        self.allowance = allowance

    def applies_to(self, filename):
        if self.matcher is not None:
            m = self.matcher(filename)
            if m:
                return m.end() + 1
        else:
            if self.fullmatch:
                if filename == self.path:
                    return len(self.path) + 1
            else:
                if filename.startswith(self.path):
                    return len(self.path) + 1
        return 0

    def __str__(self):
        return (("Allow" if self.allowance else "Disallow") + ": " + self.path
                + ('$' if self.fullmatch else ''))


class Entry:
    """An entry has one or more user-agents and zero or more rulelines"""
    def __init__(self):
        self.useragents = []
        self.rulelines = []
        self.delay = None
        self.req_rate = None

    def __str__(self):
        if not self.useragents:
            return ''
        ret = []
        for agent in self.useragents:
            ret.append(f"User-agent: {agent}")
        if self.delay is not None:
            ret.append(f"Crawl-delay: {self.delay}")
        if self.req_rate is not None:
            rate = self.req_rate
            ret.append(f"Request-rate: {rate.requests}/{rate.seconds}")
        if self.rulelines:
            ret.extend(map(str, self.rulelines))
        else:
            ret.append("Allow:")
        return '\n'.join(ret)

    def applies_to(self, useragent):
        """check if this entry applies to the specified agent"""
        if useragent is None:
            return '*' in self.useragents
        # split the name token and make it lower case
        useragent = useragent.split("/")[0].lower()
        for agent in self.useragents:
            if agent != '*':
                agent = agent.lower()
                if agent in useragent:
                    return True
        return False

    def allowance(self, filename):
        """Preconditions:
        - rules without wildcards are sorted from longest to shortest,
          "Allow" before "Disallow"
        - our agent applies to this entry
        - filename is URL encoded"""
        best_match = -1
        allowance = True
        for line in self.rulelines:
            m = line.applies_to(filename)
            if m:
                if m > best_match:
                    best_match = m
                    allowance = line.allowance
                elif m == best_match and not allowance:
                    allowance = line.allowance
            # Optimization.
            if line.matcher is None and (m or len(line.path) + 1 < best_match):
                break
        return allowance


def normalize(path):
    unquoted = urllib.parse.unquote(path, errors='surrogateescape')
    return urllib.parse.quote(unquoted, errors='surrogateescape')

def normalize_uri(path):
    path, sep, query = path.partition('?')
    path = normalize(path)
    if sep:
        query = re.sub(r'[^=&]+', lambda m: normalize(m[0]), query)
        path += '?' + query
    return path

def normalize_pattern(path):
    path, sep, query = path.partition('?')
    path = re.sub(r'[^*$]+', lambda m: normalize(m[0]), path)
    if sep:
        query = re.sub(r'[^=&*$]+', lambda m: normalize(m[0]), query)
        path += '?' + query
    return path

def translite_pattern(path):
    parts = list(map(re.escape, path.split('*')))
    for i in range(1, len(parts)-1):
        parts[i] = f'(?>.*?{parts[i]})'
    parts[-1] = f'.*{parts[-1]}'
    return ''.join(parts)

def merge_entries(e1, e2):
    entry = Entry()
    entry.useragents = list(filter(set(e2.useragents).__contains__, e1.useragents))
    entry.rulelines = e1.rulelines + e2.rulelines
    entry.delay = e1.delay if e2.delay is None else e2.delay
    entry.req_rate = e1.req_rate if e2.req_rate is None else e2.req_rate
    return entry

def sort_rulelines(rulelines):
    def sortkey(line):
        if line.matcher is not None:
            return (True,)
        else:
            return (False, len(line.path), line.allowance)
    rulelines.sort(key=sortkey, reverse=True)
