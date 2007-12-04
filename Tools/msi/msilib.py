# Microsoft Installer Library
# (C) 2003 Martin v. Loewis

import win32com.client.gencache
import win32com.client
import pythoncom, pywintypes
from win32com.client import constants
import re, string, os, sets, glob, subprocess, sys, _winreg, struct

try:
    basestring
except NameError:
    basestring = (str, unicode)

# Partially taken from Wine
datasizemask=      0x00ff
type_valid=        0x0100
type_localizable=  0x0200

typemask=          0x0c00
type_long=         0x0000
type_short=        0x0400
type_string=       0x0c00
type_binary=       0x0800

type_nullable=     0x1000
type_key=          0x2000
# XXX temporary, localizable?
knownbits = datasizemask | type_valid | type_localizable | \
            typemask | type_nullable | type_key

# Summary Info Property IDs
PID_CODEPAGE=1
PID_TITLE=2
PID_SUBJECT=3
PID_AUTHOR=4
PID_KEYWORDS=5
PID_COMMENTS=6
PID_TEMPLATE=7
PID_LASTAUTHOR=8
PID_REVNUMBER=9
PID_LASTPRINTED=11
PID_CREATE_DTM=12
PID_LASTSAVE_DTM=13
PID_PAGECOUNT=14
PID_WORDCOUNT=15
PID_CHARCOUNT=16
PID_APPNAME=18
PID_SECURITY=19

def reset():
    global _directories
    _directories = sets.Set()

def EnsureMSI():
    win32com.client.gencache.EnsureModule('{000C1092-0000-0000-C000-000000000046}', 1033, 1, 0)

def EnsureMSM():
    try:
        win32com.client.gencache.EnsureModule('{0ADDA82F-2C26-11D2-AD65-00A0C9AF11A6}', 0, 1, 0)
    except pywintypes.com_error:
        win32com.client.gencache.EnsureModule('{0ADDA82F-2C26-11D2-AD65-00A0C9AF11A6}', 0, 2, 0)

_Installer=None
def MakeInstaller():
    global _Installer
    if _Installer is None:
        EnsureMSI()
        _Installer = win32com.client.Dispatch('WindowsInstaller.Installer',
                     resultCLSID='{000C1090-0000-0000-C000-000000000046}')
    return _Installer

_Merge=None
def MakeMerge2():
    global _Merge
    if _Merge is None:
        EnsureMSM()
        _Merge = win32com.client.Dispatch("Msm.Merge2.1")
    return _Merge

class Table:
    def __init__(self, name):
        self.name = name
        self.fields = []

    def add_field(self, index, name, type):
        self.fields.append((index,name,type))

    def sql(self):
        fields = []
        keys = []
        self.fields.sort()
        fields = [None]*len(self.fields)
        for index, name, type in self.fields:
            index -= 1
            unk = type & ~knownbits
            if unk:
                print "%s.%s unknown bits %x" % (self.name, name, unk)
            size = type & datasizemask
            dtype = type & typemask
            if dtype == type_string:
                if size:
                    tname="CHAR(%d)" % size
                else:
                    tname="CHAR"
            elif dtype == type_short:
                assert size==2
                tname = "SHORT"
            elif dtype == type_long:
                assert size==4
                tname="LONG"
            elif dtype == type_binary:
                assert size==0
                tname="OBJECT"
            else:
                tname="unknown"
                print "%s.%sunknown integer type %d" % (self.name, name, size)
            if type & type_nullable:
                flags = ""
            else:
                flags = " NOT NULL"
            if type & type_localizable:
                flags += " LOCALIZABLE"
            fields[index] = "`%s` %s%s" % (name, tname, flags)
            if type & type_key:
                keys.append("`%s`" % name)
        fields = ", ".join(fields)
        keys = ", ".join(keys)
        return "CREATE TABLE %s (%s PRIMARY KEY %s)" % (self.name, fields, keys)

    def create(self, db):
        v = db.OpenView(self.sql())
        v.Execute(None)
        v.Close()

