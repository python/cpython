"""PyPI and direct package downloading"""

import sys, os.path, re, urlparse, urllib2, shutil, random, socket
from pkg_resources import *
from distutils import log
from distutils.errors import DistutilsError
from md5 import md5
from fnmatch import translate

EGG_FRAGMENT = re.compile(r'^egg=([-A-Za-z0-9_.]+)$')
HREF = re.compile("""href\\s*=\\s*['"]?([^'"> ]+)""", re.I)
# this is here to fix emacs' cruddy broken syntax highlighting
PYPI_MD5 = re.compile(
    '<a href="([^"#]+)">([^<]+)</a>\n\s+\\(<a href="[^?]+\?:action=show_md5'
    '&amp;digest=([0-9a-f]{32})">md5</a>\\)'
)

URL_SCHEME = re.compile('([-+.a-z0-9]{2,}):',re.I).match
EXTENSIONS = ".tar.gz .tar.bz2 .tar .zip .tgz".split()

__all__ = [
    'PackageIndex', 'distros_for_url', 'parse_bdist_wininst',
    'interpret_distro_name',
]


def parse_bdist_wininst(name):
    """Return (base,pyversion) or (None,None) for possible .exe name"""

    lower = name.lower()
    base, py_ver = None, None

    if lower.endswith('.exe'):
        if lower.endswith('.win32.exe'):
            base = name[:-10]
        elif lower.startswith('.win32-py',-16):
            py_ver = name[-7:-4]
            base = name[:-16]

    return base,py_ver

def egg_info_for_url(url):
    scheme, server, path, parameters, query, fragment = urlparse.urlparse(url)
    base = urllib2.unquote(path.split('/')[-1])
    if '#' in base: base, fragment = base.split('#',1)
    return base,fragment

def distros_for_url(url, metadata=None):
    """Yield egg or source distribution objects that might be found at a URL"""
    base, fragment = egg_info_for_url(url)
    dists = distros_for_location(url, base, metadata)
    if fragment and not dists:
        match = EGG_FRAGMENT.match(fragment)
        if match:
            return interpret_distro_name(
                url, match.group(1), metadata, precedence = CHECKOUT_DIST
            )
    return dists

def distros_for_location(location, basename, metadata=None):
    """Yield egg or source distribution objects based on basename"""
    if basename.endswith('.egg.zip'):
        basename = basename[:-4]    # strip the .zip
    if basename.endswith('.egg'):   # only one, unambiguous interpretation
        return [Distribution.from_location(location, basename, metadata)]

    if basename.endswith('.exe'):
        win_base, py_ver = parse_bdist_wininst(basename)
        if win_base is not None:
            return interpret_distro_name(
                location, win_base, metadata, py_ver, BINARY_DIST, "win32"
            )

    # Try source distro extensions (.zip, .tgz, etc.)
    #
    for ext in EXTENSIONS:
        if basename.endswith(ext):
            basename = basename[:-len(ext)]
            return interpret_distro_name(location, basename, metadata)
    return []  # no extension matched


def distros_for_filename(filename, metadata=None):
    """Yield possible egg or source distribution objects based on a filename"""
    return distros_for_location(
        normalize_path(filename), os.path.basename(filename), metadata
    )


def interpret_distro_name(location, basename, metadata,
    py_version=None, precedence=SOURCE_DIST, platform=None
):
    """Generate alternative interpretations of a source distro name

    Note: if `location` is a filesystem filename, you should call
    ``pkg_resources.normalize_path()`` on it before passing it to this
    routine!
    """

    # Generate alternative interpretations of a source distro name
    # Because some packages are ambiguous as to name/versions split
    # e.g. "adns-python-1.1.0", "egenix-mx-commercial", etc.
    # So, we generate each possible interepretation (e.g. "adns, python-1.1.0"
    # "adns-python, 1.1.0", and "adns-python-1.1.0, no version").  In practice,
    # the spurious interpretations should be ignored, because in the event
    # there's also an "adns" package, the spurious "python-1.1.0" version will
    # compare lower than any numeric version number, and is therefore unlikely
    # to match a request for it.  It's still a potential problem, though, and
    # in the long run PyPI and the distutils should go for "safe" names and
    # versions in distribution archive names (sdist and bdist).

    parts = basename.split('-')
    for p in range(1,len(parts)+1):
        yield Distribution(
            location, metadata, '-'.join(parts[:p]), '-'.join(parts[p:]),
            py_version=py_version, precedence = precedence,
            platform = platform
        )





