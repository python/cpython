# tk.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Tk library via the stubs table.
#	This file is used to generate the tkDecls.h, tkPlatDecls.h,
#	tkStub.c, and tkPlatStub.c files.
#
# Copyright (c) 1998-2000 Ajuba Solutions.
# Copyright (c) 2007 Daniel A. Steffen <das@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

library tk

# Define the tk interface with 3 sub interfaces:
#     tkPlat	 - platform specific public
#     tkInt	 - generic private
#     tkPlatInt - platform specific private

interface tk
hooks {tkPlat tkInt tkIntPlat tkIntXlib}
scspec EXTERN

# Declare each of the functions in the public Tk interface.  Note that
# the an index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 {
    void Tk_MainLoop(void)
}
declare 1 {
    XColor *Tk_3DBorderColor(Tk_3DBorder border)
}
declare 2 {
    GC Tk_3DBorderGC(Tk_Window tkwin, Tk_3DBorder border,
	    int which)
}
declare 3 {
    void Tk_3DHorizontalBevel(Tk_Window tkwin,
	    Drawable drawable, Tk_3DBorder border, int x,
	    int y, int width, int height, int leftIn,
	    int rightIn, int topBevel, int relief)
}
declare 4 {
    void Tk_3DVerticalBevel(Tk_Window tkwin,
	    Drawable drawable, Tk_3DBorder border, int x,
	    int y, int width, int height, int leftBevel,
	    int relief)
}
declare 5 {
    void Tk_AddOption(Tk_Window tkwin, const char *name,
	    const char *value, int priority)
}
declare 6 {
    void Tk_BindEvent(Tk_BindingTable bindingTable,
	    XEvent *eventPtr, Tk_Window tkwin, int numObjects,
	    ClientData *objectPtr)
}
declare 7 {
    void Tk_CanvasDrawableCoords(Tk_Canvas canvas,
	    double x, double y, short *drawableXPtr,
	    short *drawableYPtr)
}
declare 8 {
    void Tk_CanvasEventuallyRedraw(Tk_Canvas canvas, int x1, int y1,
	    int x2, int y2)
}
declare 9 {
    int Tk_CanvasGetCoord(Tcl_Interp *interp,
	    Tk_Canvas canvas, const char *str, double *doublePtr)
}
declare 10 {
    Tk_CanvasTextInfo *Tk_CanvasGetTextInfo(Tk_Canvas canvas)
}
declare 11 {
    int Tk_CanvasPsBitmap(Tcl_Interp *interp,
	    Tk_Canvas canvas, Pixmap bitmap, int x, int y,
	    int width, int height)
}
declare 12 {
    int Tk_CanvasPsColor(Tcl_Interp *interp,
	    Tk_Canvas canvas, XColor *colorPtr)
}
declare 13 {
    int Tk_CanvasPsFont(Tcl_Interp *interp,
	    Tk_Canvas canvas, Tk_Font font)
}
declare 14 {
    void Tk_CanvasPsPath(Tcl_Interp *interp,
	    Tk_Canvas canvas, double *coordPtr, int numPoints)
}
declare 15 {
    int Tk_CanvasPsStipple(Tcl_Interp *interp,
	    Tk_Canvas canvas, Pixmap bitmap)
}
declare 16 {
    double Tk_CanvasPsY(Tk_Canvas canvas, double y)
}
declare 17 {
    void Tk_CanvasSetStippleOrigin(Tk_Canvas canvas, GC gc)
}
declare 18 {
    int Tk_CanvasTagsParseProc(ClientData clientData, Tcl_Interp *interp,
	    Tk_Window tkwin, const char *value, char *widgRec, int offset)
}
declare 19 {
    CONST86 char *Tk_CanvasTagsPrintProc(ClientData clientData, Tk_Window tkwin,
	    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
}
declare 20 {
    Tk_Window	Tk_CanvasTkwin(Tk_Canvas canvas)
}
declare 21 {
    void Tk_CanvasWindowCoords(Tk_Canvas canvas, double x, double y,
	    short *screenXPtr, short *screenYPtr)
}
declare 22 {
    void Tk_ChangeWindowAttributes(Tk_Window tkwin, unsigned long valueMask,
	    XSetWindowAttributes *attsPtr)
}
declare 23 {
    int Tk_CharBbox(Tk_TextLayout layout, int index, int *xPtr,
	    int *yPtr, int *widthPtr, int *heightPtr)
}
declare 24 {
    void Tk_ClearSelection(Tk_Window tkwin, Atom selection)
}
declare 25 {
    int Tk_ClipboardAppend(Tcl_Interp *interp, Tk_Window tkwin,
	    Atom target, Atom format, const char *buffer)
}
declare 26 {
    int Tk_ClipboardClear(Tcl_Interp *interp, Tk_Window tkwin)
}
declare 27 {
    int Tk_ConfigureInfo(Tcl_Interp *interp,
	    Tk_Window tkwin, const Tk_ConfigSpec *specs,
	    char *widgRec, const char *argvName, int flags)
}
declare 28 {
    int Tk_ConfigureValue(Tcl_Interp *interp,
	    Tk_Window tkwin, const Tk_ConfigSpec *specs,
	    char *widgRec, const char *argvName, int flags)
}
declare 29 {
    int Tk_ConfigureWidget(Tcl_Interp *interp,
	    Tk_Window tkwin, const Tk_ConfigSpec *specs,
	    int argc, CONST84 char **argv, char *widgRec,
	    int flags)
}
declare 30 {
    void Tk_ConfigureWindow(Tk_Window tkwin,
	    unsigned int valueMask, XWindowChanges *valuePtr)
}
declare 31 {
    Tk_TextLayout Tk_ComputeTextLayout(Tk_Font font,
	    const char *str, int numChars, int wrapLength,
	    Tk_Justify justify, int flags, int *widthPtr,
	    int *heightPtr)
}
declare 32 {
    Tk_Window Tk_CoordsToWindow(int rootX, int rootY, Tk_Window tkwin)
}
declare 33 {
    unsigned long Tk_CreateBinding(Tcl_Interp *interp,
	    Tk_BindingTable bindingTable, ClientData object,
	    const char *eventStr, const char *script, int append)
}
declare 34 {
    Tk_BindingTable Tk_CreateBindingTable(Tcl_Interp *interp)
}
declare 35 {
    Tk_ErrorHandler Tk_CreateErrorHandler(Display *display,
	    int errNum, int request, int minorCode,
	    Tk_ErrorProc *errorProc, ClientData clientData)
}
declare 36 {
    void Tk_CreateEventHandler(Tk_Window token,
	    unsigned long mask, Tk_EventProc *proc,
	    ClientData clientData)
}
declare 37 {
    void Tk_CreateGenericHandler(Tk_GenericProc *proc, ClientData clientData)
}
declare 38 {
    void Tk_CreateImageType(const Tk_ImageType *typePtr)
}
declare 39 {
    void Tk_CreateItemType(Tk_ItemType *typePtr)
}
declare 40 {
    void Tk_CreatePhotoImageFormat(const Tk_PhotoImageFormat *formatPtr)
}
declare 41 {
    void Tk_CreateSelHandler(Tk_Window tkwin,
	    Atom selection, Atom target,
	    Tk_SelectionProc *proc, ClientData clientData,
	    Atom format)
}
declare 42 {
    Tk_Window Tk_CreateWindow(Tcl_Interp *interp,
	    Tk_Window parent, const char *name, const char *screenName)
}
declare 43 {
    Tk_Window Tk_CreateWindowFromPath(Tcl_Interp *interp, Tk_Window tkwin,
	    const char *pathName, const char *screenName)
}
declare 44 {
    int Tk_DefineBitmap(Tcl_Interp *interp, const char *name,
	    const void *source, int width, int height)
}
declare 45 {
    void Tk_DefineCursor(Tk_Window window, Tk_Cursor cursor)
}
declare 46 {
    void Tk_DeleteAllBindings(Tk_BindingTable bindingTable, ClientData object)
}
declare 47 {
    int Tk_DeleteBinding(Tcl_Interp *interp,
	    Tk_BindingTable bindingTable, ClientData object,
	    const char *eventStr)
}
declare 48 {
    void Tk_DeleteBindingTable(Tk_BindingTable bindingTable)
}
declare 49 {
    void Tk_DeleteErrorHandler(Tk_ErrorHandler handler)
}
declare 50 {
    void Tk_DeleteEventHandler(Tk_Window token,
	    unsigned long mask, Tk_EventProc *proc,
	    ClientData clientData)
}
declare 51 {
    void Tk_DeleteGenericHandler(Tk_GenericProc *proc, ClientData clientData)
}
declare 52 {
    void Tk_DeleteImage(Tcl_Interp *interp, const char *name)
}
declare 53 {
    void Tk_DeleteSelHandler(Tk_Window tkwin, Atom selection, Atom target)
}
declare 54 {
    void Tk_DestroyWindow(Tk_Window tkwin)
}
declare 55 {
    CONST84_RETURN char *Tk_DisplayName(Tk_Window tkwin)
}
declare 56 {
    int Tk_DistanceToTextLayout(Tk_TextLayout layout, int x, int y)
}
declare 57 {
    void Tk_Draw3DPolygon(Tk_Window tkwin,
	    Drawable drawable, Tk_3DBorder border,
	    XPoint *pointPtr, int numPoints, int borderWidth,
	    int leftRelief)
}
declare 58 {
    void Tk_Draw3DRectangle(Tk_Window tkwin, Drawable drawable,
	    Tk_3DBorder border, int x, int y, int width, int height,
	    int borderWidth, int relief)
}
declare 59 {
    void Tk_DrawChars(Display *display, Drawable drawable, GC gc,
	    Tk_Font tkfont, const char *source, int numBytes, int x, int y)
}
declare 60 {
    void Tk_DrawFocusHighlight(Tk_Window tkwin, GC gc, int width,
	    Drawable drawable)
}
declare 61 {
    void Tk_DrawTextLayout(Display *display,
	    Drawable drawable, GC gc, Tk_TextLayout layout,
	    int x, int y, int firstChar, int lastChar)
}
declare 62 {
    void Tk_Fill3DPolygon(Tk_Window tkwin,
	    Drawable drawable, Tk_3DBorder border,
	    XPoint *pointPtr, int numPoints, int borderWidth,
	    int leftRelief)
}
declare 63 {
    void Tk_Fill3DRectangle(Tk_Window tkwin,
	    Drawable drawable, Tk_3DBorder border, int x,
	    int y, int width, int height, int borderWidth,
	    int relief)
}
declare 64 {
    Tk_PhotoHandle Tk_FindPhoto(Tcl_Interp *interp, const char *imageName)
}
declare 65 {
    Font Tk_FontId(Tk_Font font)
}
declare 66 {
    void Tk_Free3DBorder(Tk_3DBorder border)
}
declare 67 {
    void Tk_FreeBitmap(Display *display, Pixmap bitmap)
}
declare 68 {
    void Tk_FreeColor(XColor *colorPtr)
}
declare 69 {
    void Tk_FreeColormap(Display *display, Colormap colormap)
}
declare 70 {
    void Tk_FreeCursor(Display *display, Tk_Cursor cursor)
}
declare 71 {
    void Tk_FreeFont(Tk_Font f)
}
declare 72 {
    void Tk_FreeGC(Display *display, GC gc)
}
declare 73 {
    void Tk_FreeImage(Tk_Image image)
}
declare 74 {
    void Tk_FreeOptions(const Tk_ConfigSpec *specs,
	    char *widgRec, Display *display, int needFlags)
}
declare 75 {
    void Tk_FreePixmap(Display *display, Pixmap pixmap)
}
declare 76 {
    void Tk_FreeTextLayout(Tk_TextLayout textLayout)
}
declare 77 {
    void Tk_FreeXId(Display *display, XID xid)
}
declare 78 {
    GC Tk_GCForColor(XColor *colorPtr, Drawable drawable)
}
declare 79 {
    void Tk_GeometryRequest(Tk_Window tkwin, int reqWidth,  int reqHeight)
}
declare 80 {
    Tk_3DBorder	Tk_Get3DBorder(Tcl_Interp *interp, Tk_Window tkwin,
	    Tk_Uid colorName)
}
declare 81 {
    void Tk_GetAllBindings(Tcl_Interp *interp,
	    Tk_BindingTable bindingTable, ClientData object)
}
declare 82 {
    int Tk_GetAnchor(Tcl_Interp *interp,
	    const char *str, Tk_Anchor *anchorPtr)
}
declare 83 {
    CONST84_RETURN char *Tk_GetAtomName(Tk_Window tkwin, Atom atom)
}
declare 84 {
    CONST84_RETURN char *Tk_GetBinding(Tcl_Interp *interp,
	    Tk_BindingTable bindingTable, ClientData object,
	    const char *eventStr)
}
declare 85 {
    Pixmap Tk_GetBitmap(Tcl_Interp *interp, Tk_Window tkwin, const char *str)
}
declare 86 {
    Pixmap Tk_GetBitmapFromData(Tcl_Interp *interp,
	    Tk_Window tkwin, const void *source, int width, int height)
}
declare 87 {
    int Tk_GetCapStyle(Tcl_Interp *interp, const char *str, int *capPtr)
}
declare 88 {
    XColor *Tk_GetColor(Tcl_Interp *interp, Tk_Window tkwin, Tk_Uid name)
}
declare 89 {
    XColor *Tk_GetColorByValue(Tk_Window tkwin, XColor *colorPtr)
}
declare 90 {
    Colormap Tk_GetColormap(Tcl_Interp *interp, Tk_Window tkwin,
	    const char *str)
}
declare 91 {
    Tk_Cursor Tk_GetCursor(Tcl_Interp *interp, Tk_Window tkwin,
	    Tk_Uid str)
}
declare 92 {
    Tk_Cursor Tk_GetCursorFromData(Tcl_Interp *interp,
	    Tk_Window tkwin, const char *source, const char *mask,
	    int width, int height, int xHot, int yHot,
	    Tk_Uid fg, Tk_Uid bg)
}
declare 93 {
    Tk_Font Tk_GetFont(Tcl_Interp *interp,
	    Tk_Window tkwin, const char *str)
}
declare 94 {
    Tk_Font Tk_GetFontFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 95 {
    void Tk_GetFontMetrics(Tk_Font font, Tk_FontMetrics *fmPtr)
}
declare 96 {
    GC Tk_GetGC(Tk_Window tkwin, unsigned long valueMask, XGCValues *valuePtr)
}
declare 97 {
    Tk_Image Tk_GetImage(Tcl_Interp *interp, Tk_Window tkwin, const char *name,
	    Tk_ImageChangedProc *changeProc, ClientData clientData)
}
declare 98 {
    ClientData Tk_GetImageMasterData(Tcl_Interp *interp,
	    const char *name, CONST86 Tk_ImageType **typePtrPtr)
}
declare 99 {
    Tk_ItemType *Tk_GetItemTypes(void)
}
declare 100 {
    int Tk_GetJoinStyle(Tcl_Interp *interp, const char *str, int *joinPtr)
}
declare 101 {
    int Tk_GetJustify(Tcl_Interp *interp,
	    const char *str, Tk_Justify *justifyPtr)
}
declare 102 {
    int Tk_GetNumMainWindows(void)
}
declare 103 {
    Tk_Uid Tk_GetOption(Tk_Window tkwin, const char *name,
	    const char *className)
}
declare 104 {
    int Tk_GetPixels(Tcl_Interp *interp,
	    Tk_Window tkwin, const char *str, int *intPtr)
}
declare 105 {
    Pixmap Tk_GetPixmap(Display *display, Drawable d,
	    int width, int height, int depth)
}
declare 106 {
    int Tk_GetRelief(Tcl_Interp *interp, const char *name, int *reliefPtr)
}
declare 107 {
    void Tk_GetRootCoords(Tk_Window tkwin, int *xPtr, int *yPtr)
}
declare 108 {
    int Tk_GetScrollInfo(Tcl_Interp *interp,
	    int argc, CONST84 char **argv, double *dblPtr, int *intPtr)
}
declare 109 {
    int Tk_GetScreenMM(Tcl_Interp *interp,
	    Tk_Window tkwin, const char *str, double *doublePtr)
}
declare 110 {
    int Tk_GetSelection(Tcl_Interp *interp,
	    Tk_Window tkwin, Atom selection, Atom target,
	    Tk_GetSelProc *proc, ClientData clientData)
}
declare 111 {
    Tk_Uid Tk_GetUid(const char *str)
}
declare 112 {
    Visual *Tk_GetVisual(Tcl_Interp *interp,
	    Tk_Window tkwin, const char *str, int *depthPtr,
	    Colormap *colormapPtr)
}
declare 113 {
    void Tk_GetVRootGeometry(Tk_Window tkwin,
	    int *xPtr, int *yPtr, int *widthPtr, int *heightPtr)
}
declare 114 {
    int Tk_Grab(Tcl_Interp *interp, Tk_Window tkwin, int grabGlobal)
}
declare 115 {
    void Tk_HandleEvent(XEvent *eventPtr)
}
declare 116 {
    Tk_Window Tk_IdToWindow(Display *display, Window window)
}
declare 117 {
    void Tk_ImageChanged(Tk_ImageMaster master, int x, int y,
	    int width, int height, int imageWidth, int imageHeight)
}
declare 118 {
    int Tk_Init(Tcl_Interp *interp)
}
declare 119 {
    Atom Tk_InternAtom(Tk_Window tkwin, const char *name)
}
declare 120 {
    int Tk_IntersectTextLayout(Tk_TextLayout layout, int x, int y,
	    int width, int height)
}
declare 121 {
    void Tk_MaintainGeometry(Tk_Window slave,
	    Tk_Window master, int x, int y, int width, int height)
}
declare 122 {
    Tk_Window Tk_MainWindow(Tcl_Interp *interp)
}
declare 123 {
    void Tk_MakeWindowExist(Tk_Window tkwin)
}
declare 124 {
    void Tk_ManageGeometry(Tk_Window tkwin,
	    const Tk_GeomMgr *mgrPtr, ClientData clientData)
}
declare 125 {
    void Tk_MapWindow(Tk_Window tkwin)
}
declare 126 {
    int Tk_MeasureChars(Tk_Font tkfont,
	    const char *source, int numBytes, int maxPixels,
	    int flags, int *lengthPtr)
}
declare 127 {
    void Tk_MoveResizeWindow(Tk_Window tkwin,
	    int x, int y, int width, int height)
}
declare 128 {
    void Tk_MoveWindow(Tk_Window tkwin, int x, int y)
}
declare 129 {
    void Tk_MoveToplevelWindow(Tk_Window tkwin, int x, int y)
}
declare 130 {
    CONST84_RETURN char *Tk_NameOf3DBorder(Tk_3DBorder border)
}
declare 131 {
    CONST84_RETURN char *Tk_NameOfAnchor(Tk_Anchor anchor)
}
declare 132 {
    CONST84_RETURN char *Tk_NameOfBitmap(Display *display, Pixmap bitmap)
}
declare 133 {
    CONST84_RETURN char *Tk_NameOfCapStyle(int cap)
}
declare 134 {
    CONST84_RETURN char *Tk_NameOfColor(XColor *colorPtr)
}
declare 135 {
    CONST84_RETURN char *Tk_NameOfCursor(Display *display, Tk_Cursor cursor)
}
declare 136 {
    CONST84_RETURN char *Tk_NameOfFont(Tk_Font font)
}
declare 137 {
    CONST84_RETURN char *Tk_NameOfImage(Tk_ImageMaster imageMaster)
}
declare 138 {
    CONST84_RETURN char *Tk_NameOfJoinStyle(int join)
}
declare 139 {
    CONST84_RETURN char *Tk_NameOfJustify(Tk_Justify justify)
}
declare 140 {
    CONST84_RETURN char *Tk_NameOfRelief(int relief)
}
declare 141 {
    Tk_Window Tk_NameToWindow(Tcl_Interp *interp,
	    const char *pathName, Tk_Window tkwin)
}
declare 142 {
    void Tk_OwnSelection(Tk_Window tkwin,
	    Atom selection, Tk_LostSelProc *proc,
	    ClientData clientData)
}
declare 143 {
    int Tk_ParseArgv(Tcl_Interp *interp,
	    Tk_Window tkwin, int *argcPtr, CONST84 char **argv,
	    const Tk_ArgvInfo *argTable, int flags)
}
declare 144 {
    void Tk_PhotoPutBlock_NoComposite(Tk_PhotoHandle handle,
	    Tk_PhotoImageBlock *blockPtr, int x, int y,
	    int width, int height)
}
declare 145 {
    void Tk_PhotoPutZoomedBlock_NoComposite(Tk_PhotoHandle handle,
	    Tk_PhotoImageBlock *blockPtr, int x, int y,
	    int width, int height, int zoomX, int zoomY,
	    int subsampleX, int subsampleY)
}
declare 146 {
    int Tk_PhotoGetImage(Tk_PhotoHandle handle, Tk_PhotoImageBlock *blockPtr)
}
declare 147 {
    void Tk_PhotoBlank(Tk_PhotoHandle handle)
}
declare 148 {
    void Tk_PhotoExpand_Panic(Tk_PhotoHandle handle, int width, int height )
}
declare 149 {
    void Tk_PhotoGetSize(Tk_PhotoHandle handle, int *widthPtr, int *heightPtr)
}
declare 150 {
    void Tk_PhotoSetSize_Panic(Tk_PhotoHandle handle, int width, int height)
}
declare 151 {
    int Tk_PointToChar(Tk_TextLayout layout, int x, int y)
}
declare 152 {
    int Tk_PostscriptFontName(Tk_Font tkfont, Tcl_DString *dsPtr)
}
declare 153 {
    void Tk_PreserveColormap(Display *display, Colormap colormap)
}
declare 154 {
    void Tk_QueueWindowEvent(XEvent *eventPtr, Tcl_QueuePosition position)
}
declare 155 {
    void Tk_RedrawImage(Tk_Image image, int imageX,
	    int imageY, int width, int height,
	    Drawable drawable, int drawableX, int drawableY)
}
declare 156 {
    void Tk_ResizeWindow(Tk_Window tkwin, int width, int height)
}
declare 157 {
    int Tk_RestackWindow(Tk_Window tkwin, int aboveBelow, Tk_Window other)
}
declare 158 {
    Tk_RestrictProc *Tk_RestrictEvents(Tk_RestrictProc *proc,
	    ClientData arg, ClientData *prevArgPtr)
}
declare 159 {
    int Tk_SafeInit(Tcl_Interp *interp)
}
declare 160 {
    const char *Tk_SetAppName(Tk_Window tkwin, const char *name)
}
declare 161 {
    void Tk_SetBackgroundFromBorder(Tk_Window tkwin, Tk_3DBorder border)
}
declare 162 {
    void Tk_SetClass(Tk_Window tkwin, const char *className)
}
declare 163 {
    void Tk_SetGrid(Tk_Window tkwin, int reqWidth, int reqHeight,
	    int gridWidth, int gridHeight)
}
declare 164 {
    void Tk_SetInternalBorder(Tk_Window tkwin, int width)
}
declare 165 {
    void Tk_SetWindowBackground(Tk_Window tkwin, unsigned long pixel)
}
declare 166 {
    void Tk_SetWindowBackgroundPixmap(Tk_Window tkwin, Pixmap pixmap)
}
declare 167 {
    void Tk_SetWindowBorder(Tk_Window tkwin, unsigned long pixel)
}
declare 168 {
    void Tk_SetWindowBorderWidth(Tk_Window tkwin, int width)
}
declare 169 {
    void Tk_SetWindowBorderPixmap(Tk_Window tkwin, Pixmap pixmap)
}
declare 170 {
    void Tk_SetWindowColormap(Tk_Window tkwin, Colormap colormap)
}
declare 171 {
    int Tk_SetWindowVisual(Tk_Window tkwin, Visual *visual, int depth,
	    Colormap colormap)
}
declare 172 {
    void Tk_SizeOfBitmap(Display *display, Pixmap bitmap, int *widthPtr,
	    int *heightPtr)
}
declare 173 {
    void Tk_SizeOfImage(Tk_Image image, int *widthPtr, int *heightPtr)
}
declare 174 {
    int Tk_StrictMotif(Tk_Window tkwin)
}
declare 175 {
    void Tk_TextLayoutToPostscript(Tcl_Interp *interp, Tk_TextLayout layout)
}
declare 176 {
    int Tk_TextWidth(Tk_Font font, const char *str, int numBytes)
}
declare 177 {
    void Tk_UndefineCursor(Tk_Window window)
}
declare 178 {
    void Tk_UnderlineChars(Display *display,
	    Drawable drawable, GC gc, Tk_Font tkfont,
	    const char *source, int x, int y, int firstByte,
	    int lastByte)
}
declare 179 {
    void Tk_UnderlineTextLayout(Display *display, Drawable drawable, GC gc,
	    Tk_TextLayout layout, int x, int y,
	    int underline)
}
declare 180 {
    void Tk_Ungrab(Tk_Window tkwin)
}
declare 181 {
    void Tk_UnmaintainGeometry(Tk_Window slave, Tk_Window master)
}
declare 182 {
    void Tk_UnmapWindow(Tk_Window tkwin)
}
declare 183 {
    void Tk_UnsetGrid(Tk_Window tkwin)
}
declare 184 {
    void Tk_UpdatePointer(Tk_Window tkwin, int x, int y, int state)
}

# new functions for 8.1

declare 185 {
    Pixmap  Tk_AllocBitmapFromObj(Tcl_Interp *interp, Tk_Window tkwin,
    Tcl_Obj *objPtr)
}
declare 186 {
    Tk_3DBorder Tk_Alloc3DBorderFromObj(Tcl_Interp *interp, Tk_Window tkwin,
	    Tcl_Obj *objPtr)
}
declare 187 {
    XColor *Tk_AllocColorFromObj(Tcl_Interp *interp, Tk_Window tkwin,
	    Tcl_Obj *objPtr)
}
declare 188 {
    Tk_Cursor Tk_AllocCursorFromObj(Tcl_Interp *interp, Tk_Window tkwin,
	    Tcl_Obj *objPtr)
}
declare 189 {
    Tk_Font  Tk_AllocFontFromObj(Tcl_Interp *interp, Tk_Window tkwin,
	    Tcl_Obj *objPtr)

}
declare 190 {
    Tk_OptionTable Tk_CreateOptionTable(Tcl_Interp *interp,
	    const Tk_OptionSpec *templatePtr)
}
declare 191 {
    void  Tk_DeleteOptionTable(Tk_OptionTable optionTable)
}
declare 192 {
    void  Tk_Free3DBorderFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 193 {
    void  Tk_FreeBitmapFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 194 {
    void  Tk_FreeColorFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 195 {
    void  Tk_FreeConfigOptions(char *recordPtr, Tk_OptionTable optionToken,
	    Tk_Window tkwin)
}
declare 196 {
    void  Tk_FreeSavedOptions(Tk_SavedOptions *savePtr)
}
declare 197 {
    void  Tk_FreeCursorFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 198 {
    void  Tk_FreeFontFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 199 {
    Tk_3DBorder Tk_Get3DBorderFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 200 {
    int	 Tk_GetAnchorFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    Tk_Anchor *anchorPtr)
}
declare 201 {
    Pixmap  Tk_GetBitmapFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 202 {
    XColor *Tk_GetColorFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 203 {
    Tk_Cursor Tk_GetCursorFromObj(Tk_Window tkwin, Tcl_Obj *objPtr)
}
declare 204 {
    Tcl_Obj *Tk_GetOptionInfo(Tcl_Interp *interp,
	    char *recordPtr, Tk_OptionTable optionTable,
	    Tcl_Obj *namePtr, Tk_Window tkwin)
}
declare 205 {
    Tcl_Obj *Tk_GetOptionValue(Tcl_Interp *interp, char *recordPtr,
	    Tk_OptionTable optionTable, Tcl_Obj *namePtr, Tk_Window tkwin)
}
declare 206 {
    int	 Tk_GetJustifyFromObj(Tcl_Interp *interp,
	    Tcl_Obj *objPtr, Tk_Justify *justifyPtr)
}
declare 207 {
    int	 Tk_GetMMFromObj(Tcl_Interp *interp,
	    Tk_Window tkwin, Tcl_Obj *objPtr, double *doublePtr)
}
declare 208 {
    int	 Tk_GetPixelsFromObj(Tcl_Interp *interp,
	    Tk_Window tkwin, Tcl_Obj *objPtr, int *intPtr)
}
declare 209 {
    int	 Tk_GetReliefFromObj(Tcl_Interp *interp,
	    Tcl_Obj *objPtr, int *resultPtr)
}
declare 210 {
    int	 Tk_GetScrollInfoObj(Tcl_Interp *interp,
	    int objc, Tcl_Obj *const objv[], double *dblPtr, int *intPtr)
}
declare 211 {
    int	 Tk_InitOptions(Tcl_Interp *interp, char *recordPtr,
	    Tk_OptionTable optionToken, Tk_Window tkwin)
}
declare 212 {
    void  Tk_MainEx(int argc, char **argv, Tcl_AppInitProc *appInitProc,
	    Tcl_Interp *interp)
}
declare 213 {
    void  Tk_RestoreSavedOptions(Tk_SavedOptions *savePtr)
}
declare 214 {
    int	 Tk_SetOptions(Tcl_Interp *interp, char *recordPtr,
	    Tk_OptionTable optionTable, int objc,
	    Tcl_Obj *const objv[], Tk_Window tkwin,
	    Tk_SavedOptions *savePtr, int *maskPtr)
}
declare 215 {
    void Tk_InitConsoleChannels(Tcl_Interp *interp)
}
declare 216 {
    int Tk_CreateConsoleWindow(Tcl_Interp *interp)
}
declare 217 {
    void Tk_CreateSmoothMethod(Tcl_Interp *interp, const Tk_SmoothMethod *method)
}
#declare 218 {
#    void Tk_CreateCanvasVisitor(Tcl_Interp *interp, void *typePtr)
#}
#declare 219 {
#    void *Tk_GetCanvasVisitor(Tcl_Interp *interp, const char *name)
#}
declare 220 {
    int Tk_GetDash(Tcl_Interp *interp, const char *value, Tk_Dash *dash)
}
declare 221 {
    void Tk_CreateOutline(Tk_Outline *outline)
}
declare 222 {
    void Tk_DeleteOutline(Display *display, Tk_Outline *outline)
}
declare 223 {
    int Tk_ConfigOutlineGC(XGCValues *gcValues, Tk_Canvas canvas,
	    Tk_Item *item, Tk_Outline *outline)
}
declare 224 {
    int Tk_ChangeOutlineGC(Tk_Canvas canvas, Tk_Item *item,
	    Tk_Outline *outline)
}
declare 225 {
    int Tk_ResetOutlineGC(Tk_Canvas canvas, Tk_Item *item,
	    Tk_Outline *outline)
}
declare 226 {
    int Tk_CanvasPsOutline(Tk_Canvas canvas, Tk_Item *item,
	    Tk_Outline *outline)
}
declare 227 {
    void Tk_SetTSOrigin(Tk_Window tkwin, GC gc, int x, int y)
}
declare 228 {
    int Tk_CanvasGetCoordFromObj(Tcl_Interp *interp, Tk_Canvas canvas,
	    Tcl_Obj *obj, double *doublePtr)
}
declare 229 {
    void Tk_CanvasSetOffset(Tk_Canvas canvas, GC gc, Tk_TSOffset *offset)
}
declare 230 {
    void Tk_DitherPhoto(Tk_PhotoHandle handle, int x, int y, int width,
	    int height)
}
declare 231 {
    int Tk_PostscriptBitmap(Tcl_Interp *interp, Tk_Window tkwin,
	    Tk_PostscriptInfo psInfo, Pixmap bitmap, int startX,
	    int startY, int width, int height)
}
declare 232 {
    int Tk_PostscriptColor(Tcl_Interp *interp, Tk_PostscriptInfo psInfo,
	    XColor *colorPtr)
}
declare 233 {
    int Tk_PostscriptFont(Tcl_Interp *interp, Tk_PostscriptInfo psInfo,
	    Tk_Font font)
}
declare 234 {
    int Tk_PostscriptImage(Tk_Image image, Tcl_Interp *interp,
	    Tk_Window tkwin, Tk_PostscriptInfo psinfo, int x, int y,
	    int width, int height, int prepass)
}
declare 235 {
    void Tk_PostscriptPath(Tcl_Interp *interp, Tk_PostscriptInfo psInfo,
	    double *coordPtr, int numPoints)
}
declare 236 {
    int Tk_PostscriptStipple(Tcl_Interp *interp, Tk_Window tkwin,
	    Tk_PostscriptInfo psInfo, Pixmap bitmap)
}
declare 237 {
    double Tk_PostscriptY(double y, Tk_PostscriptInfo psInfo)
}
declare 238 {
    int	Tk_PostscriptPhoto(Tcl_Interp *interp,
	    Tk_PhotoImageBlock *blockPtr, Tk_PostscriptInfo psInfo,
	    int width, int height)
}

# New in 8.4a1
#
declare 239 {
    void Tk_CreateClientMessageHandler(Tk_ClientMessageProc *proc)
}
declare 240 {
    void Tk_DeleteClientMessageHandler(Tk_ClientMessageProc *proc)
}

# New in 8.4a2
#
declare 241 {
    Tk_Window Tk_CreateAnonymousWindow(Tcl_Interp *interp,
	    Tk_Window parent, const char *screenName)
}
declare 242 {
    void Tk_SetClassProcs(Tk_Window tkwin,
	    const Tk_ClassProcs *procs, ClientData instanceData)
}

# New in 8.4a4
#
declare 243 {
    void Tk_SetInternalBorderEx(Tk_Window tkwin, int left, int right,
	    int top, int bottom)
}
declare 244 {
    void Tk_SetMinimumRequestSize(Tk_Window tkwin,
	    int minWidth, int minHeight)
}

# New in 8.4a5
#
declare 245 {
    void Tk_SetCaretPos(Tk_Window tkwin, int x, int y, int height)
}
declare 246 {
    void Tk_PhotoPutBlock_Panic(Tk_PhotoHandle handle,
	    Tk_PhotoImageBlock *blockPtr, int x, int y,
	    int width, int height, int compRule)
}
declare 247 {
    void Tk_PhotoPutZoomedBlock_Panic(Tk_PhotoHandle handle,
	    Tk_PhotoImageBlock *blockPtr, int x, int y,
	    int width, int height, int zoomX, int zoomY,
	    int subsampleX, int subsampleY, int compRule)
}
declare 248 {
    int Tk_CollapseMotionEvents(Display *display, int collapse)
}

# Style engine
declare 249 {
    Tk_StyleEngine Tk_RegisterStyleEngine(const char *name,
	    Tk_StyleEngine parent)
}
declare 250 {
    Tk_StyleEngine Tk_GetStyleEngine(const char *name)
}
declare 251 {
    int Tk_RegisterStyledElement(Tk_StyleEngine engine,
	    Tk_ElementSpec *templatePtr)
}
declare 252 {
    int Tk_GetElementId(const char *name)
}
declare 253 {
    Tk_Style Tk_CreateStyle(const char *name, Tk_StyleEngine engine,
	    ClientData clientData)
}
declare 254 {
    Tk_Style Tk_GetStyle(Tcl_Interp *interp, const char *name)
}
declare 255 {
    void Tk_FreeStyle(Tk_Style style)
}
declare 256 {
    const char *Tk_NameOfStyle(Tk_Style style)
}
declare 257 {
    Tk_Style  Tk_AllocStyleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 258 {
    Tk_Style Tk_GetStyleFromObj(Tcl_Obj *objPtr)
}
declare 259 {
    void  Tk_FreeStyleFromObj(Tcl_Obj *objPtr)
}
declare 260 {
    Tk_StyledElement Tk_GetStyledElement(Tk_Style style, int elementId,
	Tk_OptionTable optionTable)
}
declare 261 {
    void Tk_GetElementSize(Tk_Style style, Tk_StyledElement element,
	    char *recordPtr, Tk_Window tkwin, int width, int height,
	    int inner, int *widthPtr, int *heightPtr)
}
declare 262 {
    void Tk_GetElementBox(Tk_Style style, Tk_StyledElement element,
	    char *recordPtr, Tk_Window tkwin, int x, int y, int width,
	    int height, int inner, int *xPtr, int *yPtr, int *widthPtr,
	    int *heightPtr)
}
declare 263 {
    int Tk_GetElementBorderWidth(Tk_Style style, Tk_StyledElement element,
	    char *recordPtr, Tk_Window tkwin)
}
declare 264 {
    void Tk_DrawElement(Tk_Style style, Tk_StyledElement element,
	    char *recordPtr, Tk_Window tkwin, Drawable d, int x, int y,
	    int width, int height, int state)
}

# TIP#116
declare 265 {
    int Tk_PhotoExpand(Tcl_Interp *interp, Tk_PhotoHandle handle,
	    int width, int height)
}
declare 266 {
    int Tk_PhotoPutBlock(Tcl_Interp *interp, Tk_PhotoHandle handle,
	    Tk_PhotoImageBlock *blockPtr, int x, int y, int width, int height,
	    int compRule)
}
declare 267 {
    int Tk_PhotoPutZoomedBlock(Tcl_Interp *interp, Tk_PhotoHandle handle,
	    Tk_PhotoImageBlock *blockPtr, int x, int y, int width, int height,
	    int zoomX, int zoomY, int subsampleX, int subsampleY, int compRule)
}
declare 268 {
    int Tk_PhotoSetSize(Tcl_Interp *interp, Tk_PhotoHandle handle,
	    int width, int height)
}
# TIP#245
declare 269 {
    long Tk_GetUserInactiveTime(Display *dpy)
}
declare 270 {
    void Tk_ResetUserInactiveTime(Display *dpy)
}

# TIP #264
declare 271 {
    Tcl_Interp *Tk_Interp(Tk_Window tkwin)
}

# Now that the Tk 8.2 -> 8.3 transition is long past, use more conventional
# means to continue support for extensions using the USE_OLD_IMAGE to
# continue use of their string-based Tcl_ImageTypes and Tcl_PhotoImageFormats.
#
# Note that this restores the usual rules for stub compatibility.  Stub-enabled
# extensions compiled against 8.5 headers and linked to the 8.5 stub library
# will produce a file [load]able into an interp with Tk 8.X, for X >= 5.
# It will *not* be [load]able into interps with Tk 8.4 (or Tk 8.2!).
# Developers who need to produce a file [load]able into legacy interps must
# build against legacy sources.
declare 272 {
    void Tk_CreateOldImageType(const Tk_ImageType *typePtr)
}
declare 273 {
    void Tk_CreateOldPhotoImageFormat(const Tk_PhotoImageFormat *formatPtr)
}

# Define the platform specific public Tk interface.  These functions are
# only available on the designated platform.

interface tkPlat

################################
# Windows specific functions

declare 0 win {
    Window Tk_AttachHWND(Tk_Window tkwin, HWND hwnd)
}
declare 1 win {
    HINSTANCE Tk_GetHINSTANCE(void)
}
declare 2 win {
    HWND Tk_GetHWND(Window window)
}
declare 3 win {
    Tk_Window Tk_HWNDToWindow(HWND hwnd)
}
declare 4 win {
    void Tk_PointerEvent(HWND hwnd, int x, int y)
}
declare 5 win {
    int Tk_TranslateWinEvent(HWND hwnd,
	    UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result)
}

################################
# Aqua specific functions

declare 0 aqua {
    void Tk_MacOSXSetEmbedHandler(
	    Tk_MacOSXEmbedRegisterWinProc *registerWinProcPtr,
	    Tk_MacOSXEmbedGetGrafPortProc *getPortProcPtr,
	    Tk_MacOSXEmbedMakeContainerExistProc *containerExistProcPtr,
	    Tk_MacOSXEmbedGetClipProc *getClipProc,
	    Tk_MacOSXEmbedGetOffsetInParentProc *getOffsetProc)
}
declare 1 aqua {
    void Tk_MacOSXTurnOffMenus(void)
}
declare 2 aqua {
    void Tk_MacOSXTkOwnsCursor(int tkOwnsIt)
}
declare 3 aqua {
    void TkMacOSXInitMenus(Tcl_Interp *interp)
}
declare 4 aqua {
    void TkMacOSXInitAppleEvents(Tcl_Interp *interp)
}
declare 5 aqua {
    void TkGenWMConfigureEvent(Tk_Window tkwin, int x, int y, int width,
	    int height, int flags)
}
declare 6 aqua {
    void TkMacOSXInvalClipRgns(Tk_Window tkwin)
}
declare 7 aqua {
    void *TkMacOSXGetDrawablePort(Drawable drawable)
}
declare 8 aqua {
    void *TkMacOSXGetRootControl(Drawable drawable)
}
declare 9 aqua {
    void Tk_MacOSXSetupTkNotifier(void)
}
declare 10 aqua {
    int Tk_MacOSXIsAppInFront(void)
}

##############################################################################

# Public functions that are not accessible via the stubs table.

export {
    const char *Tk_PkgInitStubsCheck(Tcl_Interp *interp, const char *version,
	    int exact)
}

# Local Variables:
# mode: tcl
# End:
