#
# Variable substitution. Variables are $delimited$
#
import re

error = 'varsubst.error'

class Varsubst:
    def __init__(self, dict):
        self.dict = dict
        self.prog = re.compile('\$([a-zA-Z0-9_]*)\$')
        self.do_useindent = 0

    def useindent(self, onoff):
        self.do_useindent = onoff

    def subst(self, s):
        rv = ''
        while 1:
            m = self.prog.search(s)
            if not m:
                return rv + s
            rv = rv + s[:m.start()]
            s = s[m.end():]
            if m.end() - m.start() == 2:
                # Escaped dollar
                rv = rv + '$'
                s = s[2:]
                continue
            name = m.group(1)
            if not self.dict.has_key(name):
                raise error, 'No such variable: '+name
            value = self.dict[name]
            if self.do_useindent and '\n' in value:
                value = self._modindent(value, rv)
            rv = rv + value

    def _modindent(self, value, old):
        lastnl = old.rfind('\n', 0) + 1
        lastnl = len(old) - lastnl
        sub = '\n' + (' '*lastnl)
        return re.sub('\n', sub, value)

def _test():
    import sys
    import os

    sys.stderr.write('-- Copying stdin to stdout with environment map --\n')
    c = Varsubst(os.environ)
    c.useindent(1)
    d = sys.stdin.read()
    sys.stdout.write(c.subst(d))
    sys.exit(1)

if __name__ == '__main__':
    _test()
