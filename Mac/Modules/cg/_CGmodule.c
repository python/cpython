
/* =========================== Module _CG =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    	PyErr_SetString(PyExc_NotImplementedError, \
    	"Not available in this shared library/OS version"); \
    	return NULL; \
    }} while(0)


#ifdef WITHOUT_FRAMEWORKS
#include <Quickdraw.h>
#include <CGContext.h>
#else
#include <ApplicationServices/ApplicationServices.h>
#endif

#if !TARGET_API_MAC_OSX
	/* This code is adapted from the CallMachOFramework demo at:
       http://developer.apple.com/samplecode/Sample_Code/Runtime_Architecture/CallMachOFramework.htm
       It allows us to call Mach-O functions from CFM apps. */

	#include <Folders.h>
	#include "CFMLateImport.h"

	static OSStatus LoadFrameworkBundle(CFStringRef framework, CFBundleRef *bundlePtr)
		// This routine finds a the named framework and creates a CFBundle 
		// object for it.  It looks for the framework in the frameworks folder, 
		// as defined by the Folder Manager.  Currently this is 
		// "/System/Library/Frameworks", but we recommend that you avoid hard coded 
		// paths to ensure future compatibility.
		//
		// You might think that you could use CFBundleGetBundleWithIdentifier but 
		// that only finds bundles that are already loaded into your context. 
		// That would work in the case of the System framework but it wouldn't 
		// work if you're using some other, less-obvious, framework.
	{
		OSStatus 	err;
		FSRef 		frameworksFolderRef;
		CFURLRef	baseURL;
		CFURLRef	bundleURL;
		
		*bundlePtr = nil;
		
		baseURL = nil;
		bundleURL = nil;
		
		// Find the frameworks folder and create a URL for it.
		
		err = FSFindFolder(kOnAppropriateDisk, kFrameworksFolderType, true, &frameworksFolderRef);
		if (err == noErr) {
			baseURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &frameworksFolderRef);
			if (baseURL == nil) {
				err = coreFoundationUnknownErr;
			}
		}
		
		// Append the name of the framework to the URL.
		
		if (err == noErr) {
			bundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorSystemDefault, baseURL, framework, false);
			if (bundleURL == nil) {
				err = coreFoundationUnknownErr;
			}
		}
		
		// Create a bundle based on that URL and load the bundle into memory.
		// We never unload the bundle, which is reasonable in this case because 
		// the sample assumes that you'll be calling functions from this 
		// framework throughout the life of your application.
		
		if (err == noErr) {
			*bundlePtr = CFBundleCreate(kCFAllocatorSystemDefault, bundleURL);
			if (*bundlePtr == nil) {
				err = coreFoundationUnknownErr;
			}
		}
		if (err == noErr) {
		    if ( ! CFBundleLoadExecutable( *bundlePtr ) ) {
				err = coreFoundationUnknownErr;
		    }
		}

		// Clean up.
		
		if (err != noErr && *bundlePtr != nil) {
			CFRelease(*bundlePtr);
			*bundlePtr = nil;
		}
		if (bundleURL != nil) {
			CFRelease(bundleURL);
		}	
		if (baseURL != nil) {
			CFRelease(baseURL);
		}	
		
		return err;
	}



	// The CFMLateImport approach requires that you define a fragment 
	// initialisation routine that latches the fragment's connection 
	// ID and locator.  If your code already has a fragment initialiser 
	// you will have to integrate the following into it.

	static CFragConnectionID 			gFragToFixConnID;
	static FSSpec 						gFragToFixFile;
	static CFragSystem7DiskFlatLocator 	gFragToFixLocator;

	extern OSErr FragmentInit(const CFragInitBlock *initBlock);
	extern OSErr FragmentInit(const CFragInitBlock *initBlock)
	{
		__initialize(initBlock); /* call the "original" initializer */
		gFragToFixConnID	= (CFragConnectionID) initBlock->closureID;
		gFragToFixFile 		= *(initBlock->fragLocator.u.onDisk.fileSpec);
		gFragToFixLocator 	= initBlock->fragLocator.u.onDisk;
		gFragToFixLocator.fileSpec = &gFragToFixFile;
		
		return noErr;
	}

#endif

extern int GrafObj_Convert(PyObject *, GrafPtr *);

