"""Generic FAQ Wizard.

This is a CGI program that maintains a user-editable FAQ.  It uses RCS
to keep track of changes to individual FAQ entries.  It is fully
configurable; everything you might want to change when using this
program to maintain some other FAQ than the Python FAQ is contained in
the configuration module, faqconf.py.

Note that this is not an executable script; it's an importable module.
The actual script to place in cgi-bin is faqw.py.

"""

import sys, time, os, stat, re, cgi, faqconf
from faqconf import *                   # This imports all uppercase names
now = time.time()

class FileError:
    def __init__(self, file):
        self.file = file

class InvalidFile(FileError):
    pass

class NoSuchSection(FileError):
    def __init__(self, section):
        FileError.__init__(self, NEWFILENAME %(section, 1))
        self.section = section

class NoSuchFile(FileError):
    def __init__(self, file, why=None):
        FileError.__init__(self, file)
        self.why = why

def escape(s):
    s = s.replace('&', '&amp;')
    s = s.replace('<', '&lt;')
    s = s.replace('>', '&gt;')
    return s

def escapeq(s):
    s = escape(s)
    s = s.replace('"', '&quot;')
    return s

def _interpolate(format, args, kw):
    try:
        quote = kw['_quote']
    except KeyError:
        quote = 1
    d = (kw,) + args + (faqconf.__dict__,)
    m = MagicDict(d, quote)
    return format % m

def interpolate(format, *args, **kw):
    return _interpolate(format, args, kw)

def emit(format, *args, **kw):
    try:
        f = kw['_file']
    except KeyError:
        f = sys.stdout
    f.write(_interpolate(format, args, kw))

translate_prog = None

def translate(text, pre=0):
    global translate_prog
    if not translate_prog:
        translate_prog = prog = re.compile(
            r'\b(http|ftp|https)://\S+(\b|/)|\b[-.\w]+@[-.\w]+')
    else:
        prog = translate_prog
    i = 0
    list = []
    while 1:
        m = prog.search(text, i)
        if not m:
            break
        j = m.start()
        list.append(escape(text[i:j]))
        i = j
        url = m.group(0)
        while url[-1] in '();:,.?\'"<>':
            url = url[:-1]
        i = i + len(url)
        url = escape(url)
        if not pre or (pre and PROCESS_PREFORMAT):
            if ':' in url:
                repl = '<A HREF="%s">%s</A>' % (url, url)
            else:
                repl = '<A HREF="mailto:%s">%s</A>' % (url, url)
        else:
            repl = url
        list.append(repl)
    j = len(text)
    list.append(escape(text[i:j]))
    return ''.join(list)

def emphasize(line):
    return re.sub(r'\*([a-zA-Z]+)\*', r'<I>\1</I>', line)

revparse_prog = None

def revparse(rev):
    global revparse_prog
    if not revparse_prog:
        revparse_prog = re.compile(r'^(\d{1,3})\.(\d{1,4})$')
    m = revparse_prog.match(rev)
    if not m:
        return None
    [major, minor] = map(int, m.group(1, 2))
    return major, minor

logon = 0
def log(text):
    if logon:
        logfile = open("logfile", "a")
        logfile.write(text + "\n")
        logfile.close()

def load_cookies():
    if not os.environ.has_key('HTTP_COOKIE'):
        return {}
    raw = os.environ['HTTP_COOKIE']
    words = [s.strip() for s in raw.split(';')]
    cookies = {}
    for word in words:
        i = word.find('=')
        if i >= 0:
            key, value = word[:i], word[i+1:]
            cookies[key] = value
    return cookies

def load_my_cookie():
    cookies = load_cookies()
    try:
        value = cookies[COOKIE_NAME]
    except KeyError:
        return {}
    import urllib
    value = urllib.unquote(value)
    words = value.split('/')
    while len(words) < 3:
        words.append('')
    author = '/'.join(words[:-2])
    email = words[-2]
    password = words[-1]
    return {'author': author,
            'email': email,
            'password': password}

