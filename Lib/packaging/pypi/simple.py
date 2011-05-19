"""Spider using the screen-scraping "simple" PyPI API.

This module contains the class SimpleIndexCrawler, a simple spider that
can be used to find and retrieve distributions from a project index
(like the Python Package Index), using its so-called simple API (see
reference implementation available at http://pypi.python.org/simple/).
"""

import http.client
import re
import socket
import sys
import urllib.request
import urllib.parse
import urllib.error
import os


from fnmatch import translate
from packaging import logger
from packaging.metadata import Metadata
from packaging.version import get_version_predicate
from packaging import __version__ as packaging_version
from packaging.pypi.base import BaseClient
from packaging.pypi.dist import (ReleasesList, EXTENSIONS,
                                  get_infos_from_url, MD5_HASH)
from packaging.pypi.errors import (PackagingPyPIError, DownloadError,
                                    UnableToDownload, CantParseArchiveName,
                                    ReleaseNotFound, ProjectNotFound)
from packaging.pypi.mirrors import get_mirrors
from packaging.metadata import Metadata

__all__ = ['Crawler', 'DEFAULT_SIMPLE_INDEX_URL']

# -- Constants -----------------------------------------------
DEFAULT_SIMPLE_INDEX_URL = "http://a.pypi.python.org/simple/"
DEFAULT_HOSTS = ("*",)
SOCKET_TIMEOUT = 15
USER_AGENT = "Python-urllib/%s packaging/%s" % (
    sys.version[:3], packaging_version)

# -- Regexps -------------------------------------------------
EGG_FRAGMENT = re.compile(r'^egg=([-A-Za-z0-9_.]+)$')
HREF = re.compile("""href\\s*=\\s*['"]?([^'"> ]+)""", re.I)
URL_SCHEME = re.compile('([-+.a-z0-9]{2,}):', re.I).match

# This pattern matches a character entity reference (a decimal numeric
# references, a hexadecimal numeric reference, or a named reference).
ENTITY_SUB = re.compile(r'&(#(\d+|x[\da-fA-F]+)|[\w.:-]+);?').sub
REL = re.compile("""<([^>]*\srel\s*=\s*['"]?([^'">]+)[^>]*)>""", re.I)


def socket_timeout(timeout=SOCKET_TIMEOUT):
    """Decorator to add a socket timeout when requesting pages on PyPI.
    """
    def _socket_timeout(func):
        def _socket_timeout(self, *args, **kwargs):
            old_timeout = socket.getdefaulttimeout()
            if hasattr(self, "_timeout"):
                timeout = self._timeout
            socket.setdefaulttimeout(timeout)
            try:
                return func(self, *args, **kwargs)
            finally:
                socket.setdefaulttimeout(old_timeout)
        return _socket_timeout
    return _socket_timeout


def with_mirror_support():
    """Decorator that makes the mirroring support easier"""
    def wrapper(func):
        def wrapped(self, *args, **kwargs):
            try:
                return func(self, *args, **kwargs)
            except DownloadError:
                # if an error occurs, try with the next index_url
                if self._mirrors_tries >= self._mirrors_max_tries:
                    try:
                        self._switch_to_next_mirror()
                    except KeyError:
                        raise UnableToDownload("Tried all mirrors")
                else:
                    self._mirrors_tries += 1
                self._projects.clear()
                return wrapped(self, *args, **kwargs)
        return wrapped
    return wrapper


