#
# Genmodule - A python program to help you build (template) modules.
#
# Usage:
#
# o = genmodule.object()
# o.name = 'dwarve object'
# o.abbrev = 'dw'
# o.funclist = ['new', 'dealloc', 'getattr', 'setattr']
# o.methodlist = ['dig']
#
# m = genmodule.module()
# m.name = 'beings'
# m.abbrev = 'be'
# m.methodlist = ['newdwarve']
# m.objects = [o]
#
# genmodule.write(sys.stdout, m)
#
import sys
import os
import varsubst

error = 'genmodule.error'

#
# Names of functions in the object-description struct.
#
FUNCLIST = ['new', 'tp_dealloc', 'tp_print', 'tp_getattr', 'tp_setattr',
            'tp_compare', 'tp_repr', 'tp_hash', 'tp_call', 'tp_str']
TYPELIST = ['tp_as_number', 'tp_as_sequence', 'tp_as_mapping', 'structure']

#
# writer is a base class for the object and module classes
# it contains code common to both.
#
class writer:
    def __init__(self):
        self._subst = None

    def makesubst(self):
        if not self._subst:
            if not self.__dict__.has_key('abbrev'):
                self.abbrev = self.name
            self.Abbrev = self.abbrev[0].upper()+self.abbrev[1:]
            subst = varsubst.Varsubst(self.__dict__)
            subst.useindent(1)
            self._subst = subst.subst

    def addcode(self, name, fp):
        ifp = self.opentemplate(name)
        self.makesubst()
        d = ifp.read()
        d = self._subst(d)
        fp.write(d)

    def opentemplate(self, name):
        for p in sys.path:
            fn = os.path.join(p, name)
            if os.path.exists(fn):
                return open(fn, 'r')
            fn = os.path.join(p, 'Templates')
            fn = os.path.join(fn, name)
            if os.path.exists(fn):
                return open(fn, 'r')
        raise error('Template '+name+' not found for '+self._type+' '+ \
                     self.name)

class module(writer):
    _type = 'module'

    def writecode(self, fp):
        self.addcode('copyright', fp)
        self.addcode('module_head', fp)
        for o in self.objects:
            o.writehead(fp)
        for o in self.objects:
            o.writebody(fp)
        new_ml = ''
        for fn in self.methodlist:
            self.method = fn
            self.addcode('module_method', fp)
            new_ml = new_ml + (
                      '{"%s",\t(PyCFunction)%s_%s,\tMETH_VARARGS,\t%s_%s__doc__},\n'
                      %(fn, self.abbrev, fn, self.abbrev, fn))
        self.methodlist = new_ml
        self.addcode('module_tail', fp)

class object(writer):
    _type = 'object'
    def __init__(self):
        self.typelist = []
        self.methodlist = []
        self.funclist = ['new']
        writer.__init__(self)

    def writecode(self, fp):
        self.addcode('copyright', fp)
        self.writehead(fp)
        self.writebody(fp)

    def writehead(self, fp):
        self.addcode('object_head', fp)

    def writebody(self, fp):
        new_ml = ''
        for fn in self.methodlist:
            self.method = fn
            self.addcode('object_method', fp)
            new_ml = new_ml + (
                      '{"%s",\t(PyCFunction)%s_%s,\tMETH_VARARGS,\t%s_%s__doc__},\n'
                      %(fn, self.abbrev, fn, self.abbrev, fn))
        self.methodlist = new_ml
        self.addcode('object_mlist', fp)

        # Add getattr if we have methods
        if self.methodlist and not 'tp_getattr' in self.funclist:
            self.funclist.insert(0, 'tp_getattr')

        for fn in FUNCLIST:
            setattr(self, fn, '0')

        #
        # Special case for structure-access objects: put getattr in the
        # list of functions but don't generate code for it directly,
        # the code is obtained from the object_structure template.
        # The same goes for setattr.
        #
        if 'structure' in self.typelist:
            if 'tp_getattr' in self.funclist:
                self.funclist.remove('tp_getattr')
            if 'tp_setattr' in self.funclist:
                self.funclist.remove('tp_setattr')
            self.tp_getattr = self.abbrev + '_getattr'
            self.tp_setattr = self.abbrev + '_setattr'
        for fn in self.funclist:
            self.addcode('object_'+fn, fp)
            setattr(self, fn, '%s_%s'%(self.abbrev, fn[3:]))
        for tn in TYPELIST:
            setattr(self, tn, '0')
        for tn in self.typelist:
            self.addcode('object_'+tn, fp)
            setattr(self, tn, '&%s_%s'%(self.abbrev, tn[3:]))
        self.addcode('object_tail', fp)

def write(fp, obj):
    obj.writecode(fp)

if __name__ == '__main__':
    o = object()
    o.name = 'dwarve object'
    o.abbrev = 'dw'
    o.funclist = ['new', 'tp_dealloc']
    o.methodlist = ['dig']
    m = module()
    m.name = 'beings'
    m.abbrev = 'be'
    m.methodlist = ['newdwarve']
    m.objects = [o]
    write(sys.stdout, m)