/*
** Manual converters
*/

PyObject *CGPoint_New(CGPoint *itself)
{

	return Py_BuildValue("(ff)",
			itself->x,
			itself->y);
}

int
CGPoint_Convert(PyObject *v, CGPoint *p_itself)
{
	if( !PyArg_Parse(v, "(ff)",
			&p_itself->x,
			&p_itself->y) )
		return 0;
	return 1;
}

PyObject *CGRect_New(CGRect *itself)
{

	return Py_BuildValue("(ffff)",
			itself->origin.x,
			itself->origin.y,
			itself->size.width,
			itself->size.height);
}

int
CGRect_Convert(PyObject *v, CGRect *p_itself)
{
	if( !PyArg_Parse(v, "(ffff)",
			&p_itself->origin.x,
			&p_itself->origin.y,
			&p_itself->size.width,
			&p_itself->size.height) )
		return 0;
	return 1;
}

PyObject *CGAffineTransform_New(CGAffineTransform *itself)
{

	return Py_BuildValue("(ffffff)",
			itself->a,
			itself->b,
			itself->c,
			itself->d,
			itself->tx,
			itself->ty);
}

int
CGAffineTransform_Convert(PyObject *v, CGAffineTransform *p_itself)
{
	if( !PyArg_Parse(v, "(ffffff)",
			&p_itself->a,
			&p_itself->b,
			&p_itself->c,
			&p_itself->d,
			&p_itself->tx,
			&p_itself->ty) )
		return 0;
	return 1;
}

static PyObject *CG_Error;

/* -------------------- Object type CGContextRef -------------------- */

PyTypeObject CGContextRef_Type;

#define CGContextRefObj_Check(x) ((x)->ob_type == &CGContextRef_Type)

typedef struct CGContextRefObject {
	PyObject_HEAD
	CGContextRef ob_itself;
} CGContextRefObject;