def send_my_cookie(ui):
    name = COOKIE_NAME
    value = "%s/%s/%s" % (ui.author, ui.email, ui.password)
    import urllib
    value = urllib.quote(value)
    then = now + COOKIE_LIFETIME
    gmt = time.gmtime(then)
    path = os.environ.get('SCRIPT_NAME', '/cgi-bin/')
    print("Set-Cookie: %s=%s; path=%s;" % (name, value, path), end=' ')
    print(time.strftime("expires=%a, %d-%b-%y %X GMT", gmt))

class MagicDict:

    def __init__(self, d, quote):
        self.__d = d
        self.__quote = quote

    def __getitem__(self, key):
        for d in self.__d:
            try:
                value = d[key]
                if value:
                    value = str(value)
                    if self.__quote:
                        value = escapeq(value)
                    return value
            except KeyError:
                pass
        return ''

class UserInput:

    def __init__(self):
        self.__form = cgi.FieldStorage()
        #log("\n\nbody: " + self.body)

    def __getattr__(self, name):
        if name[0] == '_':
            raise AttributeError
        try:
            value = self.__form[name].value
        except (TypeError, KeyError):
            value = ''
        else:
            value = value.strip()
        setattr(self, name, value)
        return value

    def __getitem__(self, key):
        return getattr(self, key)

class FaqEntry:

    def __init__(self, fp, file, sec_num):
        self.file = file
        self.sec, self.num = sec_num
        if fp:
            import rfc822
            self.__headers = rfc822.Message(fp)
            self.body = fp.read().strip()
        else:
            self.__headers = {'title': "%d.%d. " % sec_num}
            self.body = ''

    def __getattr__(self, name):
        if name[0] == '_':
            raise AttributeError
        key = '-'.join(name.split('_'))
        try:
            value = self.__headers[key]
        except KeyError:
            value = ''
        setattr(self, name, value)
        return value

    def __getitem__(self, key):
        return getattr(self, key)

    def load_version(self):
        command = interpolate(SH_RLOG_H, self)
        p = os.popen(command)
        version = ''
        while 1:
            line = p.readline()
            if not line:
                break
            if line[:5] == 'head:':
                version = line[5:].strip()
        p.close()
        self.version = version

    def getmtime(self):
        if not self.last_changed_date:
            return 0
        try:
            return os.stat(self.file)[stat.ST_MTIME]
        except os.error:
            return 0

    def emit_marks(self):
        mtime = self.getmtime()
        if mtime >= now - DT_VERY_RECENT:
            emit(MARK_VERY_RECENT, self)
        elif mtime >= now - DT_RECENT:
            emit(MARK_RECENT, self)

    def show(self, edit=1):
        emit(ENTRY_HEADER1, self)
        self.emit_marks()
        emit(ENTRY_HEADER2, self)
        pre = 0
        raw = 0
        for line in self.body.split('\n'):
            # Allow the user to insert raw html into a FAQ answer
            # (Skip Montanaro, with changes by Guido)
            tag = line.rstrip().lower()
            if tag == '<html>':
                raw = 1
                continue
            if tag == '</html>':
                raw = 0
                continue
            if raw:
                print(line)
                continue
            if not line.strip():
                if pre:
                    print('</PRE>')
                    pre = 0
                else:
                    print('<P>')
            else:
                if not line[0].isspace():
                    if pre:
                        print('</PRE>')
                        pre = 0
                else:
                    if not pre:
                        print('<PRE>')
                        pre = 1
                if '/' in line or '@' in line:
                    line = translate(line, pre)
                elif '<' in line or '&' in line:
                    line = escape(line)
                if not pre and '*' in line:
                    line = emphasize(line)
                print(line)
        if pre:
            print('</PRE>')
            pre = 0
        if edit:
            print('<P>')
            emit(ENTRY_FOOTER, self)
            if self.last_changed_date:
                emit(ENTRY_LOGINFO, self)
        print('<P>')