class PackageIndex(Environment):
    """A distribution index that scans web pages for download URLs"""

    def __init__(self,index_url="http://www.python.org/pypi",hosts=('*',),*args,**kw):
        Environment.__init__(self,*args,**kw)
        self.index_url = index_url + "/"[:not index_url.endswith('/')]
        self.scanned_urls = {}
        self.fetched_urls = {}
        self.package_pages = {}
        self.allows = re.compile('|'.join(map(translate,hosts))).match
        self.to_scan = []

    def process_url(self, url, retrieve=False):
        """Evaluate a URL as a possible download, and maybe retrieve it"""
        url = fix_sf_url(url)
        if url in self.scanned_urls and not retrieve:
            return
        self.scanned_urls[url] = True
        if not URL_SCHEME(url):
            self.process_filename(url)
            return
        else:
            dists = list(distros_for_url(url))
            if dists:
                if not self.url_ok(url):
                    return
                self.debug("Found link: %s", url)

        if dists or not retrieve or url in self.fetched_urls:
            map(self.add, dists)
            return  # don't need the actual page

        if not self.url_ok(url):
            self.fetched_urls[url] = True
            return

        self.info("Reading %s", url)
        f = self.open_url(url)
        self.fetched_urls[url] = self.fetched_urls[f.url] = True


        if 'html' not in f.headers['content-type'].lower():
            f.close()   # not html, we can't process it
            return

        base = f.url     # handle redirects
        page = f.read()
        f.close()
        if url.startswith(self.index_url):
            page = self.process_index(url, page)

        for match in HREF.finditer(page):
            link = urlparse.urljoin(base, match.group(1))
            self.process_url(link)

    def process_filename(self, fn, nested=False):
        # process filenames or directories
        if not os.path.exists(fn):
            self.warn("Not found: %s", url)
            return

        if os.path.isdir(fn) and not nested:
            path = os.path.realpath(fn)
            for item in os.listdir(path):
                self.process_filename(os.path.join(path,item), True)

        dists = distros_for_filename(fn)
        if dists:
            self.debug("Found: %s", fn)
            map(self.add, dists)

    def url_ok(self, url, fatal=False):
        if self.allows(urlparse.urlparse(url)[1]):
            return True
        msg = "\nLink to % s ***BLOCKED*** by --allow-hosts\n"
        if fatal:
            raise DistutilsError(msg % url)
        else:
            self.warn(msg, url)



    def process_index(self,url,page):
        """Process the contents of a PyPI page"""
        def scan(link):
            # Process a URL to see if it's for a package page
            if link.startswith(self.index_url):
                parts = map(
                    urllib2.unquote, link[len(self.index_url):].split('/')
                )
                if len(parts)==2:
                    # it's a package page, sanitize and index it
                    pkg = safe_name(parts[0])
                    ver = safe_version(parts[1])
                    self.package_pages.setdefault(pkg.lower(),{})[link] = True
                    return to_filename(pkg), to_filename(ver)
            return None, None

        if url==self.index_url or 'Index of Packages</title>' in page:
            # process an index page into the package-page index
            for match in HREF.finditer(page):
                scan( urlparse.urljoin(url, match.group(1)) )
        else:
            pkg,ver = scan(url)   # ensure this page is in the page index
            # process individual package page
            for tag in ("<th>Home Page", "<th>Download URL"):
                pos = page.find(tag)
                if pos!=-1:
                    match = HREF.search(page,pos)
                    if match:
                        # Process the found URL
                        new_url = urlparse.urljoin(url, match.group(1))
                        base, frag = egg_info_for_url(new_url)
                        if base.endswith('.py') and not frag:
                            if pkg and ver:
                                new_url+='#egg=%s-%s' % (pkg,ver)
                            else:
                                self.need_version_info(url)
                        self.scan_url(new_url)
        return PYPI_MD5.sub(
            lambda m: '<a href="%s#md5=%s">%s</a>' % m.group(1,3,2), page
        )

    def need_version_info(self, url):
        self.scan_all(
            "Page at %s links to .py file(s) without version info; an index "
            "scan is required.", url
        )

    def scan_all(self, msg=None, *args):
        if self.index_url not in self.fetched_urls:
            if msg: self.warn(msg,*args)
            self.warn(
                "Scanning index of all packages (this may take a while)"
            )
        self.scan_url(self.index_url)

    def find_packages(self, requirement):
        self.scan_url(self.index_url + requirement.unsafe_name+'/')
        
        if not self.package_pages.get(requirement.key):
            # Fall back to safe version of the name
            self.scan_url(self.index_url + requirement.project_name+'/')

        if not self.package_pages.get(requirement.key):
            # We couldn't find the target package, so search the index page too
            self.warn(
                "Couldn't find index page for %r (maybe misspelled?)",
                requirement.unsafe_name
            )
            self.scan_all()

        for url in self.package_pages.get(requirement.key,()):
            # scan each page that might be related to the desired package
            self.scan_url(url)

    def obtain(self, requirement, installer=None):
        self.prescan(); self.find_packages(requirement)
        for dist in self[requirement.key]:
            if dist in requirement:
                return dist
            self.debug("%s does not match %s", requirement, dist)
        return super(PackageIndex, self).obtain(requirement,installer)

    def check_md5(self, cs, info, filename, tfp):
        if re.match('md5=[0-9a-f]{32}$', info):
            self.debug("Validating md5 checksum for %s", filename)
            if cs.hexdigest()<>info[4:]:
                tfp.close()
                os.unlink(filename)
                raise DistutilsError(
                    "MD5 validation failed for "+os.path.basename(filename)+
                    "; possible download problem?"
                )

    def add_find_links(self, urls):
        """Add `urls` to the list that will be prescanned for searches"""
        for url in urls:
            if (
                self.to_scan is None        # if we have already "gone online"
                or not URL_SCHEME(url)      # or it's a local file/directory
                or url.startswith('file:')
                or list(distros_for_url(url))   # or a direct package link
            ):
                # then go ahead and process it now
                self.scan_url(url)
            else:
                # otherwise, defer retrieval till later
                self.to_scan.append(url)

    def prescan(self):
        """Scan urls scheduled for prescanning (e.g. --find-links)"""
        if self.to_scan:
            map(self.scan_url, self.to_scan)
        self.to_scan = None     # from now on, go ahead and process immediately










    def download(self, spec, tmpdir):
        """Locate and/or download `spec` to `tmpdir`, returning a local path

        `spec` may be a ``Requirement`` object, or a string containing a URL,
        an existing local filename, or a project/version requirement spec
        (i.e. the string form of a ``Requirement`` object).  If it is the URL
        of a .py file with an unambiguous ``#egg=name-version`` tag (i.e., one
        that escapes ``-`` as ``_`` throughout), a trivial ``setup.py`` is
        automatically created alongside the downloaded file.

        If `spec` is a ``Requirement`` object or a string containing a
        project/version requirement spec, this method returns the location of
        a matching distribution (possibly after downloading it to `tmpdir`).
        If `spec` is a locally existing file or directory name, it is simply
        returned unchanged.  If `spec` is a URL, it is downloaded to a subpath
        of `tmpdir`, and the local filename is returned.  Various errors may be
        raised if a problem occurs during downloading.
        """
        if not isinstance(spec,Requirement):
            scheme = URL_SCHEME(spec)
            if scheme:
                # It's a url, download it to tmpdir
                found = self._download_url(scheme.group(1), spec, tmpdir)
                base, fragment = egg_info_for_url(spec)
                if base.endswith('.py'):
                    found = self.gen_setup(found,fragment,tmpdir)
                return found
            elif os.path.exists(spec):
                # Existing file or directory, just return it
                return spec
            else:
                try:
                    spec = Requirement.parse(spec)
                except ValueError:
                    raise DistutilsError(
                        "Not a URL, existing file, or requirement spec: %r" %
                        (spec,)
                    )
        return getattr(self.fetch_distribution(spec, tmpdir),'location',None)


    def fetch_distribution(self,
        requirement, tmpdir, force_scan=False, source=False, develop_ok=False
    ):
        """Obtain a distribution suitable for fulfilling `requirement`

        `requirement` must be a ``pkg_resources.Requirement`` instance.
        If necessary, or if the `force_scan` flag is set, the requirement is
        searched for in the (online) package index as well as the locally
        installed packages.  If a distribution matching `requirement` is found,
        the returned distribution's ``location`` is the value you would have
        gotten from calling the ``download()`` method with the matching
        distribution's URL or filename.  If no matching distribution is found,
        ``None`` is returned.

        If the `source` flag is set, only source distributions and source
        checkout links will be considered.  Unless the `develop_ok` flag is
        set, development and system eggs (i.e., those using the ``.egg-info``
        format) will be ignored.
        """

        # process a Requirement
        self.info("Searching for %s", requirement)
        skipped = {}

        def find(req):
            # Find a matching distribution; may be called more than once

            for dist in self[req.key]:

                if dist.precedence==DEVELOP_DIST and not develop_ok:
                    if dist not in skipped:
                        self.warn("Skipping development or system egg: %s",dist)
                        skipped[dist] = 1
                    continue

                if dist in req and (dist.precedence<=SOURCE_DIST or not source):
                    self.info("Best match: %s", dist)
                    return dist.clone(
                        location=self.download(dist.location, tmpdir)
                    )

        if force_scan:
            self.prescan()
            self.find_packages(requirement)

        dist = find(requirement)
        if dist is None and self.to_scan is not None:
            self.prescan()
            dist = find(requirement)

        if dist is None and not force_scan:
            self.find_packages(requirement)
            dist = find(requirement)

        if dist is None:
            self.warn(
                "No local packages or download links found for %s%s",
                (source and "a source distribution of " or ""),
                requirement,
            )
        return dist

    def fetch(self, requirement, tmpdir, force_scan=False, source=False):
        """Obtain a file suitable for fulfilling `requirement`

        DEPRECATED; use the ``fetch_distribution()`` method now instead.  For
        backward compatibility, this routine is identical but returns the
        ``location`` of the downloaded distribution instead of a distribution
        object.
        """
        dist = self.fetch_distribution(requirement,tmpdir,force_scan,source)
        if dist is not None:
            return dist.location
        return None








    def gen_setup(self, filename, fragment, tmpdir):
        match = EGG_FRAGMENT.match(fragment); #import pdb; pdb.set_trace()
        dists = match and [d for d in
            interpret_distro_name(filename, match.group(1), None) if d.version
        ] or []

        if len(dists)==1:   # unambiguous ``#egg`` fragment
            basename = os.path.basename(filename)

            # Make sure the file has been downloaded to the temp dir.
            if os.path.dirname(filename) != tmpdir:
                dst = os.path.join(tmpdir, basename)
                from setuptools.command.easy_install import samefile
                if not samefile(filename, dst):
                    shutil.copy2(filename, dst)
                    filename=dst

            file = open(os.path.join(tmpdir, 'setup.py'), 'w')
            file.write(
                "from setuptools import setup\n"
                "setup(name=%r, version=%r, py_modules=[%r])\n"
                % (
                    dists[0].project_name, dists[0].version,
                    os.path.splitext(basename)[0]
                )
            )
            file.close()
            return filename

        elif match:
            raise DistutilsError(
                "Can't unambiguously interpret project/version identifier %r; "
                "any dashes in the name or version should be escaped using "
                "underscores. %r" % (fragment,dists)
            )
        else:
            raise DistutilsError(
                "Can't process plain .py files without an '#egg=name-version'"
                " suffix to enable automatic setup script generation."
            )
        
    dl_blocksize = 8192
    def _download_to(self, url, filename):
        self.url_ok(url,True)   # raises error if not allowed
        self.info("Downloading %s", url)
        # Download the file
        fp, tfp, info = None, None, None
        try:
            if '#' in url:
                url, info = url.split('#', 1)
            fp = self.open_url(url)
            if isinstance(fp, urllib2.HTTPError):
                raise DistutilsError(
                    "Can't download %s: %s %s" % (url, fp.code,fp.msg)
                )
            cs = md5()
            headers = fp.info()
            blocknum = 0
            bs = self.dl_blocksize
            size = -1
            if "content-length" in headers:
                size = int(headers["Content-Length"])
                self.reporthook(url, filename, blocknum, bs, size)
            tfp = open(filename,'wb')
            while True:
                block = fp.read(bs)
                if block:
                    cs.update(block)
                    tfp.write(block)
                    blocknum += 1
                    self.reporthook(url, filename, blocknum, bs, size)
                else:
                    break
            if info: self.check_md5(cs, info, filename, tfp)
            return headers
        finally:
            if fp: fp.close()
            if tfp: tfp.close()

    def reporthook(self, url, filename, blocknum, blksize, size):
        pass    # no-op

    def retry_sf_download(self, url, filename):
        try:
            return self._download_to(url, filename)
        except:
            scheme, server, path, param, query, frag = urlparse.urlparse(url)
            if server!='dl.sourceforge.net':
                raise

        mirror = get_sf_ip()

        while _sf_mirrors:
            self.warn("Download failed: %s", sys.exc_info()[1])
            url = urlparse.urlunparse((scheme, mirror, path, param, '', frag))
            try:
                return self._download_to(url, filename)
            except:
                _sf_mirrors.remove(mirror)  # don't retry the same mirror
                mirror = get_sf_ip()

        raise   # fail if no mirror works





















    def open_url(self, url):
        try:
            return urllib2.urlopen(url)
        except urllib2.HTTPError, v:
            return v
        except urllib2.URLError, v:
            raise DistutilsError("Download error: %s" % v.reason)


    def _download_url(self, scheme, url, tmpdir):

        # Determine download filename
        #
        name = filter(None,urlparse.urlparse(url)[2].split('/'))
        if name:
            name = name[-1]
            while '..' in name:
                name = name.replace('..','.').replace('\\','_')
        else:
            name = "__downloaded__"    # default if URL has no path contents

        if name.endswith('.egg.zip'):
            name = name[:-4]    # strip the extra .zip before download

        filename = os.path.join(tmpdir,name)

        # Download the file
        #
        if scheme=='svn' or scheme.startswith('svn+'):
            return self._download_svn(url, filename)
        else:
            headers = self.retry_sf_download(url, filename)
            if 'html' in headers['content-type'].lower():
                return self._download_html(url, headers, filename, tmpdir)
            else:
                return filename

    def scan_url(self, url):
        self.process_url(url, True)


    def _download_html(self, url, headers, filename, tmpdir):
        file = open(filename)
        for line in file:
            if line.strip():
                # Check for a subversion index page
                if re.search(r'<title>Revision \d+:', line):
                    # it's a subversion index page:
                    file.close()
                    os.unlink(filename)
                    return self._download_svn(url, filename)
                break   # not an index page
        file.close()
        os.unlink(filename)
        raise DistutilsError("Unexpected HTML page found at "+url)

    def _download_svn(self, url, filename):
        url = url.split('#',1)[0]   # remove any fragment for svn's sake
        self.info("Doing subversion checkout from %s to %s", url, filename)
        os.system("svn checkout -q %s %s" % (url, filename))
        return filename

    def debug(self, msg, *args):
        log.debug(msg, *args)

    def info(self, msg, *args):
        log.info(msg, *args)

    def warn(self, msg, *args):
        log.warn(msg, *args)












def fix_sf_url(url):
    scheme, server, path, param, query, frag = urlparse.urlparse(url)
    if server!='prdownloads.sourceforge.net':
        return url
    return urlparse.urlunparse(
        (scheme, 'dl.sourceforge.net', 'sourceforge'+path, param, '', frag)
    )

_sf_mirrors = []

def get_sf_ip():
    if not _sf_mirrors:
        try:
            _sf_mirrors[:] = socket.gethostbyname_ex('dl.sourceforge.net')[-1]
        except socket.error:
            # DNS-bl0ck1n9 f1r3w4llz sUx0rs!
            _sf_mirrors[:] = ['dl.sourceforge.net']
    return random.choice(_sf_mirrors)























