""" robotparser.py

    Copyright (C) 2000  Bastian Kleineidam

    You can choose between two licenses when using this package:
    1) GNU GPLv2
    2) PYTHON 2.0 OPEN SOURCE LICENSE

    The robots.txt Exclusion Protocol is implemented as specified in
    http://info.webcrawler.com/mak/projects/robots/norobots-rfc.html
"""
import re,urlparse,urllib

__all__ = ["RobotFileParser"]

debug = 0

def _debug(msg):
    if debug: print msg


class RobotFileParser:
    def __init__(self, url=''):
        self.entries = []
        self.disallow_all = 0
        self.allow_all = 0
        self.set_url(url)
        self.last_checked = 0

    def mtime(self):
        return self.last_checked

    def modified(self):
        import time
        self.last_checked = time.time()

    def set_url(self, url):
        self.url = url
        self.host, self.path = urlparse.urlparse(url)[1:3]

    def read(self):
        import httplib
        tries = 0
        while tries<5:
            connection = httplib.HTTP(self.host)
            connection.putrequest("GET", self.path)
            connection.putheader("Host", self.host)
            connection.endheaders()
            status, text, mime = connection.getreply()
            if status in [301,302] and mime:
                tries = tries + 1
                newurl = mime.get("Location", mime.get("Uri", ""))
                newurl = urlparse.urljoin(self.url, newurl)
                self.set_url(newurl)
            else:
                break
        if status==401 or status==403:
            self.disallow_all = 1
        elif status>=400:
            self.allow_all = 1
        else:
            # status < 400
            self.parse(connection.getfile().readlines())

    def parse(self, lines):
        """parse the input lines from a robot.txt file.
           We allow that a user-agent: line is not preceded by
           one or more blank lines."""
        state = 0
        linenumber = 0
        entry = Entry()

        for line in lines:
            line = line.strip()
            linenumber = linenumber + 1
            if not line:
                if state==1:
                    _debug("line %d: warning: you should insert"
                           " allow: or disallow: directives below any"
                           " user-agent: line" % linenumber)
                    entry = Entry()
                    state = 0
                elif state==2:
                    self.entries.append(entry)
                    entry = Entry()
                    state = 0
            # remove optional comment and strip line
            i = line.find('#')
            if i>=0:
                line = line[:i]
            line = line.strip()
            if not line:
                continue
            line = line.split(':', 1)
            if len(line) == 2:
                line[0] = line[0].strip().lower()
                line[1] = line[1].strip()
                if line[0] == "user-agent":
                    if state==2:
                        _debug("line %d: warning: you should insert a blank"
                               " line before any user-agent"
                               " directive" % linenumber)
                        self.entries.append(entry)
                        entry = Entry()
                    entry.useragents.append(line[1])
                    state = 1
                elif line[0] == "disallow":
                    if state==0:
                        _debug("line %d: error: you must insert a user-agent:"
                               " directive before this line" % linenumber)
                    else:
                        entry.rulelines.append(RuleLine(line[1], 0))
                        state = 2
                elif line[0] == "allow":
                    if state==0:
                        _debug("line %d: error: you must insert a user-agent:"
                               " directive before this line" % linenumber)
                    else:
                        entry.rulelines.append(RuleLine(line[1], 1))
                else:
                    _debug("line %d: warning: unknown key %s" % (linenumber,
                               line[0]))
            else:
                _debug("line %d: error: malformed line %s"%(linenumber, line))
        if state==2:
            self.entries.append(entry)
        _debug("Parsed rules:\n%s" % str(self))


    def can_fetch(self, useragent, url):
        """using the parsed robots.txt decide if useragent can fetch url"""
        _debug("Checking robot.txt allowance for\n%s\n%s" % (useragent, url))
        if self.disallow_all:
            return 0
        if self.allow_all:
            return 1
        # search for given user agent matches
        # the first match counts
        useragent = useragent.lower()
        url = urllib.quote(urlparse.urlparse(url)[2])
        for entry in self.entries:
            if entry.applies_to(useragent):
                return entry.allowance(url)
        # agent not found ==> access granted
        return 1


    def __str__(self):
        ret = ""
        for entry in self.entries:
            ret = ret + str(entry) + "\n"
        return ret


class RuleLine:
    """A rule line is a single "Allow:" (allowance==1) or "Disallow:"
       (allowance==0) followed by a path."""
    def __init__(self, path, allowance):
        self.path = urllib.quote(path)
        self.allowance = allowance

    def applies_to(self, filename):
        return self.path=="*" or re.match(self.path, filename)

    def __str__(self):
        return (self.allowance and "Allow" or "Disallow")+": "+self.path


class Entry:
    """An entry has one or more user-agents and zero or more rulelines"""
    def __init__(self):
        self.useragents = []
        self.rulelines = []

    def __str__(self):
        ret = ""
        for agent in self.useragents:
            ret = ret + "User-agent: "+agent+"\n"
        for line in self.rulelines:
            ret = ret + str(line) + "\n"
        return ret

    def applies_to(self, useragent):
        "check if this entry applies to the specified agent"
        for agent in self.useragents:
            if agent=="*":
                return 1
            if re.match(agent, useragent):
                return 1
        return 0

    def allowance(self, filename):
        """Preconditions:
        - our agent applies to this entry
        - filename is URL decoded"""
        for line in self.rulelines:
            if line.applies_to(filename):
                return line.allowance
        return 1


def _test():
    global debug
    import sys
    rp = RobotFileParser()
    debug = 1
    if len(sys.argv) <= 1:
        rp.set_url('http://www.musi-cal.com/robots.txt')
        rp.read()
    else:
        rp.parse(open(sys.argv[1]).readlines())
    print rp.can_fetch('*', 'http://www.musi-cal.com/')
    print rp.can_fetch('Musi-Cal-Robot/1.0',
                       'http://www.musi-cal.com/cgi-bin/event-search'
                       '?city=San+Francisco')

if __name__ == '__main__':
    _test()