class Binary:
    def __init__(self, fname):
        self.name = fname
    def __repr__(self):
        return 'msilib.Binary(os.path.join(dirname,"%s"))' % self.name

def gen_schema(destpath, schemapath):
    d = MakeInstaller()
    schema = d.OpenDatabase(schemapath,
            win32com.client.constants.msiOpenDatabaseModeReadOnly)

    # XXX ORBER BY
    v=schema.OpenView("SELECT * FROM _Columns")
    curtable=None
    tables = []
    v.Execute(None)
    f = open(destpath, "wt")
    f.write("from msilib import Table\n")
    while 1:
        r=v.Fetch()
        if not r:break
        name=r.StringData(1)
        if curtable != name:
            f.write("\n%s = Table('%s')\n" % (name,name))
            curtable = name
            tables.append(name)
        f.write("%s.add_field(%d,'%s',%d)\n" %
                (name, r.IntegerData(2), r.StringData(3), r.IntegerData(4)))
    v.Close()

    f.write("\ntables=[%s]\n\n" % (", ".join(tables)))

    # Fill the _Validation table
    f.write("_Validation_records = [\n")
    v = schema.OpenView("SELECT * FROM _Validation")
    v.Execute(None)
    while 1:
        r = v.Fetch()
        if not r:break
        # Table, Column, Nullable
        f.write("(%s,%s,%s," %
                (`r.StringData(1)`, `r.StringData(2)`, `r.StringData(3)`))
        def put_int(i):
            if r.IsNull(i):f.write("None, ")
            else:f.write("%d," % r.IntegerData(i))
        def put_str(i):
            if r.IsNull(i):f.write("None, ")
            else:f.write("%s," % `r.StringData(i)`)
        put_int(4) # MinValue
        put_int(5) # MaxValue
        put_str(6) # KeyTable
        put_int(7) # KeyColumn
        put_str(8) # Category
        put_str(9) # Set
        put_str(10)# Description
        f.write("),\n")
    f.write("]\n\n")

    f.close()

def gen_sequence(destpath, msipath):
    dir = os.path.dirname(destpath)
    d = MakeInstaller()
    seqmsi = d.OpenDatabase(msipath,
            win32com.client.constants.msiOpenDatabaseModeReadOnly)

    v = seqmsi.OpenView("SELECT * FROM _Tables");
    v.Execute(None)
    f = open(destpath, "w")
    print >>f, "import msilib,os;dirname=os.path.dirname(__file__)"
    tables = []
    while 1:
        r = v.Fetch()
        if not r:break
        table = r.StringData(1)
        tables.append(table)
        f.write("%s = [\n" % table)
        v1 = seqmsi.OpenView("SELECT * FROM `%s`" % table)
        v1.Execute(None)
        info = v1.ColumnInfo(constants.msiColumnInfoTypes)
        while 1:
            r = v1.Fetch()
            if not r:break
            rec = []
            for i in range(1,r.FieldCount+1):
                if r.IsNull(i):
                    rec.append(None)
                elif info.StringData(i)[0] in "iI":
                    rec.append(r.IntegerData(i))
                elif info.StringData(i)[0] in "slSL":
                    rec.append(r.StringData(i))
                elif info.StringData(i)[0]=="v":
                    size = r.DataSize(i)
                    bytes = r.ReadStream(i, size, constants.msiReadStreamBytes)
                    bytes = bytes.encode("latin-1") # binary data represented "as-is"
                    if table == "Binary":
                        fname = rec[0]+".bin"
                        open(os.path.join(dir,fname),"wb").write(bytes)
                        rec.append(Binary(fname))
                    else:
                        rec.append(bytes)
                else:
                    raise "Unsupported column type", info.StringData(i)
            f.write(repr(tuple(rec))+",\n")
        v1.Close()
        f.write("]\n\n")
    v.Close()
    f.write("tables=%s\n" % repr(map(str,tables)))
    f.close()

