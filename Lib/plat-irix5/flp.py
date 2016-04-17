#
# flp - Module to load fl forms from fd files
#
# Jack Jansen, December 1991
#
from warnings import warnpy3k
warnpy3k("the flp module has been removed in Python 3.0", stacklevel=2)
del warnpy3k

import string
import os
import sys
import FL

SPLITLINE = '--------------------'
FORMLINE = '=============== FORM ==============='
ENDLINE = '=============================='

class error(Exception):
    pass

##################################################################
#    Part 1 - The parsing routines                               #
##################################################################

#
# Externally visible function. Load form.
#
def parse_form(filename, formname):
    forms = checkcache(filename)
    if forms is None:
        forms = parse_forms(filename)
    if forms.has_key(formname):
        return forms[formname]
    else:
        raise error, 'No such form in fd file'

#
# Externally visible function. Load all forms.
#
def parse_forms(filename):
    forms = checkcache(filename)
    if forms is not None: return forms
    fp = _open_formfile(filename)
    nforms = _parse_fd_header(fp)
    forms = {}
    for i in range(nforms):
        form = _parse_fd_form(fp, None)
        forms[form[0].Name] = form
    writecache(filename, forms)
    return forms

#
# Internal: see if a cached version of the file exists
#
MAGIC = '.fdc'
_internal_cache = {}                    # Used by frozen scripts only
def checkcache(filename):
    if _internal_cache.has_key(filename):
        altforms = _internal_cache[filename]
        return _unpack_cache(altforms)
    import marshal
    fp, filename = _open_formfile2(filename)
    fp.close()
    cachename = filename + 'c'
    try:
        fp = open(cachename, 'r')
    except IOError:
        #print 'flp: no cache file', cachename
        return None
    try:
        if fp.read(4) != MAGIC:
            print 'flp: bad magic word in cache file', cachename
            return None
        cache_mtime = rdlong(fp)
        file_mtime = getmtime(filename)
        if cache_mtime != file_mtime:
            #print 'flp: outdated cache file', cachename
            return None
        #print 'flp: valid cache file', cachename
        altforms = marshal.load(fp)
        return _unpack_cache(altforms)
    finally:
        fp.close()

def _unpack_cache(altforms):
    forms = {}
    for name in altforms.keys():
        altobj, altlist = altforms[name]
        obj = _newobj()
        obj.make(altobj)
        list = []
        for altobj in altlist:
            nobj = _newobj()
            nobj.make(altobj)
            list.append(nobj)
        forms[name] = obj, list
    return forms

def rdlong(fp):
    s = fp.read(4)
    if len(s) != 4: return None
    a, b, c, d = s[0], s[1], s[2], s[3]
    return ord(a)<<24 | ord(b)<<16 | ord(c)<<8 | ord(d)

def wrlong(fp, x):
    a, b, c, d = (x>>24)&0xff, (x>>16)&0xff, (x>>8)&0xff, x&0xff
    fp.write(chr(a) + chr(b) + chr(c) + chr(d))

def getmtime(filename):
    import os
    from stat import ST_MTIME
    try:
        return os.stat(filename)[ST_MTIME]
    except os.error:
        return None

#
# Internal: write cached version of the form (parsing is too slow!)
#
def writecache(filename, forms):
    import marshal
    fp, filename = _open_formfile2(filename)
    fp.close()
    cachename = filename + 'c'
    try:
        fp = open(cachename, 'w')
    except IOError:
        print 'flp: can\'t create cache file', cachename
        return # Never mind
    fp.write('\0\0\0\0') # Seek back and write MAGIC when done
    wrlong(fp, getmtime(filename))
    altforms = _pack_cache(forms)
    marshal.dump(altforms, fp)
    fp.seek(0)
    fp.write(MAGIC)
    fp.close()
    #print 'flp: wrote cache file', cachename

#
# External: print some statements that set up the internal cache.
# This is for use with the "freeze" script.  You should call
# flp.freeze(filename) for all forms used by the script, and collect
# the output on a file in a module file named "frozenforms.py".  Then
# in the main program of the script import frozenforms.
# (Don't forget to take this out when using the unfrozen version of
# the script!)
#
def freeze(filename):
    forms = parse_forms(filename)
    altforms = _pack_cache(forms)
    print 'import flp'
    print 'flp._internal_cache[', repr(filename), '] =', altforms

