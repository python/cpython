"""Wiki main program.  Imported and run by cgi3.py."""

import os, re, cgi, sys, tempfile
escape = cgi.escape

def main():
    form = cgi.FieldStorage()
    print "Content-type: text/html"
    print
    cmd = form.getvalue("cmd", "view")
    page = form.getvalue("page", "FrontPage")
    wiki = WikiPage(page)
    method = getattr(wiki, 'cmd_' + cmd, None) or wiki.cmd_view
    method(form)

class WikiPage:

    homedir = tempfile.gettempdir()
    scripturl = os.path.basename(sys.argv[0])

    def __init__(self, name):
        if not self.iswikiword(name):
            raise ValueError, "page name is not a wiki word"
        self.name = name
        self.load()

    def cmd_view(self, form):
        print "<h1>", escape(self.splitwikiword(self.name)), "</h1>"
        print "<p>"
        for line in self.data.splitlines():
            line = line.rstrip()
            if not line:
                print "<p>"
            else:
                print self.formatline(line)
        print "<hr>"
        print "<p>", self.mklink("edit", self.name, "Edit this page") + ";"
        print self.mklink("view", "FrontPage", "go to front page") + "."

    def formatline(self, line):
        words = []
        for word in re.split('(\W+)', line):
            if self.iswikiword(word):
                if os.path.isfile(self.mkfile(word)):
                    word = self.mklink("view", word, word)
                else:
                    word = self.mklink("new", word, word + "*")
            else:
                word = escape(word)
            words.append(word)
        return "".join(words)

    def cmd_edit(self, form, label="Change"):
        print "<h1>", label, self.name, "</h1>"
        print '<form method="POST" action="%s">' % self.scripturl
        s = '<textarea cols="70" rows="20" name="text">%s</textarea>'
        print s % self.data
        print '<input type="hidden" name="cmd" value="create">'
        print '<input type="hidden" name="page" value="%s">' % self.name
        print '<br>'
        print '<input type="submit" value="%s Page">' % label
        print "</form>"

    def cmd_create(self, form):
        self.data = form.getvalue("text", "").strip()
        error = self.store()
        if error:
            print "<h1>I'm sorry.  That didn't work</h1>"
            print "<p>An error occurred while attempting to write the file:"
            print "<p>", escape(error)
        else:
            # Use a redirect directive, to avoid "reload page" problems
            print "<head>"
            s = '<meta http-equiv="refresh" content="1; URL=%s">'
            print s % (self.scripturl + "?cmd=view&page=" + self.name)
            print "<head>"
            print "<h1>OK</h1>"
            print "<p>If nothing happens, please click here:",
            print self.mklink("view", self.name, self.name)

    def cmd_new(self, form):
        self.cmd_edit(form, label="Create")

    def iswikiword(self, word):
        return re.match("[A-Z][a-z]+([A-Z][a-z]*)+", word)

    def splitwikiword(self, word):
        chars = []
        for c in word:
            if chars and c.isupper():
                chars.append(' ')
            chars.append(c)
        return "".join(chars)

    def mkfile(self, name=None):
        if name is None:
            name = self.name
        return os.path.join(self.homedir, name + ".txt")

    def mklink(self, cmd, page, text):
        link = self.scripturl + "?cmd=" + cmd + "&page=" + page
        return '<a href="%s">%s</a>' % (link, text)

    def load(self):
        try:
            f = open(self.mkfile())
            data = f.read().strip()
            f.close()
        except IOError:
            data = ""
        self.data = data

    def store(self):
        data = self.data
        try:
            f = open(self.mkfile(), "w")
            f.write(data)
            if data and not data.endswith('\n'):
                f.write('\n')
            f.close()
            return ""
        except IOError, err:
            return "IOError: %s" % str(err)