class FaqDir:

    entryclass = FaqEntry

    __okprog = re.compile(OKFILENAME)

    def __init__(self, dir=os.curdir):
        self.__dir = dir
        self.__files = None

    def __fill(self):
        if self.__files is not None:
            return
        self.__files = files = []
        okprog = self.__okprog
        for file in os.listdir(self.__dir):
            if self.__okprog.match(file):
                files.append(file)
        files.sort()

    def good(self, file):
        return self.__okprog.match(file)

    def parse(self, file):
        m = self.good(file)
        if not m:
            return None
        sec, num = m.group(1, 2)
        return int(sec), int(num)

    def list(self):
        # XXX Caller shouldn't modify result
        self.__fill()
        return self.__files

    def open(self, file):
        sec_num = self.parse(file)
        if not sec_num:
            raise InvalidFile(file)
        try:
            fp = open(file)
        except IOError as msg:
            raise NoSuchFile(file, msg)
        try:
            return self.entryclass(fp, file, sec_num)
        finally:
            fp.close()

    def show(self, file, edit=1):
        self.open(file).show(edit=edit)

    def new(self, section):
        if not SECTION_TITLES.has_key(section):
            raise NoSuchSection(section)
        maxnum = 0
        for file in self.list():
            sec, num = self.parse(file)
            if sec == section:
                maxnum = max(maxnum, num)
        sec_num = (section, maxnum+1)
        file = NEWFILENAME % sec_num
        return self.entryclass(None, file, sec_num)

