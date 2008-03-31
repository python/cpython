# A wrapper around the (optional) built-in class dbm, supporting keys
# and values of almost any type instead of just string.
# (Actually, this works only for keys and values that can be read back
# correctly after being converted to a string.)


class Dbm:

    def __init__(self, filename, mode, perm):
        import dbm
        self.db = dbm.open(filename, mode, perm)

    def __repr__(self):
        s = ''
        for key in self.keys():
            t = repr(key) + ': ' + repr(self[key])
            if s: t = ', ' + t
            s = s + t
        return '{' + s + '}'

    def __len__(self):
        return len(self.db)

    def __getitem__(self, key):
        return eval(self.db[repr(key)])

    def __setitem__(self, key, value):
        self.db[repr(key)] = repr(value)

    def __delitem__(self, key):
        del self.db[repr(key)]

    def keys(self):
        res = []
        for key in self.db.keys():
            res.append(eval(key))
        return res

    def has_key(self, key):
        return repr(key) in self.db


def test():
    d = Dbm('@dbm', 'rw', 0o600)
    print(d)
    while 1:
        try:
            key = eval(input('key: '))
            if key in d:
                value = d[key]
                print('currently:', value)
            value = eval(input('value: '))
            if value is None:
                del d[key]
            else:
                d[key] = value
        except KeyboardInterrupt:
            print('')
            print(d)
        except EOFError:
            print('[eof]')
            break
    print(d)


test()