class _Unspecified:pass
def change_sequence(seq, action, seqno=_Unspecified, cond = _Unspecified):
    "Change the sequence number of an action in a sequence list"
    for i in range(len(seq)):
        if seq[i][0] == action:
            if cond is _Unspecified:
                cond = seq[i][1]
            if seqno is _Unspecified:
                seqno = seq[i][2]
            seq[i] = (action, cond, seqno)
            return
    raise ValueError, "Action not found in sequence"

def add_data(db, table, values):
    d = MakeInstaller()
    v = db.OpenView("SELECT * FROM `%s`" % table)
    count = v.ColumnInfo(0).FieldCount
    r = d.CreateRecord(count)
    for value in values:
        assert len(value) == count, value
        for i in range(count):
            field = value[i]
            if isinstance(field, (int, long)):
                r.SetIntegerData(i+1,field)
            elif isinstance(field, basestring):
                r.SetStringData(i+1,field)
            elif field is None:
                pass
            elif isinstance(field, Binary):
                r.SetStream(i+1, field.name)
            else:
                raise TypeError, "Unsupported type %s" % field.__class__.__name__
        v.Modify(win32com.client.constants.msiViewModifyInsert, r)
        r.ClearData()
    v.Close()

def add_stream(db, name, path):
    d = MakeInstaller()
    v = db.OpenView("INSERT INTO _Streams (Name, Data) VALUES ('%s', ?)" % name)
    r = d.CreateRecord(1)
    r.SetStream(1, path)
    v.Execute(r)
    v.Close()

def init_database(name, schema,
                  ProductName, ProductCode, ProductVersion,
                  Manufacturer):
    try:
        os.unlink(name)
    except OSError:
        pass
    ProductCode = ProductCode.upper()
    d = MakeInstaller()
    # Create the database
    db = d.OpenDatabase(name,
         win32com.client.constants.msiOpenDatabaseModeCreate)
    # Create the tables
    for t in schema.tables:
        t.create(db)
    # Fill the validation table
    add_data(db, "_Validation", schema._Validation_records)
    # Initialize the summary information, allowing atmost 20 properties
    si = db.GetSummaryInformation(20)
    si.SetProperty(PID_TITLE, "Installation Database")
    si.SetProperty(PID_SUBJECT, ProductName)
    si.SetProperty(PID_AUTHOR, Manufacturer)
    si.SetProperty(PID_TEMPLATE, msi_type)
    si.SetProperty(PID_REVNUMBER, gen_uuid())
    si.SetProperty(PID_WORDCOUNT, 2) # long file names, compressed, original media
    si.SetProperty(PID_PAGECOUNT, 200)
    si.SetProperty(PID_APPNAME, "Python MSI Library")
    # XXX more properties
    si.Persist()
    add_data(db, "Property", [
        ("ProductName", ProductName),
        ("ProductCode", ProductCode),
        ("ProductVersion", ProductVersion),
        ("Manufacturer", Manufacturer),
        ("ProductLanguage", "1033")])
    db.Commit()
    return db

def add_tables(db, module):
    for table in module.tables:
        add_data(db, table, getattr(module, table))

def make_id(str):
    #str = str.replace(".", "_") # colons are allowed
    str = str.replace(" ", "_")
    str = str.replace("-", "_")
    if str[0] in string.digits:
        str = "_"+str
    assert re.match("^[A-Za-z_][A-Za-z0-9_.]*$", str), "FILE"+str
    return str

def gen_uuid():
    return str(pythoncom.CreateGuid())