class FaqWizard:

    def __init__(self):
        self.ui = UserInput()
        self.dir = FaqDir()

    def go(self):
        print('Content-type: text/html')
        req = self.ui.req or 'home'
        mname = 'do_%s' % req
        try:
            meth = getattr(self, mname)
        except AttributeError:
            self.error("Bad request type %r." % (req,))
        else:
            try:
                meth()
            except InvalidFile as exc:
                self.error("Invalid entry file name %s" % exc.file)
            except NoSuchFile as exc:
                self.error("No entry with file name %s" % exc.file)
            except NoSuchSection as exc:
                self.error("No section number %s" % exc.section)
        self.epilogue()

    def error(self, message, **kw):
        self.prologue(T_ERROR)
        emit(message, kw)

    def prologue(self, title, entry=None, **kw):
        emit(PROLOGUE, entry, kwdict=kw, title=escape(title))

    def epilogue(self):
        emit(EPILOGUE)

    def do_home(self):
        self.prologue(T_HOME)
        emit(HOME)

    def do_debug(self):
        self.prologue("FAQ Wizard Debugging")
        form = cgi.FieldStorage()
        cgi.print_form(form)
        cgi.print_environ(os.environ)
        cgi.print_directory()
        cgi.print_arguments()

    def do_search(self):
        query = self.ui.query
        if not query:
            self.error("Empty query string!")
            return
        if self.ui.querytype == 'simple':
            query = re.escape(query)
            queries = [query]
        elif self.ui.querytype in ('anykeywords', 'allkeywords'):
            words = filter(None, re.split('\W+', query))
            if not words:
                self.error("No keywords specified!")
                return
            words = map(lambda w: r'\b%s\b' % w, words)
            if self.ui.querytype[:3] == 'any':
                queries = ['|'.join(words)]
            else:
                # Each of the individual queries must match
                queries = words
        else:
            # Default to regular expression
            queries = [query]
        self.prologue(T_SEARCH)
        progs = []
        for query in queries:
            if self.ui.casefold == 'no':
                p = re.compile(query)
            else:
                p = re.compile(query, re.IGNORECASE)
            progs.append(p)
        hits = []
        for file in self.dir.list():
            try:
                entry = self.dir.open(file)
            except FileError:
                constants
            for p in progs:
                if not p.search(entry.title) and not p.search(entry.body):
                    break
            else:
                hits.append(file)
        if not hits:
            emit(NO_HITS, self.ui, count=0)
        elif len(hits) <= MAXHITS:
            if len(hits) == 1:
                emit(ONE_HIT, count=1)
            else:
                emit(FEW_HITS, count=len(hits))
            self.format_all(hits, headers=0)
        else:
            emit(MANY_HITS, count=len(hits))
            self.format_index(hits)

    def do_all(self):
        self.prologue(T_ALL)
        files = self.dir.list()
        self.last_changed(files)
        self.format_index(files, localrefs=1)
        self.format_all(files)

    def do_compat(self):
        files = self.dir.list()
        emit(COMPAT)
        self.last_changed(files)
        self.format_index(files, localrefs=1)
        self.format_all(files, edit=0)
        sys.exit(0)                     # XXX Hack to suppress epilogue

    def last_changed(self, files):
        latest = 0
        for file in files:
            entry = self.dir.open(file)
            if entry:
                mtime = mtime = entry.getmtime()
                if mtime > latest:
                    latest = mtime
        print(time.strftime(LAST_CHANGED, time.localtime(latest)))
        emit(EXPLAIN_MARKS)

    def format_all(self, files, edit=1, headers=1):
        sec = 0
        for file in files:
            try:
                entry = self.dir.open(file)
            except NoSuchFile:
                continue
            if headers and entry.sec != sec:
                sec = entry.sec
                try:
                    title = SECTION_TITLES[sec]
                except KeyError:
                    title = "Untitled"
                emit("\n<HR>\n<H1>%(sec)s. %(title)s</H1>\n",
                     sec=sec, title=title)
            entry.show(edit=edit)

    def do_index(self):
        self.prologue(T_INDEX)
        files = self.dir.list()
        self.last_changed(files)
        self.format_index(files, add=1)

    def format_index(self, files, add=0, localrefs=0):
        sec = 0
        for file in files:
            try:
                entry = self.dir.open(file)
            except NoSuchFile:
                continue
            if entry.sec != sec:
                if sec:
                    if add:
                        emit(INDEX_ADDSECTION, sec=sec)
                    emit(INDEX_ENDSECTION, sec=sec)
                sec = entry.sec
                try:
                    title = SECTION_TITLES[sec]
                except KeyError:
                    title = "Untitled"
                emit(INDEX_SECTION, sec=sec, title=title)
            if localrefs:
                emit(LOCAL_ENTRY, entry)
            else:
                emit(INDEX_ENTRY, entry)
            entry.emit_marks()
        if sec:
            if add:
                emit(INDEX_ADDSECTION, sec=sec)
            emit(INDEX_ENDSECTION, sec=sec)

    def do_recent(self):
        if not self.ui.days:
            days = 1
        else:
            days = float(self.ui.days)
        try:
            cutoff = now - days * 24 * 3600
        except OverflowError:
            cutoff = 0
        list = []
        for file in self.dir.list():
            entry = self.dir.open(file)
            if not entry:
                continue
            mtime = entry.getmtime()
            if mtime >= cutoff:
                list.append((mtime, file))
        list.sort()
        list.reverse()
        self.prologue(T_RECENT)
        if days <= 1:
            period = "%.2g hours" % (days*24)
        else:
            period = "%.6g days" % days
        if not list:
            emit(NO_RECENT, period=period)
        elif len(list) == 1:
            emit(ONE_RECENT, period=period)
        else:
            emit(SOME_RECENT, period=period, count=len(list))
        self.format_all(map(lambda (mtime, file): file, list), headers=0)
        emit(TAIL_RECENT)

    def do_roulette(self):
        import random
        files = self.dir.list()
        if not files:
            self.error("No entries.")
            return
        file = random.choice(files)
        self.prologue(T_ROULETTE)
        emit(ROULETTE)
        self.dir.show(file)

    def do_help(self):
        self.prologue(T_HELP)
        emit(HELP)

    def do_show(self):
        entry = self.dir.open(self.ui.file)
        self.prologue(T_SHOW)
        entry.show()

    def do_add(self):
        self.prologue(T_ADD)
        emit(ADD_HEAD)
        sections = SECTION_TITLES.items()
        sections.sort()
        for section, title in sections:
            emit(ADD_SECTION, section=section, title=title)
        emit(ADD_TAIL)

    def do_delete(self):
        self.prologue(T_DELETE)
        emit(DELETE)

    def do_log(self):
        entry = self.dir.open(self.ui.file)
        self.prologue(T_LOG, entry)
        emit(LOG, entry)
        self.rlog(interpolate(SH_RLOG, entry), entry)

    def rlog(self, command, entry=None):
        output = os.popen(command).read()
        sys.stdout.write('<PRE>')
        athead = 0
        lines = output.split('\n')
        while lines and not lines[-1]:
            del lines[-1]
        if lines:
            line = lines[-1]
            if line[:1] == '=' and len(line) >= 40 and \
               line == line[0]*len(line):
                del lines[-1]
        headrev = None
        for line in lines:
            if entry and athead and line[:9] == 'revision ':
                rev = line[9:].split()
                mami = revparse(rev)
                if not mami:
                    print(line)
                else:
                    emit(REVISIONLINK, entry, rev=rev, line=line)
                    if mami[1] > 1:
                        prev = "%d.%d" % (mami[0], mami[1]-1)
                        emit(DIFFLINK, entry, prev=prev, rev=rev)
                    if headrev:
                        emit(DIFFLINK, entry, prev=rev, rev=headrev)
                    else:
                        headrev = rev
                    print()
                athead = 0
            else:
                athead = 0
                if line[:1] == '-' and len(line) >= 20 and \
                   line == len(line) * line[0]:
                    athead = 1
                    sys.stdout.write('<HR>')
                else:
                    print(line)
        print('</PRE>')

    def do_revision(self):
        entry = self.dir.open(self.ui.file)
        rev = self.ui.rev
        mami = revparse(rev)
        if not mami:
            self.error("Invalid revision number: %r." % (rev,))
        self.prologue(T_REVISION, entry)
        self.shell(interpolate(SH_REVISION, entry, rev=rev))

    def do_diff(self):
        entry = self.dir.open(self.ui.file)
        prev = self.ui.prev
        rev = self.ui.rev
        mami = revparse(rev)
        if not mami:
            self.error("Invalid revision number: %r." % (rev,))
        if prev:
            if not revparse(prev):
                self.error("Invalid previous revision number: %r." % (prev,))
        else:
            prev = '%d.%d' % (mami[0], mami[1])
        self.prologue(T_DIFF, entry)
        self.shell(interpolate(SH_RDIFF, entry, rev=rev, prev=prev))

    def shell(self, command):
        output = os.popen(command).read()
        sys.stdout.write('<PRE>')
        print(escape(output))
        print('</PRE>')

    def do_new(self):
        entry = self.dir.new(section=int(self.ui.section))
        entry.version = '*new*'
        self.prologue(T_EDIT)
        emit(EDITHEAD)
        emit(EDITFORM1, entry, editversion=entry.version)
        emit(EDITFORM2, entry, load_my_cookie())
        emit(EDITFORM3)
        entry.show(edit=0)

    def do_edit(self):
        entry = self.dir.open(self.ui.file)
        entry.load_version()
        self.prologue(T_EDIT)
        emit(EDITHEAD)
        emit(EDITFORM1, entry, editversion=entry.version)
        emit(EDITFORM2, entry, load_my_cookie())
        emit(EDITFORM3)
        entry.show(edit=0)

    def do_review(self):
        send_my_cookie(self.ui)
        if self.ui.editversion == '*new*':
            sec, num = self.dir.parse(self.ui.file)
            entry = self.dir.new(section=sec)
            entry.version = "*new*"
            if entry.file != self.ui.file:
                self.error("Commit version conflict!")
                emit(NEWCONFLICT, self.ui, sec=sec, num=num)
                return
        else:
            entry = self.dir.open(self.ui.file)
            entry.load_version()
        # Check that the FAQ entry number didn't change
        if self.ui.title.split()[:1] != entry.title.split()[:1]:
            self.error("Don't change the entry number please!")
            return
        # Check that the edited version is the current version
        if entry.version != self.ui.editversion:
            self.error("Commit version conflict!")
            emit(VERSIONCONFLICT, entry, self.ui)
            return
        commit_ok = ((not PASSWORD
                      or self.ui.password == PASSWORD)
                     and self.ui.author
                     and '@' in self.ui.email
                     and self.ui.log)
        if self.ui.commit:
            if not commit_ok:
                self.cantcommit()
            else:
                self.commit(entry)
            return
        self.prologue(T_REVIEW)
        emit(REVIEWHEAD)
        entry.body = self.ui.body
        entry.title = self.ui.title
        entry.show(edit=0)
        emit(EDITFORM1, self.ui, entry)
        if commit_ok:
            emit(COMMIT)
        else:
            emit(NOCOMMIT_HEAD)
            self.errordetail()
            emit(NOCOMMIT_TAIL)
        emit(EDITFORM2, self.ui, entry, load_my_cookie())
        emit(EDITFORM3)

    def cantcommit(self):
        self.prologue(T_CANTCOMMIT)
        print(CANTCOMMIT_HEAD)
        self.errordetail()
        print(CANTCOMMIT_TAIL)

    def errordetail(self):
        if PASSWORD and self.ui.password != PASSWORD:
            emit(NEED_PASSWD)
        if not self.ui.log:
            emit(NEED_LOG)
        if not self.ui.author:
            emit(NEED_AUTHOR)
        if not self.ui.email:
            emit(NEED_EMAIL)

    def commit(self, entry):
        file = entry.file
        # Normalize line endings in body
        if '\r' in self.ui.body:
            self.ui.body = re.sub('\r\n?', '\n', self.ui.body)
        # Normalize whitespace in title
        self.ui.title = ' '.join(self.ui.title.split())
        # Check that there were any changes
        if self.ui.body == entry.body and self.ui.title == entry.title:
            self.error("You didn't make any changes!")
            return

        # need to lock here because otherwise the file exists and is not writable (on NT)
        command = interpolate(SH_LOCK, file=file)
        p = os.popen(command)
        output = p.read()

        try:
            os.unlink(file)
        except os.error:
            pass
        try:
            f = open(file, 'w')
        except IOError as why:
            self.error(CANTWRITE, file=file, why=why)
            return
        date = time.ctime(now)
        emit(FILEHEADER, self.ui, os.environ, date=date, _file=f, _quote=0)
        f.write('\n')
        f.write(self.ui.body)
        f.write('\n')
        f.close()

        import tempfile
        tf = tempfile.NamedTemporaryFile()
        emit(LOGHEADER, self.ui, os.environ, date=date, _file=tf)
        tf.flush()
        tf.seek(0)

        command = interpolate(SH_CHECKIN, file=file, tfn=tf.name)
        log("\n\n" + command)
        p = os.popen(command)
        output = p.read()
        sts = p.close()
        log("output: " + output)
        log("done: " + str(sts))
        log("TempFile:\n" + tf.read() + "end")

        if not sts:
            self.prologue(T_COMMITTED)
            emit(COMMITTED)
        else:
            self.error(T_COMMITFAILED)
            emit(COMMITFAILED, sts=sts)
        print('<PRE>%s</PRE>' % escape(output))

        try:
            os.unlink(tf.name)
        except os.error:
            pass

        entry = self.dir.open(file)
        entry.show()

wiz = FaqWizard()
wiz.go()
