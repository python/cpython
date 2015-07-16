from __future__ import with_statement
import os

def vs9to8(src, dest):
    for name in os.listdir(src):
        path, ext = os.path.splitext(name)
        if ext.lower() not in ('.sln', '.vcproj', '.vsprops'):
            continue

        filename = os.path.normpath(os.path.join(src, name))
        destname = os.path.normpath(os.path.join(dest, name))
        print("%s -> %s" % (filename, destname))

        with open(filename, 'rU') as fin:
            lines = fin.read()
            lines = lines.replace('Version="9,00"', 'Version="8.00"')
            lines = lines.replace('Version="9.00"', 'Version="8.00"')
            lines = lines.replace('Format Version 10.00', 'Format Version 9.00')
            lines = lines.replace('Visual Studio 2008', 'Visual Studio 2005')

            lines = lines.replace('wininst-9.0', 'wininst-8.0')
            lines = lines.replace('..\\', '..\\..\\')
            lines = lines.replace('..\\..\\..\\..\\', '..\\..\\..\\')

            # Bah. VS8.0 does not expand macros in file names.
            # Replace them here.
            lines = lines.replace('$(sqlite3Dir)', '..\\..\\..\\sqlite-3.6.21')
            lines = lines.replace('$(bsddbDir)\\..\\..', '..\\..\\..\\db-4.7.25.0\\build_windows\\..')
            lines = lines.replace('$(bsddbDir)', '..\\..\\..\\db-4.7.25.0\\build_windows')

        with open(destname, 'wb') as fout:
            lines = lines.replace("\n", "\r\n")
            fout.write(lines)

if __name__ == "__main__":
    vs9to8(src=".", dest="../PC/VS8.0")