#
# Internal: create the data structure to be placed in the cache
#
def _pack_cache(forms):
    altforms = {}
    for name in forms.keys():
        obj, list = forms[name]
        altobj = obj.__dict__
        altlist = []
        for obj in list: altlist.append(obj.__dict__)
        altforms[name] = altobj, altlist
    return altforms

#
# Internal: Locate form file (using PYTHONPATH) and open file
#
def _open_formfile(filename):
    return _open_formfile2(filename)[0]

def _open_formfile2(filename):
    if filename[-3:] != '.fd':
        filename = filename + '.fd'
    if filename[0] == '/':
        try:
            fp = open(filename,'r')
        except IOError:
            fp = None
    else:
        for pc in sys.path:
            pn = os.path.join(pc, filename)
            try:
                fp = open(pn, 'r')
                filename = pn
                break
            except IOError:
                fp = None
    if fp is None:
        raise error, 'Cannot find forms file ' + filename
    return fp, filename

#
# Internal: parse the fd file header, return number of forms
#
def _parse_fd_header(file):
    # First read the magic header line
    datum = _parse_1_line(file)
    if datum != ('Magic', 12321):
        raise error, 'Not a forms definition file'
    # Now skip until we know number of forms
    while 1:
        datum = _parse_1_line(file)
        if type(datum) == type(()) and datum[0] == 'Numberofforms':
            break
    return datum[1]
#
# Internal: parse fd form, or skip if name doesn't match.
# the special value None means 'always parse it'.
#
def _parse_fd_form(file, name):
    datum = _parse_1_line(file)
    if datum != FORMLINE:
        raise error, 'Missing === FORM === line'
    form = _parse_object(file)
    if form.Name == name or name is None:
        objs = []
        for j in range(form.Numberofobjects):
            obj = _parse_object(file)
            objs.append(obj)
        return (form, objs)
    else:
        for j in range(form.Numberofobjects):
            _skip_object(file)
    return None

#
# Internal class: a convenient place to store object info fields
#
class _newobj:
    def add(self, name, value):
        self.__dict__[name] = value
    def make(self, dict):
        for name in dict.keys():
            self.add(name, dict[name])

#
# Internal parsing routines.
#
def _parse_string(str):
    if '\\' in str:
        s = '\'' + str + '\''
        try:
            return eval(s)
        except:
            pass
    return str

def _parse_num(str):
    return eval(str)

def _parse_numlist(str):
    slist = string.split(str)
    nlist = []
    for i in slist:
        nlist.append(_parse_num(i))
    return nlist

# This dictionary maps item names to parsing routines.
# If no routine is given '_parse_num' is default.
_parse_func = { \
        'Name':         _parse_string, \
        'Box':          _parse_numlist, \
        'Colors':       _parse_numlist, \
        'Label':        _parse_string, \
        'Name':         _parse_string, \
        'Callback':     _parse_string, \
        'Argument':     _parse_string }

# This function parses a line, and returns either
# a string or a tuple (name,value)

import re
prog = re.compile('^([^:]*): *(.*)')

def _parse_line(line):
    match = prog.match(line)
    if not match:
        return line
    name, value = match.group(1, 2)
    if name[0] == 'N':
        name = string.join(string.split(name),'')
        name = string.lower(name)
    name = string.capitalize(name)
    try:
        pf = _parse_func[name]
    except KeyError:
        pf = _parse_num
    value = pf(value)
    return (name, value)

def _readline(file):
    line = file.readline()
    if not line:
        raise EOFError
    return line[:-1]

def _parse_1_line(file):
    line = _readline(file)
    while line == '':
        line = _readline(file)
    return _parse_line(line)

def _skip_object(file):
    line = ''
    while not line in (SPLITLINE, FORMLINE, ENDLINE):
        pos = file.tell()
        line = _readline(file)
    if line == FORMLINE:
        file.seek(pos)

def _parse_object(file):
    obj = _newobj()
    while 1:
        pos = file.tell()
        datum = _parse_1_line(file)
        if datum in (SPLITLINE, FORMLINE, ENDLINE):
            if datum == FORMLINE:
                file.seek(pos)
            return obj
        if type(datum) is not type(()) or len(datum) != 2:
            raise error, 'Parse error, illegal line in object: '+datum
        obj.add(datum[0], datum[1])

#################################################################
#   Part 2 - High-level object/form creation routines            #
#################################################################

#
# External - Create a form and link to an instance variable.
#
def create_full_form(inst, (fdata, odatalist)):
    form = create_form(fdata)
    exec 'inst.'+fdata.Name+' = form\n'
    for odata in odatalist:
        create_object_instance(inst, form, odata)

