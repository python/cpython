#! /usr/bin/env python
#
# Modulator - Generate skeleton modules.
#
# The user fills out some forms with information about what the module
# should support (methods, objects), names of these things, prefixes to
# use for C code, whether the objects should also support access as numbers,
# etc etc etc.
# When the user presses 'Generate code' we generate a complete skeleton
# module in C.
#
# Alternatively, the selections made can be save to a python sourcefile and
# this sourcefile can be passed on the command line (resulting in the same
# skeleton C code).
#
# Jack Jansen, CWI, October 1994.
#

import sys, os
if os.name <> 'mac':
    sys.path.append(os.path.join(os.environ['HOME'],
                                 'src/python/Tools/modulator'))

from Tkinter import *
from Tkextra import *
from ScrolledListbox import ScrolledListbox
import sys
import genmodule
import string

oops = 'oops'

# Check that string is a legal C identifier
def checkid(str):
    if not str: return 0
    if not str[0] in string.letters+'_':
        return 0
    for c in str[1:]:
        if not c in string.letters+string.digits+'_':
            return 0
    return 1

def getlistlist(list):
    rv = []
    n = list.size()
    for i in range(n):
        rv.append(list.get(i))
    return rv
    
class UI:
    def __init__(self):
        self.main = Frame()
        self.main.pack()
        self.main.master.title('Modulator: Module view')
        self.cmdframe = Frame(self.main, {'relief':'raised', 'bd':'0.5m',
                                          Pack:{'side':'top',
                                                 'fill':'x'}})
        self.objframe = Frame(self.main, {Pack:{'side':'top', 'fill':'x',
                                                'expand':1}})


        self.check_button = Button(self.cmdframe,
                                  {'text':'Check', 'command':self.cb_check,
                                   Pack:{'side':'left', 'padx':'0.5m'}})
        self.save_button = Button(self.cmdframe,
                                  {'text':'Save...', 'command':self.cb_save,
                                   Pack:{'side':'left', 'padx':'0.5m'}})
        self.code_button = Button(self.cmdframe,
                                  {'text':'Generate code...',
                                   'command':self.cb_gencode,
                                   Pack:{'side':'left', 'padx':'0.5m'}})
        self.quit_button = Button(self.cmdframe,
                                  {'text':'Quit',
                                   'command':self.cb_quit,
                                   Pack:{'side':'right', 'padx':'0.5m'}})

        self.module = UI_module(self)
        self.objects = []
        self.modified = 0

    def run(self):
        self.main.mainloop()

    def cb_quit(self, *args):
        if self.modified:
            if not askyn('You have not saved\nAre you sure you want to quit?'):
                return
        sys.exit(0)

    def cb_check(self, *args):
        try:
            self.module.synchronize()
            for o in self.objects:
                o.synchronize()
        except oops:
            pass
        
    def cb_save(self, *args):
        try:
            pycode = self.module.gencode('m', self.objects)
        except oops:
            return

        fn = askfile('Python file name: ')
        if not fn:
            return

        fp = open(fn, 'w')

        fp.write(pycode)
        fp.close()

    def cb_gencode(self, *args):
        try:
            pycode = self.module.gencode('m', self.objects)
        except oops:
            pass

        fn = askfile('C file name: ')
        if not fn:
            return

        fp = open(fn, 'w')

        try:
            exec pycode
        except:
            message('An error occurred:-)')
            return
        genmodule.write(fp, m)
        fp.close()

