#!/usr/bin/env python

# Make a reST file compliant to our pre-commit hook.
# Currently just remove trailing whitespace.


import sys, re, shutil

ws_re = re.compile(br'\s+(\r?\n)$')

def main(argv=sys.argv):
    rv = 0
    for filename in argv[1:]:
        try:
            with open(filename, 'rb') as f:
                lines = f.readlines()
            new_lines = [ws_re.sub(br'\1', line) for line in lines]
            if new_lines != lines:
                print('Fixing %s...' % filename)
            shutil.copyfile(filename, filename + '.bak')
            with open(filename, 'wb') as f:
                f.writelines(new_lines)
        except Exception as err:
            print('Cannot fix %s: %s' % (filename, err))
            rv = 1
    return rv

if __name__ == '__main__':
    sys.exit(main())