class Crawler(BaseClient):
    """Provides useful tools to request the Python Package Index simple API.

    You can specify both mirrors and mirrors_url, but mirrors_url will only be
    used if mirrors is set to None.

    :param index_url: the url of the simple index to search on.
    :param prefer_final: if the version is not mentioned, and the last
                         version is not a "final" one (alpha, beta, etc.),
                         pick up the last final version.
    :param prefer_source: if the distribution type is not mentioned, pick up
                          the source one if available.
    :param follow_externals: tell if following external links is needed or
                             not. Default is False.
    :param hosts: a list of hosts allowed to be processed while using
                  follow_externals=True. Default behavior is to follow all
                  hosts.
    :param follow_externals: tell if following external links is needed or
                             not. Default is False.
    :param mirrors_url: the url to look on for DNS records giving mirror
                        adresses.
    :param mirrors: a list of mirrors (see PEP 381).
    :param timeout: time in seconds to consider a url has timeouted.
    :param mirrors_max_tries": number of times to try requesting informations
                               on mirrors before switching.
    """

    def __init__(self, index_url=DEFAULT_SIMPLE_INDEX_URL, prefer_final=False,
                 prefer_source=True, hosts=DEFAULT_HOSTS,
                 follow_externals=False, mirrors_url=None, mirrors=None,
                 timeout=SOCKET_TIMEOUT, mirrors_max_tries=0):
        super(Crawler, self).__init__(prefer_final, prefer_source)
        self.follow_externals = follow_externals

        # mirroring attributes.
        if not index_url.endswith("/"):
            index_url += "/"
        # if no mirrors are defined, use the method described in PEP 381.
        if mirrors is None:
            mirrors = get_mirrors(mirrors_url)
        self._mirrors = set(mirrors)
        self._mirrors_used = set()
        self.index_url = index_url
        self._mirrors_max_tries = mirrors_max_tries
        self._mirrors_tries = 0
        self._timeout = timeout

        # create a regexp to match all given hosts
        self._allowed_hosts = re.compile('|'.join(map(translate, hosts))).match

        # we keep an index of pages we have processed, in order to avoid
        # scanning them multple time (eg. if there is multiple pages pointing
        # on one)
        self._processed_urls = []
        self._projects = {}

    @with_mirror_support()
    def search_projects(self, name=None, **kwargs):
        """Search the index for projects containing the given name.

        Return a list of names.
        """
        with self._open_url(self.index_url) as index:
            if '*' in name:
                name.replace('*', '.*')
            else:
                name = "%s%s%s" % ('*.?', name, '*.?')
            name = name.replace('*', '[^<]*')  # avoid matching end tag
            projectname = re.compile('<a[^>]*>(%s)</a>' % name, re.I)
            matching_projects = []

            index_content = index.read()

        # FIXME should use bytes I/O and regexes instead of decoding
        index_content = index_content.decode()

        for match in projectname.finditer(index_content):
            project_name = match.group(1)
            matching_projects.append(self._get_project(project_name))
        return matching_projects

    def get_releases(self, requirements, prefer_final=None,
                     force_update=False):
        """Search for releases and return a ReleaseList object containing
        the results.
        """
        predicate = get_version_predicate(requirements)
        if predicate.name.lower() in self._projects and not force_update:
            return self._projects.get(predicate.name.lower())
        prefer_final = self._get_prefer_final(prefer_final)
        logger.info('reading info on PyPI about %s', predicate.name)
        self._process_index_page(predicate.name)

        if predicate.name.lower() not in self._projects:
            raise ProjectNotFound()

        releases = self._projects.get(predicate.name.lower())
        releases.sort_releases(prefer_final=prefer_final)
        return releases

    def get_release(self, requirements, prefer_final=None):
        """Return only one release that fulfill the given requirements"""
        predicate = get_version_predicate(requirements)
        release = self.get_releases(predicate, prefer_final)\
                      .get_last(predicate)
        if not release:
            raise ReleaseNotFound("No release matches the given criterias")
        return release

    def get_distributions(self, project_name, version):
        """Return the distributions found on the index for the specific given
        release"""
        # as the default behavior of get_release is to return a release
        # containing the distributions, just alias it.
        return self.get_release("%s (%s)" % (project_name, version))

    def get_metadata(self, project_name, version):
        """Return the metadatas from the simple index.

        Currently, download one archive, extract it and use the PKG-INFO file.
        """
        release = self.get_distributions(project_name, version)
        if not release.metadata:
            location = release.get_distribution().unpack()
            pkg_info = os.path.join(location, 'PKG-INFO')
            release.metadata = Metadata(pkg_info)
        return release

    def _switch_to_next_mirror(self):
        """Switch to the next mirror (eg. point self.index_url to the next
        mirror url.

        Raise a KeyError if all mirrors have been tried.
        """
        self._mirrors_used.add(self.index_url)
        index_url = self._mirrors.pop()
        if not ("http://" or "https://" or "file://") in index_url:
            index_url = "http://%s" % index_url

        if not index_url.endswith("/simple"):
            index_url = "%s/simple/" % index_url

        self.index_url = index_url

    def _is_browsable(self, url):
        """Tell if the given URL can be browsed or not.

        It uses the follow_externals and the hosts list to tell if the given
        url is browsable or not.
        """
        # if _index_url is contained in the given URL, we are browsing the
        # index, and it's always "browsable".
        # local files are always considered browable resources
        if self.index_url in url or urllib.parse.urlparse(url)[0] == "file":
            return True
        elif self.follow_externals:
            if self._allowed_hosts(urllib.parse.urlparse(url)[1]):  # 1 is netloc
                return True
            else:
                return False
        return False

    def _is_distribution(self, link):
        """Tell if the given URL matches to a distribution name or not.
        """
        #XXX find a better way to check that links are distributions
        # Using a regexp ?
        for ext in EXTENSIONS:
            if ext in link:
                return True
        return False

    def _register_release(self, release=None, release_info={}):
        """Register a new release.

        Both a release or a dict of release_info can be provided, the prefered
        way (eg. the quicker) is the dict one.

        Return the list of existing releases for the given project.
        """
        # Check if the project already has a list of releases (refering to
        # the project name). If not, create a new release list.
        # Then, add the release to the list.
        if release:
            name = release.name
        else:
            name = release_info['name']
        if not name.lower() in self._projects:
            self._projects[name.lower()] = ReleasesList(name, index=self._index)

        if release:
            self._projects[name.lower()].add_release(release=release)
        else:
            name = release_info.pop('name')
            version = release_info.pop('version')
            dist_type = release_info.pop('dist_type')
            self._projects[name.lower()].add_release(version, dist_type,
                                                     **release_info)
        return self._projects[name.lower()]

    def _process_url(self, url, project_name=None, follow_links=True):
        """Process an url and search for distributions packages.

        For each URL found, if it's a download, creates a PyPIdistribution
        object. If it's a homepage and we can follow links, process it too.

        :param url: the url to process
        :param project_name: the project name we are searching for.
        :param follow_links: Do not want to follow links more than from one
                             level. This parameter tells if we want to follow
                             the links we find (eg. run recursively this
                             method on it)
        """
        with self._open_url(url) as f:
            base_url = f.url
            if url not in self._processed_urls:
                self._processed_urls.append(url)
                link_matcher = self._get_link_matcher(url)
                for link, is_download in link_matcher(f.read().decode(), base_url):
                    if link not in self._processed_urls:
                        if self._is_distribution(link) or is_download:
                            self._processed_urls.append(link)
                            # it's a distribution, so create a dist object
                            try:
                                infos = get_infos_from_url(link, project_name,
                                            is_external=not self.index_url in url)
                            except CantParseArchiveName as e:
                                logger.warning(
                                    "version has not been parsed: %s", e)
                            else:
                                self._register_release(release_info=infos)
                        else:
                            if self._is_browsable(link) and follow_links:
                                self._process_url(link, project_name,
                                    follow_links=False)

    def _get_link_matcher(self, url):
        """Returns the right link matcher function of the given url
        """
        if self.index_url in url:
            return self._simple_link_matcher
        else:
            return self._default_link_matcher

    def _get_full_url(self, url, base_url):
        return urllib.parse.urljoin(base_url, self._htmldecode(url))

    def _simple_link_matcher(self, content, base_url):
        """Yield all links with a rel="download" or rel="homepage".

        This matches the simple index requirements for matching links.
        If follow_externals is set to False, dont yeld the external
        urls.

        :param content: the content of the page we want to parse
        :param base_url: the url of this page.
        """
        for match in HREF.finditer(content):
            url = self._get_full_url(match.group(1), base_url)
            if MD5_HASH.match(url):
                yield (url, True)

        for match in REL.finditer(content):
            # search for rel links.
            tag, rel = match.groups()
            rels = [s.strip() for s in rel.lower().split(',')]
            if 'homepage' in rels or 'download' in rels:
                for match in HREF.finditer(tag):
                    url = self._get_full_url(match.group(1), base_url)
                    if 'download' in rels or self._is_browsable(url):
                        # yield a list of (url, is_download)
                        yield (url, 'download' in rels)

    def _default_link_matcher(self, content, base_url):
        """Yield all links found on the page.
        """
        for match in HREF.finditer(content):
            url = self._get_full_url(match.group(1), base_url)
            if self._is_browsable(url):
                yield (url, False)

    @with_mirror_support()
    def _process_index_page(self, name):
        """Find and process a PyPI page for the given project name.

        :param name: the name of the project to find the page
        """
        # Browse and index the content of the given PyPI page.
        url = self.index_url + name + "/"
        self._process_url(url, name)

    @socket_timeout()
    def _open_url(self, url):
        """Open a urllib2 request, handling HTTP authentication, and local
        files support.

        """
        scheme, netloc, path, params, query, frag = urllib.parse.urlparse(url)

        # authentication stuff
        if scheme in ('http', 'https'):
            auth, host = urllib.parse.splituser(netloc)
        else:
            auth = None

        # add index.html automatically for filesystem paths
        if scheme == 'file':
            if url.endswith('/'):
                url += "index.html"

        # add authorization headers if auth is provided
        if auth:
            auth = "Basic " + \
                urllib.parse.unquote(auth).encode('base64').strip()
            new_url = urllib.parse.urlunparse((
                scheme, host, path, params, query, frag))
            request = urllib.request.Request(new_url)
            request.add_header("Authorization", auth)
        else:
            request = urllib.request.Request(url)
        request.add_header('User-Agent', USER_AGENT)
        try:
            fp = urllib.request.urlopen(request)
        except (ValueError, http.client.InvalidURL) as v:
            msg = ' '.join([str(arg) for arg in v.args])
            raise PackagingPyPIError('%s %s' % (url, msg))
        except urllib.error.HTTPError as v:
            return v
        except urllib.error.URLError as v:
            raise DownloadError("Download error for %s: %s" % (url, v.reason))
        except http.client.BadStatusLine as v:
            raise DownloadError('%s returned a bad status line. '
                'The server might be down, %s' % (url, v.line))
        except http.client.HTTPException as v:
            raise DownloadError("Download error for %s: %s" % (url, v))
        except socket.timeout:
            raise DownloadError("The server timeouted")

        if auth:
            # Put authentication info back into request URL if same host,
            # so that links found on the page will work
            s2, h2, path2, param2, query2, frag2 = \
                urllib.parse.urlparse(fp.url)
            if s2 == scheme and h2 == host:
                fp.url = urllib.parse.urlunparse(
                    (s2, netloc, path2, param2, query2, frag2))
        return fp

    def _decode_entity(self, match):
        what = match.group(1)
        if what.startswith('#x'):
            what = int(what[2:], 16)
        elif what.startswith('#'):
            what = int(what[1:])
        else:
            from html.entities import name2codepoint
            what = name2codepoint.get(what, match.group(0))
        return chr(what)

    def _htmldecode(self, text):
        """Decode HTML entities in the given text."""
        return ENTITY_SUB(self._decode_entity, text)