class UI_module:
    def __init__(self, parent):
        self.parent = parent
        self.frame = Frame(parent.objframe, {'relief':'raised', 'bd':'0.2m',
                                             Pack:{'side':'top',
                                                   'fill':'x'}})
        self.f1 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        self.f2 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        self.f3 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        self.f4 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})

        self.l1 = Label(self.f1, {'text':'Module:', Pack:{'side':'left',
                                                        'padx':'0.5m'}})
        self.name_entry = Entry(self.f1, {'relief':'sunken',
                              Pack:{'side':'left', 'padx':'0.5m', 'expand':1}})
        self.l2 = Label(self.f1, {'text':'Abbrev:', Pack:{'side':'left',
                                                        'padx':'0.5m'}})
        self.abbrev_entry = Entry(self.f1, {'relief':'sunken', 'width':5,
                              Pack:{'side':'left', 'padx':'0.5m'}})

        self.l3 = Label(self.f2, {'text':'Methods:', Pack:{'side':'left',
                                                        'padx':'0.5m'}})
        self.method_list = ScrolledListbox(self.f2, {'relief':'sunken','bd':2,
                                      Pack:{'side':'left', 'expand':1,
                                            'padx':'0.5m', 'fill':'both'}})

        self.l4 = Label(self.f3, {'text':'Add method:', Pack:{'side':'left',
                                                              'padx':'0.5m'}})
        self.method_entry = Entry(self.f3, {'relief':'sunken',
                             Pack:{'side':'left', 'padx':'0.5m', 'expand':1}})
        self.method_entry.bind('<Return>', self.cb_method)
        self.delete_button = Button(self.f3, {'text':'Delete method',
                                              'command':self.cb_delmethod,
                                              Pack:{'side':'left',
                                                    'padx':'0.5m'}})

        self.newobj_button = Button(self.f4, {'text':'new object',
                                              'command':self.cb_newobj,
                                              Pack:{'side':'left',
                                                    'padx':'0.5m'}})
        
    def cb_delmethod(self, *args):
        list = self.method_list.curselection()
        for i in list:
            self.method_list.delete(i)
        
    def cb_newobj(self, *arg):
        self.parent.objects.append(UI_object(self.parent))

    def cb_method(self, *arg):
        name = self.method_entry.get()
        if not name:
            return
        self.method_entry.delete('0', 'end')
        self.method_list.insert('end', name)

    def synchronize(self):
        n = self.name_entry.get()
        if not n:
            message('Module name not set')
            raise oops
        if not checkid(n):
            message('Module name not an identifier:\n'+n)
            raise oops
        if not self.abbrev_entry.get():
            self.abbrev_entry.insert('end', n)
        m = getlistlist(self.method_list)
        for n in m:
            if not checkid(n):
                message('Method name not an identifier:\n'+n)
                raise oops
            
    def gencode(self, name, objects):
        rv = ''
        self.synchronize()
        for o in objects:
            o.synchronize()
        onames = []
        for i in range(len(objects)):
            oname = 'o'+`i+1`
            rv = rv + objects[i].gencode(oname)
            onames.append(oname)
        rv = rv + (name+' = genmodule.module()\n')
        rv = rv + (name+'.name = '+`self.name_entry.get()`+'\n')
        rv = rv + (name+'.abbrev = '+`self.abbrev_entry.get()`+'\n')
        rv = rv + (name+'.methodlist = '+`getlistlist(self.method_list)`+'\n')
        rv = rv + (name+'.objects = ['+string.joinfields(onames, ',')+']\n')
        rv = rv + ('\n')
        return rv
        
object_number = 0

class UI_object:
    def __init__(self, parent):
        global object_number

        object_number = object_number + 1
        self.num = object_number
        self.vpref = 'o'+`self.num`+'_'
        self.frame = Toplevel(parent.objframe)
#       self.frame.pack()
        self.frame.title('Modulator: object view')
