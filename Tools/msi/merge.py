import msilib,os,win32com,tempfile,sys
PCBUILD="PCBuild"
certname = None
from config import *

Win64 = "amd64" in PCBUILD

mod_dir = os.path.join(os.environ["ProgramFiles"], "Common Files", "Merge Modules")
msi = None
if len(sys.argv)==2:
    msi = sys.argv[1]
if Win64:
    modules = ["Microsoft_VC90_CRT_x86_x64.msm", "policy_9_0_Microsoft_VC90_CRT_x86_x64.msm"]
    if not msi: msi = "python-%s.amd64.msi" % full_current_version
else:
    modules = ["Microsoft_VC90_CRT_x86.msm","policy_9_0_Microsoft_VC90_CRT_x86.msm"]
    if not msi: msi = "python-%s.msi" % full_current_version
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
    # The merge module sets ALLUSERS to 1 in the property table. 
    # This is undesired; delete that
    v = db.OpenView("DELETE FROM Property WHERE Property='ALLUSERS'")
    v.Execute(None)
    v.Close()
    db.Commit()

merge(msi, "SharedCRT", "TARGETDIR", modules)

# certname (from config.py) should be (a substring of)
# the certificate subject, e.g. "Python Software Foundation"
if certname:
    os.system('signtool sign /n "%s" /t http://timestamp.verisign.com/scripts/timestamp.dll %s' % (certname, msi))