class CAB:
    def __init__(self, name):
        self.name = name
        self.file = open(name+".txt", "wt")
        self.filenames = sets.Set()
        self.index = 0

    def gen_id(self, dir, file):
        logical = _logical = make_id(file)
        pos = 1
        while logical in self.filenames:
            logical = "%s.%d" % (_logical, pos)
            pos += 1
        self.filenames.add(logical)
        return logical

    def append(self, full, file, logical = None):
        if os.path.isdir(full):
            return
        if not logical:
            logical = self.gen_id(dir, file)
        self.index += 1
        if full.find(" ")!=-1:
            print >>self.file, '"%s" %s' % (full, logical)
        else:
            print >>self.file, '%s %s' % (full, logical)
        return self.index, logical

    def commit(self, db):
        self.file.close()
        try:
            os.unlink(self.name+".cab")
        except OSError:
            pass
        for k, v in [(r"Software\Microsoft\VisualStudio\7.1\Setup\VS", "VS7CommonBinDir"),
                     (r"Software\Microsoft\VisualStudio\8.0\Setup\VS", "VS7CommonBinDir"),
                     (r"Software\Microsoft\VisualStudio\9.0\Setup\VS", "VS7CommonBinDir"),
                     (r"Software\Microsoft\Win32SDK\Directories", "Install Dir"),
                    ]:
            try:
                key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, k)
                dir = _winreg.QueryValueEx(key, v)[0]
                _winreg.CloseKey(key)
            except (WindowsError, IndexError):
                continue
            cabarc = os.path.join(dir, r"Bin", "cabarc.exe")
            if not os.path.exists(cabarc):
                continue
            break
        else:
            print "WARNING: cabarc.exe not found in registry"
            cabarc = "cabarc.exe"
        cmd = r'"%s" -m lzx:21 n %s.cab @%s.txt' % (cabarc, self.name, self.name)
        p = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for line in p.stdout:
            if line.startswith("  -- adding "):
                sys.stdout.write(".")
            else:
                sys.stdout.write(line)
            sys.stdout.flush()
        if not os.path.exists(self.name+".cab"):
            raise IOError, "cabarc failed"
        add_data(db, "Media",
                [(1, self.index, None, "#"+self.name, None, None)])
        add_stream(db, self.name, self.name+".cab")
        os.unlink(self.name+".txt")
        os.unlink(self.name+".cab")
        db.Commit()

