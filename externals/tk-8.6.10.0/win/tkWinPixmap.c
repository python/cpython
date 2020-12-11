/*
 * tkWinPixmap.c --
 *
 *	This file contains the Xlib emulation functions pertaining to creating
 *	and destroying pixmaps.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetPixmap --
 *
 *	Creates an in memory drawing surface.
 *
 * Results:
 *	Returns a handle to a new pixmap.
 *
 * Side effects:
 *	Allocates a new Win32 bitmap.
 *
 *----------------------------------------------------------------------
 */

Pixmap
Tk_GetPixmap(
    Display *display,
    Drawable d,
    int width,
    int height,
    int depth)
{
    TkWinDrawable *newTwdPtr, *twdPtr;
    int planes;
    Screen *screen;

    display->request++;

    newTwdPtr = ckalloc(sizeof(TkWinDrawable));
    newTwdPtr->type = TWD_BITMAP;
    newTwdPtr->bitmap.depth = depth;
    twdPtr = (TkWinDrawable *) d;
    if (twdPtr->type != TWD_BITMAP) {
	if (twdPtr->window.winPtr == NULL) {
	    newTwdPtr->bitmap.colormap = DefaultColormap(display,
		    DefaultScreen(display));
	} else {
	    newTwdPtr->bitmap.colormap = twdPtr->window.winPtr->atts.colormap;
	}
    } else {
	newTwdPtr->bitmap.colormap = twdPtr->bitmap.colormap;
    }
    screen = &display->screens[0];
    planes = 1;
    if (depth == screen->root_depth) {
	planes = PTR2INT(screen->ext_data);
	depth /= planes;
    }
    newTwdPtr->bitmap.handle =
	    CreateBitmap(width, height, (DWORD) planes, (DWORD) depth, NULL);

    /*
     * CreateBitmap tries to use memory on the graphics card. If it fails,
     * call CreateDIBSection which uses real memory; slower, but at least
     * still works. [Bug 2080533]
     */

    if (newTwdPtr->bitmap.handle == NULL) {
	static int repeatError = 0;
	void *bits = NULL;
	BITMAPINFO bitmapInfo;
	HDC dc;

	memset(&bitmapInfo, 0, sizeof(bitmapInfo));
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = height;
	bitmapInfo.bmiHeader.biPlanes = planes;
	bitmapInfo.bmiHeader.biBitCount = depth;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	dc = GetDC(NULL);
	newTwdPtr->bitmap.handle = CreateDIBSection(dc, &bitmapInfo,
		DIB_RGB_COLORS, &bits, 0, 0);
	ReleaseDC(NULL, dc);

	/*
	 * Oh no! Things are still going wrong. Pop up a warning message here
	 * (because things will probably crash soon) which will encourage
	 * people to report this as a bug...
	 */

	if (newTwdPtr->bitmap.handle == NULL && !repeatError) {
	    LPVOID lpMsgBuf;

	    repeatError = 1;
	    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		    FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL, GetLastError(),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPWSTR)&lpMsgBuf, 0, NULL)) {
		MessageBoxW(NULL, (LPWSTR) lpMsgBuf,
			L"Tk_GetPixmap: Error from CreateDIBSection",
			MB_OK | MB_ICONINFORMATION);
		LocalFree(lpMsgBuf);
	    }
	}
    }

    if (newTwdPtr->bitmap.handle == NULL) {
	ckfree(newTwdPtr);
	return None;
    }

    return (Pixmap) newTwdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreePixmap --
 *
 *	Release the resources associated with a pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the bitmap created by Tk_GetPixmap.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreePixmap(
    Display *display,
    Pixmap pixmap)
{
    TkWinDrawable *twdPtr = (TkWinDrawable *) pixmap;

    display->request++;
    if (twdPtr != NULL) {
	DeleteObject(twdPtr->bitmap.handle);
	ckfree(twdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetPixmapColormap --
 *
 *	The following function is a hack used by the photo widget to
 *	explicitly set the colormap slot of a Pixmap.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkSetPixmapColormap(
    Pixmap pixmap,
    Colormap colormap)
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)pixmap;
    twdPtr->bitmap.colormap = colormap;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetGeometry --
 *
 *	Retrieve the geometry of the given drawable. Note that this is a
 *	degenerate implementation that only returns the size of a pixmap or
 *	window.
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
XGetGeometry(
    Display *display,
    Drawable d,
    Window *root_return,
    int *x_return,
    int *y_return,
    unsigned int *width_return,
    unsigned int *height_return,
    unsigned int *border_width_return,
    unsigned int *depth_return)
{
    TkWinDrawable *twdPtr = (TkWinDrawable *)d;

    if (twdPtr->type == TWD_BITMAP) {
	HDC dc;
	BITMAPINFO info;

	if (twdPtr->bitmap.handle == NULL) {
	    Tcl_Panic("XGetGeometry: invalid pixmap");
	}
	dc = GetDC(NULL);
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biBitCount = 0;
	if (!GetDIBits(dc, twdPtr->bitmap.handle, 0, 0, NULL, &info,
		DIB_RGB_COLORS)) {
	    Tcl_Panic("XGetGeometry: unable to get bitmap size");
	}
	ReleaseDC(NULL, dc);

	*width_return = info.bmiHeader.biWidth;
	*height_return = info.bmiHeader.biHeight;
    } else if (twdPtr->type == TWD_WINDOW) {
	RECT rect;

	if (twdPtr->window.handle == NULL) {
	    Tcl_Panic("XGetGeometry: invalid window");
	}
	GetClientRect(twdPtr->window.handle, &rect);
	*width_return = rect.right - rect.left;
	*height_return = rect.bottom - rect.top;
    } else {
	Tcl_Panic("XGetGeometry: invalid window");
    }
    return 1;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
