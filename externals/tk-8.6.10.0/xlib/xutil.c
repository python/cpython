/*
 * xutil.c --
 *
 *	This function contains generic X emulation routines.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 *----------------------------------------------------------------------
 *
 * XInternAtom --
 *
 *	This procedure simulates the XInternAtom function by calling Tk_Uid to
 *	get a unique id for every atom. This is only a partial implementation,
 *	since it doesn't work across applications.
 *
 * Results:
 *	A new Atom.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Atom
XInternAtom(
    Display *display,
    _Xconst char *atom_name,
    Bool only_if_exists)
{
    static Atom atom = XA_LAST_PREDEFINED;

    display->request++;
    return ++atom;
}

/*
 *----------------------------------------------------------------------
 *
 * XGetVisualInfo --
 *
 *	Returns information about the specified visual.
 *
 * Results:
 *	Returns a newly allocated XVisualInfo structure.
 *
 * Side effects:
 *	Allocates storage.
 *
 *----------------------------------------------------------------------
 */

XVisualInfo *
XGetVisualInfo(
    Display *display,
    long vinfo_mask,
    XVisualInfo *vinfo_template,
    int *nitems_return)
{
    XVisualInfo *info = ckalloc(sizeof(XVisualInfo));

    info->visual = DefaultVisual(display, 0);
    info->visualid = info->visual->visualid;
    info->screen = 0;
    info->depth = info->visual->bits_per_rgb;
    info->c_class = info->visual->c_class;
    info->colormap_size = info->visual->map_entries;
    info->bits_per_rgb = info->visual->bits_per_rgb;
    info->red_mask = info->visual->red_mask;
    info->green_mask = info->visual->green_mask;
    info->blue_mask = info->visual->blue_mask;

    if (((vinfo_mask & VisualIDMask)
	    && (vinfo_template->visualid != info->visualid))
	    || ((vinfo_mask & VisualScreenMask)
		    && (vinfo_template->screen != info->screen))
	    || ((vinfo_mask & VisualDepthMask)
		    && (vinfo_template->depth != info->depth))
	    || ((vinfo_mask & VisualClassMask)
		    && (vinfo_template->c_class != info->c_class))
	    || ((vinfo_mask & VisualColormapSizeMask)
		    && (vinfo_template->colormap_size != info->colormap_size))
	    || ((vinfo_mask & VisualBitsPerRGBMask)
		    && (vinfo_template->bits_per_rgb != info->bits_per_rgb))
	    || ((vinfo_mask & VisualRedMaskMask)
		    && (vinfo_template->red_mask != info->red_mask))
	    || ((vinfo_mask & VisualGreenMaskMask)
		    && (vinfo_template->green_mask != info->green_mask))
	    || ((vinfo_mask & VisualBlueMaskMask)
		    && (vinfo_template->blue_mask != info->blue_mask))
	) {
	ckfree(info);
	return NULL;
    }

    *nitems_return = 1;
    return info;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