_directories = sets.Set()
class Directory:
    def __init__(self, db, cab, basedir, physical, _logical, default, componentflags=None):
        """Create a new directory in the Directory table. There is a current component
        at each point in time for the directory, which is either explicitly created
        through start_component, or implicitly when files are added for the first
        time. Files are added into the current component, and into the cab file.
        To create a directory, a base directory object needs to be specified (can be
        None), the path to the physical directory, and a logical directory name.
        Default specifies the DefaultDir slot in the directory table. componentflags
        specifies the default flags that new components get."""
        index = 1
        _logical = make_id(_logical)
        logical = _logical
        while logical in _directories:
            logical = "%s%d" % (_logical, index)
            index += 1
        _directories.add(logical)
        self.db = db
        self.cab = cab
        self.basedir = basedir
        self.physical = physical
        self.logical = logical
        self.component = None
        self.short_names = sets.Set()
        self.ids = sets.Set()
        self.keyfiles = {}
        self.componentflags = componentflags
        if basedir:
            self.absolute = os.path.join(basedir.absolute, physical)
            blogical = basedir.logical
        else:
            self.absolute = physical
            blogical = None
        add_data(db, "Directory", [(logical, blogical, default)])

    def start_component(self, component = None, feature = None, flags = None, keyfile = None, uuid=None):
        """Add an entry to the Component table, and make this component the current for this
        directory. If no component name is given, the directory name is used. If no feature
        is given, the current feature is used. If no flags are given, the directory's default
        flags are used. If no keyfile is given, the KeyPath is left null in the Component
        table."""
        if flags is None:
            flags = self.componentflags
        if uuid is None:
            uuid = gen_uuid()
        else:
            uuid = uuid.upper()
        if component is None:
            component = self.logical
        self.component = component
        if Win64:
            flags |= 256
        if keyfile:
            keyid = self.cab.gen_id(self.absolute, keyfile)
            self.keyfiles[keyfile] = keyid
        else:
            keyid = None
        add_data(self.db, "Component",
                        [(component, uuid, self.logical, flags, None, keyid)])
        if feature is None:
            feature = current_feature
        add_data(self.db, "FeatureComponents",
                        [(feature.id, component)])

    def make_short(self, file):
        parts = file.split(".")
        if len(parts)>1:
            suffix = parts[-1].upper()
        else:
            suffix = None
        prefix = parts[0].upper()
        if len(prefix) <= 8 and (not suffix or len(suffix)<=3):
            if suffix:
                file = prefix+"."+suffix
            else:
                file = prefix
            assert file not in self.short_names
        else:
            prefix = prefix[:6]
            if suffix:
                suffix = suffix[:3]
            pos = 1
            while 1:
                if suffix:
                    file = "%s~%d.%s" % (prefix, pos, suffix)
                else:
                    file = "%s~%d" % (prefix, pos)
                if file not in self.short_names: break
                pos += 1
                assert pos < 10000
                if pos in (10, 100, 1000):
                    prefix = prefix[:-1]
        self.short_names.add(file)
        assert not re.search(r'[\?|><:/*"+,;=\[\]]', file) # restrictions on short names
        return file

    def add_file(self, file, src=None, version=None, language=None):
        """Add a file to the current component of the directory, starting a new one
        one if there is no current component. By default, the file name in the source
        and the file table will be identical. If the src file is specified, it is
        interpreted relative to the current directory. Optionally, a version and a
        language can be specified for the entry in the File table."""
        if not self.component:
            self.start_component(self.logical, current_feature)
        if not src:
            # Allow relative paths for file if src is not specified
            src = file
            file = os.path.basename(file)
        absolute = os.path.join(self.absolute, src)
        assert not re.search(r'[\?|><:/*]"', file) # restrictions on long names
        if self.keyfiles.has_key(file):
            logical = self.keyfiles[file]
        else:
            logical = None
        sequence, logical = self.cab.append(absolute, file, logical)
        assert logical not in self.ids
        self.ids.add(logical)
        short = self.make_short(file)
        full = "%s|%s" % (short, file)
        filesize = os.stat(absolute).st_size
        # constants.msidbFileAttributesVital
        # Compressed omitted, since it is the database default
        # could add r/o, system, hidden
        attributes = 512
        add_data(self.db, "File",
                        [(logical, self.component, full, filesize, version,
                         language, attributes, sequence)])
        if not version:
            # Add hash if the file is not versioned
            filehash = MakeInstaller().FileHash(absolute, 0)
            add_data(self.db, "MsiFileHash",
                     [(logical, 0, filehash.IntegerData(1),
                       filehash.IntegerData(2), filehash.IntegerData(3),
                       filehash.IntegerData(4))])
        # Automatically remove .pyc/.pyo files on uninstall (2)
        # XXX: adding so many RemoveFile entries makes installer unbelievably
        # slow. So instead, we have to use wildcard remove entries
        # if file.endswith(".py"):
        #     add_data(self.db, "RemoveFile",
        #              [(logical+"c", self.component, "%sC|%sc" % (short, file),
        #                self.logical, 2),
        #               (logical+"o", self.component, "%sO|%so" % (short, file),
        #                self.logical, 2)])

    def glob(self, pattern, exclude = None):
        """Add a list of files to the current component as specified in the
        glob pattern. Individual files can be excluded in the exclude list."""
        files = glob.glob1(self.absolute, pattern)
        for f in files:
            if exclude and f in exclude: continue
            self.add_file(f)
        return files

    def remove_pyc(self):
        "Remove .pyc/.pyo files on uninstall"
        add_data(self.db, "RemoveFile",
                 [(self.component+"c", self.component, "*.pyc", self.logical, 2),
                  (self.component+"o", self.component, "*.pyo", self.logical, 2)])

