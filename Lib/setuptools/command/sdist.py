from distutils.command.sdist import sdist as _sdist
from distutils.util import convert_path
import os, re, sys, pkg_resources

entities = [
    ("&lt;","<"), ("&gt;", ">"), ("&quot;", '"'), ("&apos;", "'"),
    ("&amp;", "&")
]

def unescape(data):
    for old,new in entities:
        data = data.replace(old,new)
    return data

def re_finder(pattern, postproc=None):
    def find(dirname, filename):
        f = open(filename,'rU')
        data = f.read()
        f.close()
        for match in pattern.finditer(data):
            path = match.group(1)
            if postproc:
                path = postproc(path)
            yield joinpath(dirname,path)
    return find

def joinpath(prefix,suffix):
    if not prefix:
        return suffix
    return os.path.join(prefix,suffix)











def walk_revctrl(dirname=''):
    """Find all files under revision control"""
    for ep in pkg_resources.iter_entry_points('setuptools.file_finders'):
        for item in ep.load()(dirname):
            yield item

def _default_revctrl(dirname=''):
    for path, finder in finders:
        path = joinpath(dirname,path)
        if os.path.isfile(path):
            for path in finder(dirname,path):
                if os.path.isfile(path):
                    yield path
                elif os.path.isdir(path):
                    for item in _default_revctrl(path):
                        yield item

def externals_finder(dirname, filename):
    """Find any 'svn:externals' directories"""
    found = False
    f = open(filename,'rb')
    for line in iter(f.readline, ''):    # can't use direct iter!
        parts = line.split()
        if len(parts)==2:
            kind,length = parts
            data = f.read(int(length))
            if kind=='K' and data=='svn:externals':
                found = True
            elif kind=='V' and found:
                f.close()
                break
    else:
        f.close()
        return

    for line in data.splitlines():
        parts = line.split()
        if parts:
            yield joinpath(dirname, parts[0])


finders = [
    (convert_path('CVS/Entries'),
        re_finder(re.compile(r"^\w?/([^/]+)/", re.M))),
    (convert_path('.svn/entries'),
        re_finder(
            re.compile(r'name="([^"]+)"(?![^>]+deleted="true")', re.I),
            unescape
        )
    ),
    (convert_path('.svn/dir-props'), externals_finder),
]






























class sdist(_sdist):
    """Smart sdist that finds anything supported by revision control"""

    user_options = [
        ('formats=', None,
         "formats for source distribution (comma-separated list)"),
        ('keep-temp', 'k',
         "keep the distribution tree around after creating " +
         "archive file(s)"),
        ('dist-dir=', 'd',
         "directory to put the source distribution archive(s) in "
         "[default: dist]"),
        ]

    negative_opt = {}

    def run(self):
        self.run_command('egg_info')
        ei_cmd = self.get_finalized_command('egg_info')
        self.filelist = ei_cmd.filelist
        self.filelist.append(os.path.join(ei_cmd.egg_info,'SOURCES.txt'))

        self.check_metadata()
        self.make_distribution()        

        dist_files = getattr(self.distribution,'dist_files',[])
        for file in self.archive_files:
            data = ('sdist', '', file)
            if data not in dist_files:
                dist_files.append(data)

    def read_template(self):
        try:
            _sdist.read_template(self)
        except:
            # grody hack to close the template file (MANIFEST.in)
            # this prevents easy_install's attempt at deleting the file from
            # dying and thus masking the real error
            sys.exc_info()[2].tb_next.tb_frame.f_locals['template'].close()
            raise

