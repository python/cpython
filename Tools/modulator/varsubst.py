#
# Variable substitution. Variables are $delimited$
#
import string
import regex
import regsub

error = 'varsubst.error'

class Varsubst:
    def __init__(self, dict):
        self.dict = dict
        self.prog = regex.compile('\$[a-zA-Z0-9_]*\$')
        self.do_useindent = 0

    def useindent(self, onoff):
        self.do_useindent = onoff
        
    def subst(self, str):
        rv = ''
        while 1:
            pos = self.prog.search(str)
            if pos < 0:
                return rv + str
            if pos:
                rv = rv + str[:pos]
                str = str[pos:]
            len = self.prog.match(str)
            if len == 2:
                # Escaped dollar
                rv = rv + '$'
                str = str[2:]
                continue
            name = str[1:len-1]
            str = str[len:]
            if not self.dict.has_key(name):
                raise error, 'No such variable: '+name
            value = self.dict[name]
            if self.do_useindent and '\n' in value:
                value = self._modindent(value, rv)
            rv = rv + value

    def _modindent(self, value, old):
        lastnl = string.rfind(old, '\n', 0) + 1
        lastnl = len(old) - lastnl
        sub = '\n' + (' '*lastnl)
        return regsub.gsub('\n', sub, value)

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
