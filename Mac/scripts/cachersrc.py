# Scan the tree passed as argv[0] for .rsrc files, skipping .rsrc.df.rsrc
# files, and open these. The effect of this is to create the .rsrc.df.rsrc
# cache files if needed.
# These are needed on OSX: the .rsrc files are in reality AppleSingle-encoded
# files. We decode the resources into a datafork-based resource file.

import macresource
import os
import sys
import getopt

class NoArgsError(Exception):
    pass

def handler((verbose, force), dirname, fnames):
    for fn in fnames:
        if fn[-5:] == '.rsrc' and fn[-13:] != '.rsrc.df.rsrc':
            if force:
                try:
                    os.unlink(os.path.join(dirname, fn + '.df.rsrc'))
                except IOError:
                    pass
            macresource.open_pathname(os.path.join(dirname, fn), verbose=verbose)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'vf')
        if not args:
            raise NoArgsError
    except (getopt.GetoptError, NoArgsError):
        sys.stderr.write('Usage: cachersrc.py dirname ...\n')
        sys.exit(1)
    verbose = 0
    force = 0
    for o, v in opts:
        if o == '-v':
            verbose = 1
        if o == '-f':
            force = 1
    for dir in sys.argv[1:]:
        os.path.walk(dir, handler, (verbose, force))

if __name__ == '__main__':
    main()
