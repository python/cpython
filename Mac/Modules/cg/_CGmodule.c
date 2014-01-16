
/* =========================== Module _CG =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <ApplicationServices/ApplicationServices.h>

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

#define CGContextRefObj_Check(x) ((x)->ob_type == &CGContextRef_Type || PyObject_TypeCheck((x), &CGContextRef_Type))

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
    self->ob_type->tp_free((PyObject *)self);
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
    float red;
    float green;
    float blue;
    float alpha;
    if (!PyArg_ParseTuple(_args, "ffff",
                          &red,
                          &green,
                          &blue,
                          &alpha))
        return NULL;
    CGContextSetRGBFillColor(_self->ob_itself,
                             red,
                             green,
                             blue,
                             alpha);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CGContextRefObj_CGContextSetRGBStrokeColor(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    float red;
    float green;
    float blue;
    float alpha;
    if (!PyArg_ParseTuple(_args, "ffff",
                          &red,
                          &green,
                          &blue,
                          &alpha))
        return NULL;
    CGContextSetRGBStrokeColor(_self->ob_itself,
                               red,
                               green,
                               blue,
                               alpha);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CGContextRefObj_CGContextSetCMYKFillColor(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    float cyan;
    float magenta;
    float yellow;
    float black;
    float alpha;
    if (!PyArg_ParseTuple(_args, "fffff",
                          &cyan,
                          &magenta,
                          &yellow,
                          &black,
                          &alpha))
        return NULL;
    CGContextSetCMYKFillColor(_self->ob_itself,
                              cyan,
                              magenta,
                              yellow,
                              black,
                              alpha);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CGContextRefObj_CGContextSetCMYKStrokeColor(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    float cyan;
    float magenta;
    float yellow;
    float black;
    float alpha;
    if (!PyArg_ParseTuple(_args, "fffff",
                          &cyan,
                          &magenta,
                          &yellow,
                          &black,
                          &alpha))
        return NULL;
    CGContextSetCMYKStrokeColor(_self->ob_itself,
                                cyan,
                                magenta,
                                yellow,
                                black,
                                alpha);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CGContextRefObj_CGContextGetInterpolationQuality(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    int _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CGContextGetInterpolationQuality(_self->ob_itself);
    _res = Py_BuildValue("i",
                         _rv);
    return _res;
}

static PyObject *CGContextRefObj_CGContextSetInterpolationQuality(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    int quality;
    if (!PyArg_ParseTuple(_args, "i",
                          &quality))
        return NULL;
    CGContextSetInterpolationQuality(_self->ob_itself,
                                     quality);
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

#ifndef __LP64__
static PyObject *CGContextRefObj_SyncCGContextOriginWithPort(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CGrafPtr port;
    if (!PyArg_ParseTuple(_args, "O&",
                          GrafObj_Convert, &port))
        return NULL;
    SyncCGContextOriginWithPort(_self->ob_itself,
                                port);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CGContextRefObj_ClipCGContextToRegion(CGContextRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Rect portRect;
    RgnHandle region;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          PyMac_GetRect, &portRect,
                          ResObj_Convert, &region))
        return NULL;
    ClipCGContextToRegion(_self->ob_itself,
                          &portRect,
                          region);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}
#endif

static PyMethodDef CGContextRefObj_methods[] = {
    {"CGContextSaveGState", (PyCFunction)CGContextRefObj_CGContextSaveGState, 1,
     PyDoc_STR("() -> None")},
    {"CGContextRestoreGState", (PyCFunction)CGContextRefObj_CGContextRestoreGState, 1,
     PyDoc_STR("() -> None")},
    {"CGContextScaleCTM", (PyCFunction)CGContextRefObj_CGContextScaleCTM, 1,
     PyDoc_STR("(float sx, float sy) -> None")},
    {"CGContextTranslateCTM", (PyCFunction)CGContextRefObj_CGContextTranslateCTM, 1,
     PyDoc_STR("(float tx, float ty) -> None")},
    {"CGContextRotateCTM", (PyCFunction)CGContextRefObj_CGContextRotateCTM, 1,
     PyDoc_STR("(float angle) -> None")},
    {"CGContextConcatCTM", (PyCFunction)CGContextRefObj_CGContextConcatCTM, 1,
     PyDoc_STR("(CGAffineTransform transform) -> None")},
    {"CGContextGetCTM", (PyCFunction)CGContextRefObj_CGContextGetCTM, 1,
     PyDoc_STR("() -> (CGAffineTransform _rv)")},
    {"CGContextSetLineWidth", (PyCFunction)CGContextRefObj_CGContextSetLineWidth, 1,
     PyDoc_STR("(float width) -> None")},
    {"CGContextSetLineCap", (PyCFunction)CGContextRefObj_CGContextSetLineCap, 1,
     PyDoc_STR("(int cap) -> None")},
    {"CGContextSetLineJoin", (PyCFunction)CGContextRefObj_CGContextSetLineJoin, 1,
     PyDoc_STR("(int join) -> None")},
    {"CGContextSetMiterLimit", (PyCFunction)CGContextRefObj_CGContextSetMiterLimit, 1,
     PyDoc_STR("(float limit) -> None")},
    {"CGContextSetFlatness", (PyCFunction)CGContextRefObj_CGContextSetFlatness, 1,
     PyDoc_STR("(float flatness) -> None")},
    {"CGContextSetAlpha", (PyCFunction)CGContextRefObj_CGContextSetAlpha, 1,
     PyDoc_STR("(float alpha) -> None")},
    {"CGContextBeginPath", (PyCFunction)CGContextRefObj_CGContextBeginPath, 1,
     PyDoc_STR("() -> None")},
    {"CGContextMoveToPoint", (PyCFunction)CGContextRefObj_CGContextMoveToPoint, 1,
     PyDoc_STR("(float x, float y) -> None")},
    {"CGContextAddLineToPoint", (PyCFunction)CGContextRefObj_CGContextAddLineToPoint, 1,
     PyDoc_STR("(float x, float y) -> None")},
    {"CGContextAddCurveToPoint", (PyCFunction)CGContextRefObj_CGContextAddCurveToPoint, 1,
     PyDoc_STR("(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) -> None")},
    {"CGContextAddQuadCurveToPoint", (PyCFunction)CGContextRefObj_CGContextAddQuadCurveToPoint, 1,
     PyDoc_STR("(float cpx, float cpy, float x, float y) -> None")},
    {"CGContextClosePath", (PyCFunction)CGContextRefObj_CGContextClosePath, 1,
     PyDoc_STR("() -> None")},
    {"CGContextAddRect", (PyCFunction)CGContextRefObj_CGContextAddRect, 1,
     PyDoc_STR("(CGRect rect) -> None")},
    {"CGContextAddArc", (PyCFunction)CGContextRefObj_CGContextAddArc, 1,
     PyDoc_STR("(float x, float y, float radius, float startAngle, float endAngle, int clockwise) -> None")},
    {"CGContextAddArcToPoint", (PyCFunction)CGContextRefObj_CGContextAddArcToPoint, 1,
     PyDoc_STR("(float x1, float y1, float x2, float y2, float radius) -> None")},
    {"CGContextIsPathEmpty", (PyCFunction)CGContextRefObj_CGContextIsPathEmpty, 1,
     PyDoc_STR("() -> (int _rv)")},
    {"CGContextGetPathCurrentPoint", (PyCFunction)CGContextRefObj_CGContextGetPathCurrentPoint, 1,
     PyDoc_STR("() -> (CGPoint _rv)")},
    {"CGContextGetPathBoundingBox", (PyCFunction)CGContextRefObj_CGContextGetPathBoundingBox, 1,
     PyDoc_STR("() -> (CGRect _rv)")},
    {"CGContextDrawPath", (PyCFunction)CGContextRefObj_CGContextDrawPath, 1,
     PyDoc_STR("(int mode) -> None")},
    {"CGContextFillPath", (PyCFunction)CGContextRefObj_CGContextFillPath, 1,
     PyDoc_STR("() -> None")},
    {"CGContextEOFillPath", (PyCFunction)CGContextRefObj_CGContextEOFillPath, 1,
     PyDoc_STR("() -> None")},
    {"CGContextStrokePath", (PyCFunction)CGContextRefObj_CGContextStrokePath, 1,
     PyDoc_STR("() -> None")},
    {"CGContextFillRect", (PyCFunction)CGContextRefObj_CGContextFillRect, 1,
     PyDoc_STR("(CGRect rect) -> None")},
    {"CGContextStrokeRect", (PyCFunction)CGContextRefObj_CGContextStrokeRect, 1,
     PyDoc_STR("(CGRect rect) -> None")},
    {"CGContextStrokeRectWithWidth", (PyCFunction)CGContextRefObj_CGContextStrokeRectWithWidth, 1,
     PyDoc_STR("(CGRect rect, float width) -> None")},
    {"CGContextClearRect", (PyCFunction)CGContextRefObj_CGContextClearRect, 1,
     PyDoc_STR("(CGRect rect) -> None")},
    {"CGContextClip", (PyCFunction)CGContextRefObj_CGContextClip, 1,
     PyDoc_STR("() -> None")},
    {"CGContextEOClip", (PyCFunction)CGContextRefObj_CGContextEOClip, 1,
     PyDoc_STR("() -> None")},
    {"CGContextClipToRect", (PyCFunction)CGContextRefObj_CGContextClipToRect, 1,
     PyDoc_STR("(CGRect rect) -> None")},
    {"CGContextSetGrayFillColor", (PyCFunction)CGContextRefObj_CGContextSetGrayFillColor, 1,
     PyDoc_STR("(float gray, float alpha) -> None")},
    {"CGContextSetGrayStrokeColor", (PyCFunction)CGContextRefObj_CGContextSetGrayStrokeColor, 1,
     PyDoc_STR("(float gray, float alpha) -> None")},
    {"CGContextSetRGBFillColor", (PyCFunction)CGContextRefObj_CGContextSetRGBFillColor, 1,
     PyDoc_STR("(float red, float green, float blue, float alpha) -> None")},
    {"CGContextSetRGBStrokeColor", (PyCFunction)CGContextRefObj_CGContextSetRGBStrokeColor, 1,
     PyDoc_STR("(float red, float green, float blue, float alpha) -> None")},
    {"CGContextSetCMYKFillColor", (PyCFunction)CGContextRefObj_CGContextSetCMYKFillColor, 1,
     PyDoc_STR("(float cyan, float magenta, float yellow, float black, float alpha) -> None")},
    {"CGContextSetCMYKStrokeColor", (PyCFunction)CGContextRefObj_CGContextSetCMYKStrokeColor, 1,
     PyDoc_STR("(float cyan, float magenta, float yellow, float black, float alpha) -> None")},
    {"CGContextGetInterpolationQuality", (PyCFunction)CGContextRefObj_CGContextGetInterpolationQuality, 1,
     PyDoc_STR("() -> (int _rv)")},
    {"CGContextSetInterpolationQuality", (PyCFunction)CGContextRefObj_CGContextSetInterpolationQuality, 1,
     PyDoc_STR("(int quality) -> None")},
    {"CGContextSetCharacterSpacing", (PyCFunction)CGContextRefObj_CGContextSetCharacterSpacing, 1,
     PyDoc_STR("(float spacing) -> None")},
    {"CGContextSetTextPosition", (PyCFunction)CGContextRefObj_CGContextSetTextPosition, 1,
     PyDoc_STR("(float x, float y) -> None")},
    {"CGContextGetTextPosition", (PyCFunction)CGContextRefObj_CGContextGetTextPosition, 1,
     PyDoc_STR("() -> (CGPoint _rv)")},
    {"CGContextSetTextMatrix", (PyCFunction)CGContextRefObj_CGContextSetTextMatrix, 1,
     PyDoc_STR("(CGAffineTransform transform) -> None")},
    {"CGContextGetTextMatrix", (PyCFunction)CGContextRefObj_CGContextGetTextMatrix, 1,
     PyDoc_STR("() -> (CGAffineTransform _rv)")},
    {"CGContextSetTextDrawingMode", (PyCFunction)CGContextRefObj_CGContextSetTextDrawingMode, 1,
     PyDoc_STR("(int mode) -> None")},
    {"CGContextSetFontSize", (PyCFunction)CGContextRefObj_CGContextSetFontSize, 1,
     PyDoc_STR("(float size) -> None")},
    {"CGContextSelectFont", (PyCFunction)CGContextRefObj_CGContextSelectFont, 1,
     PyDoc_STR("(char * name, float size, int textEncoding) -> None")},
    {"CGContextShowText", (PyCFunction)CGContextRefObj_CGContextShowText, 1,
     PyDoc_STR("(Buffer cstring) -> None")},
    {"CGContextShowTextAtPoint", (PyCFunction)CGContextRefObj_CGContextShowTextAtPoint, 1,
     PyDoc_STR("(float x, float y, Buffer cstring) -> None")},
    {"CGContextEndPage", (PyCFunction)CGContextRefObj_CGContextEndPage, 1,
     PyDoc_STR("() -> None")},
    {"CGContextFlush", (PyCFunction)CGContextRefObj_CGContextFlush, 1,
     PyDoc_STR("() -> None")},
    {"CGContextSynchronize", (PyCFunction)CGContextRefObj_CGContextSynchronize, 1,
     PyDoc_STR("() -> None")},
    {"CGContextSetShouldAntialias", (PyCFunction)CGContextRefObj_CGContextSetShouldAntialias, 1,
     PyDoc_STR("(int shouldAntialias) -> None")},
#ifndef __LP64__
    {"SyncCGContextOriginWithPort", (PyCFunction)CGContextRefObj_SyncCGContextOriginWithPort, 1,
     PyDoc_STR("(CGrafPtr port) -> None")},
    {"ClipCGContextToRegion", (PyCFunction)CGContextRefObj_ClipCGContextToRegion, 1,
     PyDoc_STR("(Rect portRect, RgnHandle region) -> None")},
#endif
    {NULL, NULL, 0}
};

#define CGContextRefObj_getsetlist NULL


#define CGContextRefObj_compare NULL

#define CGContextRefObj_repr NULL

#define CGContextRefObj_hash NULL
#define CGContextRefObj_tp_init 0

#define CGContextRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CGContextRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    CGContextRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CGContextRefObj_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CGContextRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define CGContextRefObj_tp_free PyObject_Del


PyTypeObject CGContextRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CG.CGContextRef", /*tp_name*/
    sizeof(CGContextRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CGContextRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CGContextRefObj_compare, /*tp_compare*/
    (reprfunc) CGContextRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CGContextRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CGContextRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CGContextRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CGContextRefObj_tp_init, /* tp_init */
    CGContextRefObj_tp_alloc, /* tp_alloc */
    CGContextRefObj_tp_new, /* tp_new */
    CGContextRefObj_tp_free, /* tp_free */
};

/* ------------------ End object type CGContextRef ------------------ */


#ifndef __LP64__
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
#endif

static PyMethodDef CG_methods[] = {
#ifndef __LP64__
    {"CreateCGContextForPort", (PyCFunction)CG_CreateCGContextForPort, 1,
     PyDoc_STR("(CGrafPtr) -> CGContextRef")},
#endif
    {NULL, NULL, 0}
};




void init_CG(void)
{
    PyObject *m;
    PyObject *d;




    m = Py_InitModule("_CG", CG_methods);
    d = PyModule_GetDict(m);
    CG_Error = PyMac_GetOSErrException();
    if (CG_Error == NULL ||
        PyDict_SetItemString(d, "Error", CG_Error) != 0)
        return;
    CGContextRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&CGContextRef_Type) < 0) return;
    Py_INCREF(&CGContextRef_Type);
    PyModule_AddObject(m, "CGContextRef", (PyObject *)&CGContextRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CGContextRef_Type);
    PyModule_AddObject(m, "CGContextRefType", (PyObject *)&CGContextRef_Type);
}

/* ========================= End module _CG ========================= */