class Feature:
    def __init__(self, db, id, title, desc, display, level = 1,
                 parent=None, directory = None, attributes=0):
        self.id = id
        if parent:
            parent = parent.id
        add_data(db, "Feature",
                        [(id, parent, title, desc, display,
                          level, directory, attributes)])
    def set_current(self):
        global current_feature
        current_feature = self

class Control:
    def __init__(self, dlg, name):
        self.dlg = dlg
        self.name = name

    def event(self, ev, arg, cond = "1", order = None):
        add_data(self.dlg.db, "ControlEvent",
                 [(self.dlg.name, self.name, ev, arg, cond, order)])

    def mapping(self, ev, attr):
        add_data(self.dlg.db, "EventMapping",
                 [(self.dlg.name, self.name, ev, attr)])

    def condition(self, action, condition):
        add_data(self.dlg.db, "ControlCondition",
                 [(self.dlg.name, self.name, action, condition)])

class RadioButtonGroup(Control):
    def __init__(self, dlg, name, property):
        self.dlg = dlg
        self.name = name
        self.property = property
        self.index = 1

    def add(self, name, x, y, w, h, text, value = None):
        if value is None:
            value = name
        add_data(self.dlg.db, "RadioButton",
                 [(self.property, self.index, value,
                   x, y, w, h, text, None)])
        self.index += 1

class Dialog:
    def __init__(self, db, name, x, y, w, h, attr, title, first, default, cancel):
        self.db = db
        self.name = name
        self.x, self.y, self.w, self.h = x,y,w,h
        add_data(db, "Dialog", [(name, x,y,w,h,attr,title,first,default,cancel)])

    def control(self, name, type, x, y, w, h, attr, prop, text, next, help):
        add_data(self.db, "Control",
                 [(self.name, name, type, x, y, w, h, attr, prop, text, next, help)])
        return Control(self, name)

    def text(self, name, x, y, w, h, attr, text):
        return self.control(name, "Text", x, y, w, h, attr, None,
                     text, None, None)

    def bitmap(self, name, x, y, w, h, text):
        return self.control(name, "Bitmap", x, y, w, h, 1, None, text, None, None)

    def line(self, name, x, y, w, h):
        return self.control(name, "Line", x, y, w, h, 1, None, None, None, None)

    def pushbutton(self, name, x, y, w, h, attr, text, next):
        return self.control(name, "PushButton", x, y, w, h, attr, None, text, next, None)

    def radiogroup(self, name, x, y, w, h, attr, prop, text, next):
        add_data(self.db, "Control",
                 [(self.name, name, "RadioButtonGroup",
                   x, y, w, h, attr, prop, text, next, None)])
        return RadioButtonGroup(self, name, prop)

    def checkbox(self, name, x, y, w, h, attr, prop, text, next):
        return self.control(name, "CheckBox", x, y, w, h, attr, prop, text, next, None)

def pe_type(path):
    header = open(path, "rb").read(1000)
    # offset of PE header is at offset 0x3c
    pe_offset = struct.unpack("<i", header[0x3c:0x40])[0]
    assert header[pe_offset:pe_offset+4] == "PE\0\0"
    machine = struct.unpack("<H", header[pe_offset+4:pe_offset+6])[0]
    return machine

def set_arch_from_file(path):
    global msi_type, Win64, arch_ext
    machine = pe_type(path)
    if machine == 0x14c:
        # i386
        msi_type = "Intel"
        Win64 = 0
        arch_ext = ''
    elif machine == 0x200:
        # Itanium
        msi_type = "Intel64"
        Win64 = 1
        arch_ext = '.ia64'
    elif machine == 0x8664:
        # AMD64
        msi_type = "x64"
        Win64 = 1
        arch_ext = '.amd64'
    else:
        raise ValueError, "Unsupported architecture"
    msi_type += ";1033"