#       self.frame = Frame(parent.objframe, {'relief':'raised', 'bd':'0.2m',
#                                            Pack:{'side':'top',
#                                                  'fill':'x'}})
        self.f1 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        self.f2 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        self.f3 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        self.f4 = Frame(self.frame, {Pack:{'side':'top', 'pady':'0.5m',
                                           'fill':'x'}})
        

        self.l1 = Label(self.f1, {'text':'Object:', Pack:{'side':'left',
                                                        'padx':'0.5m'}})
        self.name_entry = Entry(self.f1, {'relief':'sunken',
                              Pack:{'side':'left', 'padx':'0.5m', 'expand':1}})
        self.l2 = Label(self.f1, {'text':'Abbrev:', Pack:{'side':'left',
                                                        'padx':'0.5m'}})
        self.abbrev_entry = Entry(self.f1, {'relief':'sunken', 'width':5,
                              Pack:{'side':'left', 'padx':'0.5m'}})

        self.l3 = Label(self.f2, {'text':'Methods:', Pack:{'side':'left',
                                                        'padx':'0.5m'}})
        self.method_list = ScrolledListbox(self.f2, {'relief':'sunken','bd':2,
                                      Pack:{'side':'left', 'expand':1,
                                            'padx':'0.5m', 'fill':'both'}})

        self.l4 = Label(self.f3, {'text':'Add method:', Pack:{'side':'left',
                                                              'padx':'0.5m'}})
        self.method_entry = Entry(self.f3, {'relief':'sunken',
                             Pack:{'side':'left', 'padx':'0.5m', 'expand':1}})
        self.method_entry.bind('<Return>', self.cb_method)
        self.delete_button = Button(self.f3, {'text':'Delete method',
                                              'command':self.cb_delmethod,
                                              Pack:{'side':'left',
                                                    'padx':'0.5m'}})


        self.l5 = Label(self.f4, {'text':'functions:',
                                  Pack:{'side':'left',
                                        'padx':'0.5m'}})
        self.f5 = Frame(self.f4, {Pack:{'side':'left', 'pady':'0.5m',
                                           'fill':'both'}})
        self.l6 = Label(self.f4, {'text':'Types:',
                                  Pack:{'side':'left', 'padx':'0.5m'}})
        self.f6 = Frame(self.f4, {Pack:{'side':'left', 'pady':'0.5m',
                                           'fill':'x'}})
        self.funcs = {}
        for i in genmodule.FUNCLIST:
            vname = self.vpref+i
            self.f5.setvar(vname, 0)
            b = Checkbutton(self.f5, {'variable':vname, 'text':i,
                                      Pack:{'side':'top', 'pady':'0.5m',
                                            'anchor':'w','expand':1}})
            self.funcs[i] = b
        self.f5.setvar(self.vpref+'new', 1)

        self.types = {}
        for i in genmodule.TYPELIST:
            vname = self.vpref + i
            self.f6.setvar(vname, 0)
            b = Checkbutton(self.f6, {'variable':vname, 'text':i,
                                      Pack:{'side':'top', 'pady':'0.5m',
                                            'anchor':'w'}})
            self.types[i] = b
        
    def cb_method(self, *arg):
        name = self.method_entry.get()
        if not name:
            return
        self.method_entry.delete('0', 'end')
        self.method_list.insert('end', name)

    def cb_delmethod(self, *args):
        list = self.method_list.curselection()
        for i in list:
            self.method_list.delete(i)
        
    def synchronize(self):
        n = self.name_entry.get()
        if not n:
            message('Object name not set')
            raise oops
        if not self.abbrev_entry.get():
            self.abbrev_entry.insert('end', n)
        n = self.abbrev_entry.get()
        if not checkid(n):
            message('Abbreviation not an identifier:\n'+n)
            raise oops
        m = getlistlist(self.method_list)
        for n in m:
            if not checkid(n):
                message('Method name not an identifier:\n'+n)
                raise oops
        if m:
            self.f5.setvar(self.vpref+'tp_getattr', 1)
        pass
        
    def gencode(self, name):
        rv = ''
        rv = rv + (name+' = genmodule.object()\n')
        rv = rv + (name+'.name = '+`self.name_entry.get()`+'\n')
        rv = rv + (name+'.abbrev = '+`self.abbrev_entry.get()`+'\n')
        rv = rv + (name+'.methodlist = '+`getlistlist(self.method_list)`+'\n')
        fl = []
        for fn in genmodule.FUNCLIST:
            vname = self.vpref + fn
            if self.f5.getvar(vname) == '1':
                fl.append(fn)
        rv = rv + (name+'.funclist = '+`fl`+'\n')

        fl = []
        for fn in genmodule.TYPELIST:
            vname = self.vpref + fn
            if self.f5.getvar(vname) == '1':
                fl.append(fn)
                
        rv = rv + (name+'.typelist = '+`fl`+'\n')

        rv = rv + ('\n')
        return rv
        

def main():
    if len(sys.argv) < 2:
        ui = UI()
        ui.run()
    elif len(sys.argv) == 2:
        fp = open(sys.argv[1])
        pycode = fp.read()
        try:
            exec pycode
        except:
            sys.stderr.write('An error occurred:-)\n')
            sys.exit(1)
        ##genmodule.write(sys.stdout, m)
    else:
        sys.stderr.write('Usage: modulator [file]\n')
        sys.exit(1)
        
main()
