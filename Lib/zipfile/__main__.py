import sys
import os
from . import ZipFile, ZIP_DEFLATED


def main(args=None):
    import argparse

    description = 'A simple command-line interface for zipfile module.'
    parser = argparse.ArgumentParser(description=description)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-l', '--list', metavar='<zipfile>',
                       help='Show listing of a zipfile')
    group.add_argument('-e', '--extract', nargs=2,
                       metavar=('<zipfile>', '<output_dir>'),
                       help='Extract zipfile into target dir')
    group.add_argument('-c', '--create', nargs='+',
                       metavar=('<name>', '<file>'),
                       help='Create zipfile from sources')
    group.add_argument('-t', '--test', metavar='<zipfile>',
                       help='Test if a zipfile is valid')
    parser.add_argument('--metadata-encoding', metavar='<encoding>',
                        help='Specify encoding of member names for -l, -e and -t')
    args = parser.parse_args(args)

    encoding = args.metadata_encoding

    if args.test is not None:
        src = args.test
        with ZipFile(src, 'r', metadata_encoding=encoding) as zf:
            badfile = zf.testzip()
        if badfile:
            print("The following enclosed file is corrupted: {!r}".format(badfile))
        print("Done testing")

    elif args.list is not None:
        src = args.list
        with ZipFile(src, 'r', metadata_encoding=encoding) as zf:
            zf.printdir()

    elif args.extract is not None:
        src, curdir = args.extract
        with ZipFile(src, 'r', metadata_encoding=encoding) as zf:
            zf.extractall(curdir)

    elif args.create is not None:
        if encoding:
            print("Non-conforming encodings not supported with -c.",
                  file=sys.stderr)
            sys.exit(1)

        zip_name = args.create.pop(0)
        files = args.create

        def addToZip(zf, path, zippath):
            if os.path.isfile(path):
                zf.write(path, zippath, ZIP_DEFLATED)
            elif os.path.isdir(path):
                if zippath:
                    zf.write(path, zippath)
                for nm in sorted(os.listdir(path)):
                    addToZip(zf,
                             os.path.join(path, nm), os.path.join(zippath, nm))
            # else: ignore

        with ZipFile(zip_name, 'w') as zf:
            for path in files:
                zippath = os.path.basename(path)
                if not zippath:
                    zippath = os.path.basename(os.path.dirname(path))
                if zippath in ('', os.curdir, os.pardir):
                    zippath = ''
                addToZip(zf, path, zippath)


if __name__ == "__main__":
    main()