PyObject *CGContextRefObj_New(CGContextRef itself)
{
	CGContextRefObject *it;
	it = PyObject_NEW(CGContextRefObject, &CGContextRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int CGContextRefObj_Convert(PyObject *v, CGContextRef *p_itself)
{
	if (!CGContextRefObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "CGContextRef required");
		return 0;
	}
	*p_itself = ((CGContextRefObject *)v)->ob_itself;
	return 1;
}

static void CGContextRefObj_dealloc(CGContextRefObject *self)
{
	CGContextRelease(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *CGContextRefObj_CGContextSaveGState(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextSaveGState(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextRestoreGState(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextRestoreGState(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextScaleCTM(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float sx;
	float sy;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &sx,
	                      &sy))
		return NULL;
	CGContextScaleCTM(_self->ob_itself,
	                  sx,
	                  sy);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextTranslateCTM(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float tx;
	float ty;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &tx,
	                      &ty))
		return NULL;
	CGContextTranslateCTM(_self->ob_itself,
	                      tx,
	                      ty);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextRotateCTM(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float angle;
	if (!PyArg_ParseTuple(_args, "f",
	                      &angle))
		return NULL;
	CGContextRotateCTM(_self->ob_itself,
	                   angle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextConcatCTM(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGAffineTransform transform;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGAffineTransform_Convert, &transform))
		return NULL;
	CGContextConcatCTM(_self->ob_itself,
	                   transform);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextGetCTM(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGAffineTransform _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CGContextGetCTM(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CGAffineTransform_New, &_rv);
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetLineWidth(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float width;
	if (!PyArg_ParseTuple(_args, "f",
	                      &width))
		return NULL;
	CGContextSetLineWidth(_self->ob_itself,
	                      width);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetLineCap(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	int cap;
	if (!PyArg_ParseTuple(_args, "i",
	                      &cap))
		return NULL;
	CGContextSetLineCap(_self->ob_itself,
	                    cap);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetLineJoin(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	int join;
	if (!PyArg_ParseTuple(_args, "i",
	                      &join))
		return NULL;
	CGContextSetLineJoin(_self->ob_itself,
	                     join);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetMiterLimit(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float limit;
	if (!PyArg_ParseTuple(_args, "f",
	                      &limit))
		return NULL;
	CGContextSetMiterLimit(_self->ob_itself,
	                       limit);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetFlatness(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float flatness;
	if (!PyArg_ParseTuple(_args, "f",
	                      &flatness))
		return NULL;
	CGContextSetFlatness(_self->ob_itself,
	                     flatness);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetAlpha(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float alpha;
	if (!PyArg_ParseTuple(_args, "f",
	                      &alpha))
		return NULL;
	CGContextSetAlpha(_self->ob_itself,
	                  alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextBeginPath(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextBeginPath(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextMoveToPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float x;
	float y;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &x,
	                      &y))
		return NULL;
	CGContextMoveToPoint(_self->ob_itself,
	                     x,
	                     y);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextAddLineToPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float x;
	float y;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &x,
	                      &y))
		return NULL;
	CGContextAddLineToPoint(_self->ob_itself,
	                        x,
	                        y);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextAddCurveToPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float cp1x;
	float cp1y;
	float cp2x;
	float cp2y;
	float x;
	float y;
	if (!PyArg_ParseTuple(_args, "ffffff",
	                      &cp1x,
	                      &cp1y,
	                      &cp2x,
	                      &cp2y,
	                      &x,
	                      &y))
		return NULL;
	CGContextAddCurveToPoint(_self->ob_itself,
	                         cp1x,
	                         cp1y,
	                         cp2x,
	                         cp2y,
	                         x,
	                         y);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextAddQuadCurveToPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float cpx;
	float cpy;
	float x;
	float y;
	if (!PyArg_ParseTuple(_args, "ffff",
	                      &cpx,
	                      &cpy,
	                      &x,
	                      &y))
		return NULL;
	CGContextAddQuadCurveToPoint(_self->ob_itself,
	                             cpx,
	                             cpy,
	                             x,
	                             y);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextClosePath(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextClosePath(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextAddRect(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect rect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGRect_Convert, &rect))
		return NULL;
	CGContextAddRect(_self->ob_itself,
	                 rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextAddArc(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float x;
	float y;
	float radius;
	float startAngle;
	float endAngle;
	int clockwise;
	if (!PyArg_ParseTuple(_args, "fffffi",
	                      &x,
	                      &y,
	                      &radius,
	                      &startAngle,
	                      &endAngle,
	                      &clockwise))
		return NULL;
	CGContextAddArc(_self->ob_itself,
	                x,
	                y,
	                radius,
	                startAngle,
	                endAngle,
	                clockwise);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextAddArcToPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float x1;
	float y1;
	float x2;
	float y2;
	float radius;
	if (!PyArg_ParseTuple(_args, "fffff",
	                      &x1,
	                      &y1,
	                      &x2,
	                      &y2,
	                      &radius))
		return NULL;
	CGContextAddArcToPoint(_self->ob_itself,
	                       x1,
	                       y1,
	                       x2,
	                       y2,
	                       radius);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextIsPathEmpty(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	int _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CGContextIsPathEmpty(_self->ob_itself);
	_res = Py_BuildValue("i",
	                     _rv);
	return _res;
}

static PyObject *CGContextRefObj_CGContextGetPathCurrentPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGPoint _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CGContextGetPathCurrentPoint(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CGPoint_New, &_rv);
	return _res;
}

static PyObject *CGContextRefObj_CGContextGetPathBoundingBox(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CGContextGetPathBoundingBox(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CGRect_New, &_rv);
	return _res;
}

static PyObject *CGContextRefObj_CGContextDrawPath(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	int mode;
	if (!PyArg_ParseTuple(_args, "i",
	                      &mode))
		return NULL;
	CGContextDrawPath(_self->ob_itself,
	                  mode);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextFillPath(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextFillPath(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextEOFillPath(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextEOFillPath(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextStrokePath(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextStrokePath(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextFillRect(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect rect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGRect_Convert, &rect))
		return NULL;
	CGContextFillRect(_self->ob_itself,
	                  rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextStrokeRect(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect rect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGRect_Convert, &rect))
		return NULL;
	CGContextStrokeRect(_self->ob_itself,
	                    rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextStrokeRectWithWidth(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect rect;
	float width;
	if (!PyArg_ParseTuple(_args, "O&f",
	                      CGRect_Convert, &rect,
	                      &width))
		return NULL;
	CGContextStrokeRectWithWidth(_self->ob_itself,
	                             rect,
	                             width);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextClearRect(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect rect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGRect_Convert, &rect))
		return NULL;
	CGContextClearRect(_self->ob_itself,
	                   rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextClip(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextClip(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextEOClip(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextEOClip(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextClipToRect(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGRect rect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGRect_Convert, &rect))
		return NULL;
	CGContextClipToRect(_self->ob_itself,
	                    rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetGrayFillColor(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float gray;
	float alpha;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &gray,
	                      &alpha))
		return NULL;
	CGContextSetGrayFillColor(_self->ob_itself,
	                          gray,
	                          alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetGrayStrokeColor(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float gray;
	float alpha;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &gray,
	                      &alpha))
		return NULL;
	CGContextSetGrayStrokeColor(_self->ob_itself,
	                            gray,
	                            alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetRGBFillColor(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float r;
	float g;
	float b;
	float alpha;
	if (!PyArg_ParseTuple(_args, "ffff",
	                      &r,
	                      &g,
	                      &b,
	                      &alpha))
		return NULL;
	CGContextSetRGBFillColor(_self->ob_itself,
	                         r,
	                         g,
	                         b,
	                         alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetRGBStrokeColor(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float r;
	float g;
	float b;
	float alpha;
	if (!PyArg_ParseTuple(_args, "ffff",
	                      &r,
	                      &g,
	                      &b,
	                      &alpha))
		return NULL;
	CGContextSetRGBStrokeColor(_self->ob_itself,
	                           r,
	                           g,
	                           b,
	                           alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetCMYKFillColor(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float c;
	float m;
	float y;
	float k;
	float alpha;
	if (!PyArg_ParseTuple(_args, "fffff",
	                      &c,
	                      &m,
	                      &y,
	                      &k,
	                      &alpha))
		return NULL;
	CGContextSetCMYKFillColor(_self->ob_itself,
	                          c,
	                          m,
	                          y,
	                          k,
	                          alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetCMYKStrokeColor(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float c;
	float m;
	float y;
	float k;
	float alpha;
	if (!PyArg_ParseTuple(_args, "fffff",
	                      &c,
	                      &m,
	                      &y,
	                      &k,
	                      &alpha))
		return NULL;
	CGContextSetCMYKStrokeColor(_self->ob_itself,
	                            c,
	                            m,
	                            y,
	                            k,
	                            alpha);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetCharacterSpacing(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float spacing;
	if (!PyArg_ParseTuple(_args, "f",
	                      &spacing))
		return NULL;
	CGContextSetCharacterSpacing(_self->ob_itself,
	                             spacing);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetTextPosition(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float x;
	float y;
	if (!PyArg_ParseTuple(_args, "ff",
	                      &x,
	                      &y))
		return NULL;
	CGContextSetTextPosition(_self->ob_itself,
	                         x,
	                         y);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextGetTextPosition(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGPoint _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CGContextGetTextPosition(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CGPoint_New, &_rv);
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetTextMatrix(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGAffineTransform transform;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CGAffineTransform_Convert, &transform))
		return NULL;
	CGContextSetTextMatrix(_self->ob_itself,
	                       transform);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextGetTextMatrix(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGAffineTransform _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CGContextGetTextMatrix(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CGAffineTransform_New, &_rv);
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetTextDrawingMode(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	int mode;
	if (!PyArg_ParseTuple(_args, "i",
	                      &mode))
		return NULL;
	CGContextSetTextDrawingMode(_self->ob_itself,
	                            mode);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetFontSize(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float size;
	if (!PyArg_ParseTuple(_args, "f",
	                      &size))
		return NULL;
	CGContextSetFontSize(_self->ob_itself,
	                     size);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSelectFont(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char * name;
	float size;
	int textEncoding;
	if (!PyArg_ParseTuple(_args, "sfi",
	                      &name,
	                      &size,
	                      &textEncoding))
		return NULL;
	CGContextSelectFont(_self->ob_itself,
	                    name,
	                    size,
	                    textEncoding);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextShowText(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *cstring__in__;
	long cstring__len__;
	int cstring__in_len__;
	if (!PyArg_ParseTuple(_args, "s#",
	                      &cstring__in__, &cstring__in_len__))
		return NULL;
	cstring__len__ = cstring__in_len__;
	CGContextShowText(_self->ob_itself,
	                  cstring__in__, cstring__len__);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextShowTextAtPoint(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	float x;
	float y;
	char *cstring__in__;
	long cstring__len__;
	int cstring__in_len__;
	if (!PyArg_ParseTuple(_args, "ffs#",
	                      &x,
	                      &y,
	                      &cstring__in__, &cstring__in_len__))
		return NULL;
	cstring__len__ = cstring__in_len__;
	CGContextShowTextAtPoint(_self->ob_itself,
	                         x,
	                         y,
	                         cstring__in__, cstring__len__);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextEndPage(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextEndPage(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextFlush(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextFlush(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSynchronize(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CGContextSynchronize(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CGContextRefObj_CGContextSetShouldAntialias(CGContextRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	int shouldAntialias;
	if (!PyArg_ParseTuple(_args, "i",
	                      &shouldAntialias))
		return NULL;
	CGContextSetShouldAntialias(_self->ob_itself,
	                            shouldAntialias);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef CGContextRefObj_methods[] = {
	{"CGContextSaveGState", (PyCFunction)CGContextRefObj_CGContextSaveGState, 1,
	 "() -> None"},
	{"CGContextRestoreGState", (PyCFunction)CGContextRefObj_CGContextRestoreGState, 1,
	 "() -> None"},
	{"CGContextScaleCTM", (PyCFunction)CGContextRefObj_CGContextScaleCTM, 1,
	 "(float sx, float sy) -> None"},
	{"CGContextTranslateCTM", (PyCFunction)CGContextRefObj_CGContextTranslateCTM, 1,
	 "(float tx, float ty) -> None"},
	{"CGContextRotateCTM", (PyCFunction)CGContextRefObj_CGContextRotateCTM, 1,
	 "(float angle) -> None"},
	{"CGContextConcatCTM", (PyCFunction)CGContextRefObj_CGContextConcatCTM, 1,
	 "(CGAffineTransform transform) -> None"},
	{"CGContextGetCTM", (PyCFunction)CGContextRefObj_CGContextGetCTM, 1,
	 "() -> (CGAffineTransform _rv)"},
	{"CGContextSetLineWidth", (PyCFunction)CGContextRefObj_CGContextSetLineWidth, 1,
	 "(float width) -> None"},
	{"CGContextSetLineCap", (PyCFunction)CGContextRefObj_CGContextSetLineCap, 1,
	 "(int cap) -> None"},
	{"CGContextSetLineJoin", (PyCFunction)CGContextRefObj_CGContextSetLineJoin, 1,
	 "(int join) -> None"},
	{"CGContextSetMiterLimit", (PyCFunction)CGContextRefObj_CGContextSetMiterLimit, 1,
	 "(float limit) -> None"},
	{"CGContextSetFlatness", (PyCFunction)CGContextRefObj_CGContextSetFlatness, 1,
	 "(float flatness) -> None"},
	{"CGContextSetAlpha", (PyCFunction)CGContextRefObj_CGContextSetAlpha, 1,
	 "(float alpha) -> None"},
	{"CGContextBeginPath", (PyCFunction)CGContextRefObj_CGContextBeginPath, 1,
	 "() -> None"},
	{"CGContextMoveToPoint", (PyCFunction)CGContextRefObj_CGContextMoveToPoint, 1,
	 "(float x, float y) -> None"},
	{"CGContextAddLineToPoint", (PyCFunction)CGContextRefObj_CGContextAddLineToPoint, 1,
	 "(float x, float y) -> None"},
	{"CGContextAddCurveToPoint", (PyCFunction)CGContextRefObj_CGContextAddCurveToPoint, 1,
	 "(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) -> None"},
	{"CGContextAddQuadCurveToPoint", (PyCFunction)CGContextRefObj_CGContextAddQuadCurveToPoint, 1,
	 "(float cpx, float cpy, float x, float y) -> None"},
	{"CGContextClosePath", (PyCFunction)CGContextRefObj_CGContextClosePath, 1,
	 "() -> None"},
	{"CGContextAddRect", (PyCFunction)CGContextRefObj_CGContextAddRect, 1,
	 "(CGRect rect) -> None"},
	{"CGContextAddArc", (PyCFunction)CGContextRefObj_CGContextAddArc, 1,
	 "(float x, float y, float radius, float startAngle, float endAngle, int clockwise) -> None"},
	{"CGContextAddArcToPoint", (PyCFunction)CGContextRefObj_CGContextAddArcToPoint, 1,
	 "(float x1, float y1, float x2, float y2, float radius) -> None"},
	{"CGContextIsPathEmpty", (PyCFunction)CGContextRefObj_CGContextIsPathEmpty, 1,
	 "() -> (int _rv)"},
	{"CGContextGetPathCurrentPoint", (PyCFunction)CGContextRefObj_CGContextGetPathCurrentPoint, 1,
	 "() -> (CGPoint _rv)"},
	{"CGContextGetPathBoundingBox", (PyCFunction)CGContextRefObj_CGContextGetPathBoundingBox, 1,
	 "() -> (CGRect _rv)"},
	{"CGContextDrawPath", (PyCFunction)CGContextRefObj_CGContextDrawPath, 1,
	 "(int mode) -> None"},
	{"CGContextFillPath", (PyCFunction)CGContextRefObj_CGContextFillPath, 1,
	 "() -> None"},
	{"CGContextEOFillPath", (PyCFunction)CGContextRefObj_CGContextEOFillPath, 1,
	 "() -> None"},
	{"CGContextStrokePath", (PyCFunction)CGContextRefObj_CGContextStrokePath, 1,
	 "() -> None"},
	{"CGContextFillRect", (PyCFunction)CGContextRefObj_CGContextFillRect, 1,
	 "(CGRect rect) -> None"},
	{"CGContextStrokeRect", (PyCFunction)CGContextRefObj_CGContextStrokeRect, 1,
	 "(CGRect rect) -> None"},
	{"CGContextStrokeRectWithWidth", (PyCFunction)CGContextRefObj_CGContextStrokeRectWithWidth, 1,
	 "(CGRect rect, float width) -> None"},
	{"CGContextClearRect", (PyCFunction)CGContextRefObj_CGContextClearRect, 1,
	 "(CGRect rect) -> None"},
	{"CGContextClip", (PyCFunction)CGContextRefObj_CGContextClip, 1,
	 "() -> None"},
	{"CGContextEOClip", (PyCFunction)CGContextRefObj_CGContextEOClip, 1,
	 "() -> None"},
	{"CGContextClipToRect", (PyCFunction)CGContextRefObj_CGContextClipToRect, 1,
	 "(CGRect rect) -> None"},
	{"CGContextSetGrayFillColor", (PyCFunction)CGContextRefObj_CGContextSetGrayFillColor, 1,
	 "(float gray, float alpha) -> None"},
	{"CGContextSetGrayStrokeColor", (PyCFunction)CGContextRefObj_CGContextSetGrayStrokeColor, 1,
	 "(float gray, float alpha) -> None"},
	{"CGContextSetRGBFillColor", (PyCFunction)CGContextRefObj_CGContextSetRGBFillColor, 1,
	 "(float r, float g, float b, float alpha) -> None"},
	{"CGContextSetRGBStrokeColor", (PyCFunction)CGContextRefObj_CGContextSetRGBStrokeColor, 1,
	 "(float r, float g, float b, float alpha) -> None"},
	{"CGContextSetCMYKFillColor", (PyCFunction)CGContextRefObj_CGContextSetCMYKFillColor, 1,
	 "(float c, float m, float y, float k, float alpha) -> None"},
	{"CGContextSetCMYKStrokeColor", (PyCFunction)CGContextRefObj_CGContextSetCMYKStrokeColor, 1,
	 "(float c, float m, float y, float k, float alpha) -> None"},
	{"CGContextSetCharacterSpacing", (PyCFunction)CGContextRefObj_CGContextSetCharacterSpacing, 1,
	 "(float spacing) -> None"},
	{"CGContextSetTextPosition", (PyCFunction)CGContextRefObj_CGContextSetTextPosition, 1,
	 "(float x, float y) -> None"},
	{"CGContextGetTextPosition", (PyCFunction)CGContextRefObj_CGContextGetTextPosition, 1,
	 "() -> (CGPoint _rv)"},
	{"CGContextSetTextMatrix", (PyCFunction)CGContextRefObj_CGContextSetTextMatrix, 1,
	 "(CGAffineTransform transform) -> None"},
	{"CGContextGetTextMatrix", (PyCFunction)CGContextRefObj_CGContextGetTextMatrix, 1,
	 "() -> (CGAffineTransform _rv)"},
	{"CGContextSetTextDrawingMode", (PyCFunction)CGContextRefObj_CGContextSetTextDrawingMode, 1,
	 "(int mode) -> None"},
	{"CGContextSetFontSize", (PyCFunction)CGContextRefObj_CGContextSetFontSize, 1,
	 "(float size) -> None"},
	{"CGContextSelectFont", (PyCFunction)CGContextRefObj_CGContextSelectFont, 1,
	 "(char * name, float size, int textEncoding) -> None"},
	{"CGContextShowText", (PyCFunction)CGContextRefObj_CGContextShowText, 1,
	 "(Buffer cstring) -> None"},
	{"CGContextShowTextAtPoint", (PyCFunction)CGContextRefObj_CGContextShowTextAtPoint, 1,
	 "(float x, float y, Buffer cstring) -> None"},
	{"CGContextEndPage", (PyCFunction)CGContextRefObj_CGContextEndPage, 1,
	 "() -> None"},
	{"CGContextFlush", (PyCFunction)CGContextRefObj_CGContextFlush, 1,
	 "() -> None"},
	{"CGContextSynchronize", (PyCFunction)CGContextRefObj_CGContextSynchronize, 1,
	 "() -> None"},
	{"CGContextSetShouldAntialias", (PyCFunction)CGContextRefObj_CGContextSetShouldAntialias, 1,
	 "(int shouldAntialias) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain CGContextRefObj_chain = { CGContextRefObj_methods, NULL };

static PyObject *CGContextRefObj_getattr(CGContextRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CGContextRefObj_chain, (PyObject *)self, name);
}

#define CGContextRefObj_setattr NULL

#define CGContextRefObj_compare NULL

#define CGContextRefObj_repr NULL

#define CGContextRefObj_hash NULL

PyTypeObject CGContextRef_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_CG.CGContextRef", /*tp_name*/
	sizeof(CGContextRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CGContextRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CGContextRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CGContextRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CGContextRefObj_compare, /*tp_compare*/
	(reprfunc) CGContextRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CGContextRefObj_hash, /*tp_hash*/
};

/* ------------------ End object type CGContextRef ------------------ */


static PyObject *CG_CreateCGContextForPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GrafPtr port;
	CGContextRef ctx;
	OSStatus _err;

	if (!PyArg_ParseTuple(_args, "O&", GrafObj_Convert, &port))
		return NULL;

	_err = CreateCGContextForPort(port, &ctx);
	if (_err != noErr)
		if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&", CGContextRefObj_New, ctx);
	return _res;

}

static PyMethodDef CG_methods[] = {
	{"CreateCGContextForPort", (PyCFunction)CG_CreateCGContextForPort, 1,
	 "(CGrafPtr) -> CGContextRef"},
	{NULL, NULL, 0}
};




void init_CG(void)
{
	PyObject *m;
	PyObject *d;



#if !TARGET_API_MAC_OSX
	CFBundleRef sysBundle;
	OSStatus err;

	if (&LoadFrameworkBundle == NULL) {
		PyErr_SetString(PyExc_ImportError, "CoreCraphics not supported");
		return;
	}
	err = LoadFrameworkBundle(CFSTR("ApplicationServices.framework"), &sysBundle);
	if (err == noErr)
		err = CFMLateImportBundle(&gFragToFixLocator, gFragToFixConnID, FragmentInit, "\pCGStubLib", sysBundle);
	if (err != noErr) {
		PyErr_SetString(PyExc_ImportError, "CoreCraphics not supported");
		return;
	};
#endif  /* !TARGET_API_MAC_OSX */


	m = Py_InitModule("_CG", CG_methods);
	d = PyModule_GetDict(m);
	CG_Error = PyMac_GetOSErrException();
	if (CG_Error == NULL ||
	    PyDict_SetItemString(d, "Error", CG_Error) != 0)
		return;
	CGContextRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CGContextRef_Type);
	if (PyDict_SetItemString(d, "CGContextRefType", (PyObject *)&CGContextRef_Type) != 0)
		Py_FatalError("can't initialize CGContextRefType");
}

/* ========================= End module _CG ========================= */

