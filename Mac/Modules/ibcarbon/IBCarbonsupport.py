# IBCarbonsupport.py

from macsupport import *

IBNibRef = OpaqueByValueType('IBNibRef', 'IBNibRefObj')
#CFBundleRef = OpaqueByValueType('CFBundleRef')

IBCarbonFunction = OSErrFunctionGenerator
IBCarbonMethod = OSErrMethodGenerator

includestuff = """
#include <Carbon/Carbon.h>
#include "pymactoolbox.h"

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern int _CFStringRefObj_Convert(PyObject *, CFStringRef *);
#endif

"""

initstuff = """

"""

module = MacModule('_IBCarbon', 'IBCarbon', includestuff, finalstuff, initstuff)

class CFReleaserObject(PEP253Mixin, GlobalObjectDefinition):
    def outputFreeIt(self, name):
        Output("CFRelease(%s);" % name)

class CFNibDesc(PEP253Mixin, GlobalObjectDefinition):
    def outputFreeIt(self, name):
        Output("DisposeNibReference(%s);" % name)

#cfstringobject = CFReleaserObject("CFStringRef")
#module.addobject(cfstringobject)
#cfbundleobject = CFReleaserObject("CFBundleRef")
#module.addobject(cfbundleobject)
ibnibobject = CFNibDesc("IBNibRef", "IBNibRefObj")
module.addobject(ibnibobject)

functions = []
methods = []

exec(open('IBCarbongen.py').read())

for f in functions: module.add(f)
for m in methods: ibnibobject.add(m)

SetOutputFileName('_IBCarbon.c')
module.generate()
