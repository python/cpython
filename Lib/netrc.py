"""An object-oriented interface to .netrc files."""

# Module and documentation by Eric S. Raymond, 21 Dec 1998

import os, shlex, stat

__all__ = ["netrc", "NetrcParseError"]


class NetrcParseError(Exception):
    """Exception raised on syntax errors in the .netrc file."""
    def __init__(self, msg, filename=None, lineno=None):
        self.filename = filename
        self.lineno = lineno
        self.msg = msg
        Exception.__init__(self, msg)

    def __str__(self):
        return "%s (%s, line %s)" % (self.msg, self.filename, self.lineno)


class _netrclex:
    def __init__(self, fp):
        self.lineno = 1
        self.instream = fp
        self.whitespace = "\n\t\r "
        self._stack = []

    def _read_char(self):
        ch = self.instream.read(1)
        if ch == "\n":
            self.lineno += 1
        return ch

    def get_token(self):
        if self._stack:
            return self._stack.pop(0)
        token = ""
        fiter = iter(self._read_char, "")
        for ch in fiter:
            if ch in self.whitespace:
                continue
            if ch == "\"":
                for ch in fiter:
                    if ch != "\"":
                        if ch == "\\":
                            ch = self._read_char()
                        token += ch
                        continue
                    return token
            else:
                if ch == "\\":
                    ch = self._read_char()
                token += ch
                for ch in fiter:
                    if ch not in self.whitespace:
                        if ch == "\\":
                            ch = self._read_char()
                        token += ch
                        continue
                    return token
        return token

    def push_token(self, token):
        self._stack.append(token)

class netrc:
    def __init__(self, file=None):
        default_netrc = file is None
        if file is None:
            try:
                file = os.path.join(os.environ['HOME'], ".netrc")
            except KeyError:
                raise OSError("Could not find .netrc: $HOME is not set")
        self.hosts = {}
        self.macros = {}
        with open(file) as fp:
            self._parse(file, fp, default_netrc)

    def _parse(self, file, fp, default_netrc):
        lexer = _netrclex(fp)
        while 1:
            # Look for a machine, default, or macdef top-level keyword
            prev_lineno = lexer.lineno
            tt = lexer.get_token()
            if not tt:
                break
            elif tt[0] == '#':
                if prev_lineno == lexer.lineno:
                    lexer.instream.readline()
                continue
            elif tt == 'machine':
                entryname = lexer.get_token()
            elif tt == 'default':
                entryname = 'default'
            elif tt == 'macdef':
                entryname = lexer.get_token()
                self.macros[entryname] = []
                while 1:
                    line = lexer.instream.readline()
                    if not line:
                        raise NetrcParseError(
                            "Macro definition missing null line terminator.",
                            file, lexer.lineno)
                    if line == '\n':
                        break
                    self.macros[entryname].append(line)
                continue
            else:
                raise NetrcParseError(
                    "bad toplevel token %r" % tt, file, lexer.lineno)

            if not entryname:
                raise NetrcParseError("missing %r name" % tt, file, lexer.lineno)

            # We're looking at start of an entry for a named machine or default.
            login = account = password = ''
            self.hosts[entryname] = {}
            while 1:
                prev_lineno = lexer.lineno
                tt = lexer.get_token()
                if tt.startswith('#'):
                    if lexer.lineno == prev_lineno:
                        lexer.instream.readline()
                    continue
                if tt in {'', 'machine', 'default', 'macdef'}:
                    self.hosts[entryname] = (login, account, password)
                    lexer.push_token(tt)
                    break
                elif tt == 'login' or tt == 'user':
                    login = lexer.get_token()
                elif tt == 'account':
                    account = lexer.get_token()
                elif tt == 'password':
                    if os.name == 'posix' and default_netrc and login != "anonymous":
                        prop = os.fstat(fp.fileno())
                        if prop.st_uid != os.getuid():
                            import pwd
                            try:
                                fowner = pwd.getpwuid(prop.st_uid)[0]
                            except KeyError:
                                fowner = 'uid %s' % prop.st_uid
                            try:
                                user = pwd.getpwuid(os.getuid())[0]
                            except KeyError:
                                user = 'uid %s' % os.getuid()
                            raise NetrcParseError(
                                ("~/.netrc file owner (%s) does not match"
                                 " current user (%s)") % (fowner, user),
                                file, lexer.lineno)
                        if (prop.st_mode & (stat.S_IRWXG | stat.S_IRWXO)):
                            raise NetrcParseError(
                               "~/.netrc access too permissive: access"
                               " permissions must restrict access to only"
                               " the owner", file, lexer.lineno)
                    password = lexer.get_token()
                else:
                    raise NetrcParseError("bad follower token %r" % tt,
                                          file, lexer.lineno)

    def authenticators(self, host):
        """Return a (user, account, password) tuple for given host."""
        if host in self.hosts:
            return self.hosts[host]
        elif 'default' in self.hosts:
            return self.hosts['default']
        else:
            return None

    def __repr__(self):
        """Dump the class data in the format of a .netrc file."""
        rep = ""
        for host in self.hosts.keys():
            attrs = self.hosts[host]
            rep = rep + "machine "+ host + "\n\tlogin " + attrs[0] + "\n"
            if attrs[1]:
                rep = rep + "account " + attrs[1]
            rep = rep + "\tpassword " + attrs[2] + "\n"
        for macro in self.macros.keys():
            rep = rep + "macdef " + macro + "\n"
            for line in self.macros[macro]:
                rep = rep + line
            rep = rep + "\n"
        return rep

if __name__ == '__main__':
    print(netrc())
