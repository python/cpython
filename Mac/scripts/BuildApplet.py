"""Create an applet from a Python script.

This puts up a dialog asking for a Python source file ('TEXT').
The output is a file with the same name but its ".py" suffix dropped.
It is created by copying an applet template and then adding a 'PYC '
resource named __main__ containing the compiled, marshalled script.
"""


import sys
sys.stdout = sys.stderr

import os
import MacOS
import EasyDialogs
import buildtools
import getopt

def main():
    try:
        buildapplet()
    except buildtools.BuildError, detail:
        EasyDialogs.Message(detail)


def buildapplet():
    buildtools.DEBUG=1

    # Find the template
    # (there's no point in proceeding if we can't find it)

    template = buildtools.findtemplate()

    # Ask for source text if not specified in sys.argv[1:]

    if not sys.argv[1:]:
        filename = EasyDialogs.AskFileForOpen(message='Select Python source or applet:',
                typeList=('TEXT', 'APPL'))
        if not filename:
            return
        tp, tf = os.path.split(filename)
        if tf[-3:] == '.py':
            tf = tf[:-3]
        else:
            tf = tf + '.applet'
        dstfilename = EasyDialogs.AskFileForSave(message='Save application as:',
                savedFileName=tf)
        if not dstfilename: return
        cr, tp = MacOS.GetCreatorAndType(filename)
        if tp == 'APPL':
            buildtools.update(template, filename, dstfilename)
        else:
            buildtools.process(template, filename, dstfilename, 1)
    else:

        SHORTOPTS = "o:r:ne:v?PR"
        LONGOPTS=("output=", "resource=", "noargv", "extra=", "verbose", "help", "python=", "destroot=")
        try:
            options, args = getopt.getopt(sys.argv[1:], SHORTOPTS, LONGOPTS)
        except getopt.error:
            usage()
        if options and len(args) > 1:
            sys.stderr.write("Cannot use options when specifying multiple input files")
            sys.exit(1)
        dstfilename = None
        rsrcfilename = None
        raw = 0
        extras = []
        verbose = None
        destroot = ''
        for opt, arg in options:
            if opt in ('-o', '--output'):
                dstfilename = arg
            elif opt in ('-r', '--resource'):
                rsrcfilename = arg
            elif opt in ('-n', '--noargv'):
                raw = 1
            elif opt in ('-e', '--extra'):
                if ':' in arg:
                    arg = arg.split(':')
                extras.append(arg)
            elif opt in ('-P', '--python'):
                # This is a very dirty trick. We set sys.executable
                # so that bundlebuilder will use this in the #! line
                # for the applet bootstrap.
                sys.executable = arg
            elif opt in ('-v', '--verbose'):
                verbose = Verbose()
            elif opt in ('-?', '--help'):
                usage()
            elif opt in ('-d', '--destroot'):
                destroot = arg
        # On OS9 always be verbose
        if sys.platform == 'mac' and not verbose:
            verbose = 'default'
        # Loop over all files to be processed
        for filename in args:
            cr, tp = MacOS.GetCreatorAndType(filename)
            if tp == 'APPL':
                buildtools.update(template, filename, dstfilename)
            else:
                buildtools.process(template, filename, dstfilename, 1,
                        rsrcname=rsrcfilename, others=extras, raw=raw,
                        progress=verbose, destroot=destroot)

def usage():
    print "BuildApplet creates an application from a Python source file"
    print "Usage:"
    print "  BuildApplet     interactive, single file, no options"
    print "  BuildApplet src1.py src2.py ...   non-interactive multiple file"
    print "  BuildApplet [options] src.py    non-interactive single file"
    print "Options:"
    print "  --output o        Output file; default based on source filename, short -o"
    print "  --resource r      Resource file; default based on source filename, short -r"
    print "  --noargv          Build applet without drag-and-drop sys.argv emulation, short -n, OSX only"
    print "  --extra src[:dst] Extra file to put in .app bundle, short -e, OSX only"
    print "  --verbose         Verbose, short -v"
    print "  --help            This message, short -?"
    sys.exit(1)

class Verbose:
    """This class mimics EasyDialogs.ProgressBar but prints to stderr"""
    def __init__(self, *args):
        if args and args[0]:
            self.label(args[0])

    def set(self, *args):
        pass

    def inc(self, *args):
        pass

    def label(self, str):
        sys.stderr.write(str+'\n')

if __name__ == '__main__':
    main()
