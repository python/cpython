# IBCarbonsupport.py

from macsupport import *

CFStringRef = OpaqueByValueType('CFStringRef', 'CFStringRefObj')
IBNibRef = OpaqueByValueType('IBNibRef', 'IBNibRefObj')
#CFBundleRef = OpaqueByValueType('CFBundleRef')

IBCarbonFunction = OSErrFunctionGenerator
IBCarbonMethod = OSErrMethodGenerator

includestuff = """
#ifdef WITHOUT_FRAMEWORKS
#include <IBCarbonRuntime.h>
#else
#include <Carbon/Carbon.h>
#endif /* WITHOUT_FRAMEWORKS */
#include "macglue.h"

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern int _CFStringRefObj_Convert(PyObject *, CFStringRef *);
//#define CFStringRefObj_Convert _CFStringRefObj_Convert
#endif

//extern int CFBundleRefObj_Convert(PyObject *, CFBundleRef *);  // need to wrap CFBundle

"""

initstuff = """

"""

module = MacModule('_IBCarbon', 'IBCarbon', includestuff, finalstuff, initstuff)

class CFReleaserObject(GlobalObjectDefinition):
	def outputFreeIt(self, name):
		Output("CFRelease(%s);" % name)

class CFNibDesc(GlobalObjectDefinition):
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

execfile('IBCarbongen.py')

for f in functions: module.add(f)
for m in methods: ibnibobject.add(m)

SetOutputFileName('_IBCarbon.c')
module.generate()
