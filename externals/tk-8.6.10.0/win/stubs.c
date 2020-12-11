#include "tk.h"

/*
 * Undocumented Xlib internal function
 */

int
_XInitImageFuncPtrs(
    XImage *image)
{
    return Success;
}

/*
 * From Xutil.h
 */

void
XSetWMClientMachine(
    Display *display,
    Window w,
    XTextProperty *text_prop)
{
}

Status
XStringListToTextProperty(
    char **list,
    int count,
    XTextProperty *text_prop_return)
{
    return (Status) 0;
}

/*
 * From Xlib.h
 */

int
XChangeProperty(
    Display *display,
    Window w,
    Atom property,
    Atom type,
    int format,
    int mode,
    _Xconst unsigned char *data,
    int nelements)
{
    return Success;
}

Cursor
XCreateGlyphCursor(
    Display *display,
    Font source_font,
    Font mask_font,
    unsigned int source_char,
    unsigned int mask_char,
    XColor _Xconst *foreground_color,
    XColor _Xconst *background_color)
{
    return 1;
}

XIC
XCreateIC(XIM xim, ...)
{
    return NULL;
}

Cursor
XCreatePixmapCursor(
    Display *display,
    Pixmap source,
    Pixmap mask,
    XColor *foreground_color,
    XColor *background_color,
    unsigned int x,
    unsigned int y)
{
    return (Cursor) NULL;
}

int
XDeleteProperty(
    Display *display,
    Window w,
    Atom property)
{
    return Success;
}

void
XDestroyIC(
    XIC ic)
{
}

Bool
XFilterEvent(
    XEvent *event,
    Window window)
{
    return 0;
}

int
XForceScreenSaver(
    Display *display,
    int mode)
{
    return Success;
}

int
XFreeCursor(
    Display *display,
    Cursor cursor)
{
    return Success;
}

GContext
XGContextFromGC(
    GC gc)
{
    return (GContext) NULL;
}

char *
XGetAtomName(
    Display *display,
    Atom atom)
{
    return NULL;
}

int
XGetWindowAttributes(
    Display *display,
    Window w,
    XWindowAttributes *window_attributes_return)
{
    return Success;
}

Status
XGetWMColormapWindows(
    Display *display,
    Window w,
    Window **windows_return,
    int *count_return)
{
    return (Status) 0;
}

int
XIconifyWindow(
    Display *display,
    Window w,
    int screen_number)
{
    return Success;
}

XHostAddress *
XListHosts(
    Display *display,
    int *nhosts_return,
    Bool *state_return)
{
    return NULL;
}

int
XLookupColor(
    Display *display,
    Colormap colormap,
    _Xconst char *color_name,
    XColor *exact_def_return,
    XColor *screen_def_return)
{
    return Success;
}

int
XNextEvent(
    Display *display,
    XEvent *event_return)
{
    return Success;
}

int
XPutBackEvent(
    Display *display,
    XEvent *event)
{
    return Success;
}

int
XQueryColors(
    Display *display,
    Colormap colormap,
    XColor *defs_in_out,
    int ncolors)
{
    return Success;
}

int
XQueryTree(
    Display *display,
    Window w,
    Window *root_return,
    Window *parent_return,
    Window **children_return,
    unsigned int *nchildren_return)
{
    return Success;
}

int
XRefreshKeyboardMapping(
    XMappingEvent *event_map)
{
    return Success;
}

Window
XRootWindow(
    Display *display,
    int screen_number)
{
    return (Window) NULL;
}

int
XSelectInput(
    Display *display,
    Window w,
    long event_mask)
{
    return Success;
}

int
XSendEvent(
    Display *display,
    Window w,
    Bool propagate,
    long event_mask,
    XEvent *event_send)
{
    return Success;
}

int
XSetCommand(
    Display *display,
    Window w,
    char **argv,
    int argc)
{
    return Success;
}

XErrorHandler
XSetErrorHandler(
    XErrorHandler handler)
{
    return NULL;
}

int
XSetIconName(
    Display *display,
    Window w,
    _Xconst char *icon_name)
{
    return Success;
}

int
XSetWindowBackground(
    Display *display,
    Window w,
    unsigned long background_pixel)
{
    return Success;
}

int
XSetWindowBackgroundPixmap(
    Display *display,
    Window w,
    Pixmap background_pixmap)
{
    return Success;
}

int
XSetWindowBorder(
    Display *display,
    Window w,
    unsigned long border_pixel)
{
    return Success;
}

int
XSetWindowBorderPixmap(
    Display *display,
    Window w,
    Pixmap border_pixmap)
{
    return Success;
}

int
XSetWindowBorderWidth(
    Display *display,
    Window w,
    unsigned int width)
{
    return Success;
}

int
XSetWindowColormap(
    Display *display,
    Window w,
    Colormap colormap)
{
    return Success;
}

Bool
XTranslateCoordinates(
    Display *display,
    Window src_w,
    Window dest_w,
    int src_x,
    int src_y,
    int *dest_x_return,
    int *dest_y_return,
    Window *child_return)
{
    return 0;
}

int
XWindowEvent(
    Display *display,
    Window w,
    long event_mask,
    XEvent *event_return)
{
    return Success;
}

int
XWithdrawWindow(
    Display *display,
    Window w,
    int screen_number)
{
    return Success;
}

int
XmbLookupString(
    XIC ic,
    XKeyPressedEvent *event,
    char *buffer_return,
    int bytes_buffer,
    KeySym *keysym_return,
    Status *status_return)
{
    return Success;
}

int
XGetWindowProperty(
    Display *display,
    Window w,
    Atom property,
    long long_offset,
    long long_length,
    Bool delete,
    Atom req_type,
    Atom *actual_type_return,
    int *actual_format_return,
    unsigned long *nitems_return,
    unsigned long *bytes_after_return,
    unsigned char **prop_return)
{
    *actual_type_return = None;
    *actual_format_return = 0;
    *nitems_return = 0;
    *bytes_after_return = 0;
    *prop_return = NULL;
    return BadValue;
}

/*
 * The following functions were implemented as macros under Windows.
 */

int
XFlush(
    Display *display)
{
    return 0;
}

int
XGrabServer(
    Display *display)
{
    return 0;
}

int
XUngrabServer(
    Display *display)
{
    return 0;
}

int
XFree(
    void *data)
{
	if ((data) != NULL) {
		ckfree(data);
	}
    return 0;
}

int
XNoOp(
    Display *display)
{
    display->request++;
    return 0;
}

XAfterFunction
XSynchronize(
    Display *display,
    Bool onoff)
{
    display->request++;
    return NULL;
}

int
XSync(
    Display *display,
    Bool discard)
{
    display->request++;
    return 0;
}

VisualID
XVisualIDFromVisual(
    Visual *visual)
{
    return visual->visualid;
}
