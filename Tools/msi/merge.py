import msilib,os,win32com,tempfile
PCBUILD="PCBuild"
from config import *

Win64 = "amd64" in PCBUILD

mod_dir = os.path.join(os.environ["ProgramFiles"], "Common Files", "Merge Modules")
if Win64:
    modules = ["Microsoft_VC90_CRT_x86.msm", "policy_8_0_Microsoft_VC80_CRT_x86_x64.msm"]
    msi = "python-%s.amd64.msi" % full_current_version
else:
    modules = ["Microsoft_VC90_CRT_x86.msm","policy_8_0_Microsoft_VC80_CRT_x86.msm"]
    msi = "python-%s.msi" % full_current_version
for i, n in enumerate(modules):
    modules[i] = os.path.join(mod_dir, n)

def merge(msi, feature, rootdir, modules):
    cab_and_filecount = []
    # Step 1: Merge databases, extract cabfiles
    m = msilib.MakeMerge2()
    m.OpenLog("merge.log")
    print "Opened Log"
    m.OpenDatabase(msi)
    print "Opened DB"
    for module in modules:
        print module
        m.OpenModule(module,0)
        print "Opened Module",module
        m.Merge(feature, rootdir)
        print "Errors:"
        for e in m.Errors:
            print e.Type, e.ModuleTable, e.DatabaseTable
            print "   Modkeys:",
            for s in e.ModuleKeys: print s,
            print
            print "   DBKeys:",
            for s in e.DatabaseKeys: print s,
            print
        cabname = tempfile.mktemp(suffix=".cab")
        m.ExtractCAB(cabname)
        cab_and_filecount.append((cabname, len(m.ModuleFiles)))
        m.CloseModule()
    m.CloseDatabase(True)
    m.CloseLog()

    # Step 2: Add CAB files
    i = msilib.MakeInstaller()
    db = i.OpenDatabase(msi, win32com.client.constants.msiOpenDatabaseModeTransact)

    v = db.OpenView("SELECT LastSequence FROM Media")
    v.Execute(None)
    maxmedia = -1
    while 1:
        r = v.Fetch()
        if not r: break
        seq = r.IntegerData(1)
        if seq > maxmedia:
            maxmedia = seq
    print "Start of Media", maxmedia

    for cabname, count in cab_and_filecount:
        stream = "merged%d" % maxmedia
        msilib.add_data(db, "Media",
                [(maxmedia+1, maxmedia+count, None, "#"+stream, None, None)])
        msilib.add_stream(db, stream,  cabname)
        os.unlink(cabname)
        maxmedia += count
    db.Commit()

merge(msi, "SharedCRT", "TARGETDIR", modules)
