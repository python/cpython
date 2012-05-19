#Run this file after automatic convertsion of the VisualStudio 2008 solution by VisualStudio 2010.
#This can be done whenever the 2008 solution changes.
#It will make the necessary cleanup and updates to the vcxproj files
#the .props files need to be maintained by hand if the .vsprops files change

from __future__ import with_statement
import sys
import os
import os.path

def vs9to10(src, dest):
    for name in os.listdir(src):
        path, ext = os.path.splitext(name)
        if ext.lower() not in ('.vcxproj',):
            continue

        filename = os.path.normpath(os.path.join(src, name))
        destname = os.path.normpath(os.path.join(dest, name))
        print("%s -> %s" % (filename, destname))

        lines = []
        lastline = b""
        importgroup = False
        with open(filename, 'rb') as fin:
            for line in fin:
                #remove redundant linker output info
                if b"<OutputLine>" in line:
                    continue
                if b"<ProgramDatabaseFile>" in line:
                    continue
                if b"<ImportLibrary>" in line and b"</ImportLibrary>" in line:
                    continue

                #add new property sheet to the pythoncore
                if importgroup and "pythoncore" in name.lower():
                    if b"</ImportGroup>" in line:
                        if b"debug.props" in lastline:
                            lines.append(b'    <Import Project="pythoncore_d.props" />\r\n')
                        elif b"pythoncore" not in lastline:
                            lines.append(b'    <Import Project="pythoncore.props" />\r\n')
                if b"<ImportGroup Condition" in line:
                    importgroup = True
                elif b"</ImportGroup>" in line:
                    importgroup = False
                lines.append(line)
                lastline = line
        with open(destname, 'wb') as fout:
            for line in lines:
                fout.write(line)

if __name__ == "__main__":
    src = "." if len(sys.argv) < 2 else sys.argv[1]
    name = os.path.basename(os.path.abspath(src))
    dest = os.path.abspath(os.path.join(src, "..", name + "Upd"))
    os.makedirs(dest)
    vs9to10(src, dest)