#
# External - Merge a form into an existing form in an instance
# variable.
#
def merge_full_form(inst, form, (fdata, odatalist)):
    exec 'inst.'+fdata.Name+' = form\n'
    if odatalist[0].Class != FL.BOX:
        raise error, 'merge_full_form() expects FL.BOX as first obj'
    for odata in odatalist[1:]:
        create_object_instance(inst, form, odata)


#################################################################
#   Part 3 - Low-level object/form creation routines            #
#################################################################

#
# External Create_form - Create form from parameters
#
def create_form(fdata):
    import fl
    return fl.make_form(FL.NO_BOX, fdata.Width, fdata.Height)

#
# External create_object - Create an object. Make sure there are
# no callbacks. Returns the object created.
#
def create_object(form, odata):
    obj = _create_object(form, odata)
    if odata.Callback:
        raise error, 'Creating free object with callback'
    return obj
#
# External create_object_instance - Create object in an instance.
#
def create_object_instance(inst, form, odata):
    obj = _create_object(form, odata)
    if odata.Callback:
        cbfunc = eval('inst.'+odata.Callback)
        obj.set_call_back(cbfunc, odata.Argument)
    if odata.Name:
        exec 'inst.' + odata.Name + ' = obj\n'
#
# Internal _create_object: Create the object and fill options
#
def _create_object(form, odata):
    crfunc = _select_crfunc(form, odata.Class)
    obj = crfunc(odata.Type, odata.Box[0], odata.Box[1], odata.Box[2], \
            odata.Box[3], odata.Label)
    if not odata.Class in (FL.BEGIN_GROUP, FL.END_GROUP):
        obj.boxtype = odata.Boxtype
        obj.col1 = odata.Colors[0]
        obj.col2 = odata.Colors[1]
        obj.align = odata.Alignment
        obj.lstyle = odata.Style
        obj.lsize = odata.Size
        obj.lcol = odata.Lcol
    return obj
#
# Internal crfunc: helper function that returns correct create function
#
def _select_crfunc(fm, cl):
    if cl == FL.BEGIN_GROUP: return fm.bgn_group
    elif cl == FL.END_GROUP: return fm.end_group
    elif cl == FL.BITMAP: return fm.add_bitmap
    elif cl == FL.BOX: return fm.add_box
    elif cl == FL.BROWSER: return fm.add_browser
    elif cl == FL.BUTTON: return fm.add_button
    elif cl == FL.CHART: return fm.add_chart
    elif cl == FL.CHOICE: return fm.add_choice
    elif cl == FL.CLOCK: return fm.add_clock
    elif cl == FL.COUNTER: return fm.add_counter
    elif cl == FL.DIAL: return fm.add_dial
    elif cl == FL.FREE: return fm.add_free
    elif cl == FL.INPUT: return fm.add_input
    elif cl == FL.LIGHTBUTTON: return fm.add_lightbutton
    elif cl == FL.MENU: return fm.add_menu
    elif cl == FL.POSITIONER: return fm.add_positioner
    elif cl == FL.ROUNDBUTTON: return fm.add_roundbutton
    elif cl == FL.SLIDER: return fm.add_slider
    elif cl == FL.VALSLIDER: return fm.add_valslider
    elif cl == FL.TEXT: return fm.add_text
    elif cl == FL.TIMER: return fm.add_timer
    else:
        raise error, 'Unknown object type: %r' % (cl,)


def test():
    import time
    t0 = time.time()
    if len(sys.argv) == 2:
        forms = parse_forms(sys.argv[1])
        t1 = time.time()
        print 'parse time:', 0.001*(t1-t0), 'sec.'
        keys = forms.keys()
        keys.sort()
        for i in keys:
            _printform(forms[i])
    elif len(sys.argv) == 3:
        form = parse_form(sys.argv[1], sys.argv[2])
        t1 = time.time()
        print 'parse time:', round(t1-t0, 3), 'sec.'
        _printform(form)
    else:
        print 'Usage: test fdfile [form]'

def _printform(form):
    f = form[0]
    objs = form[1]
    print 'Form ', f.Name, ', size: ', f.Width, f.Height, ' Nobj ', f.Numberofobjects
    for i in objs:
        print '  Obj ', i.Name, ' type ', i.Class, i.Type
        print '    Box ', i.Box, ' btype ', i.Boxtype
        print '    Label ', i.Label, ' size/style/col/align ', i.Size,i.Style, i.Lcol, i.Alignment
        print '    cols ', i.Colors
        print '    cback ', i.Callback, i.Argument
