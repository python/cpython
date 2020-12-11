/*
 * tkImgUtil.c --
 *
 *	This file contains image related utility functions.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"
#include "xbytes.h"


/*
 *----------------------------------------------------------------------
 *
 * TkAlignImageData --
 *
 *	This function takes an image and copies the data into an aligned
 *	buffer, performing any necessary bit swapping.
 *
 * Results:
 *	Returns a newly allocated buffer that should be freed by the caller.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TkAlignImageData(
    XImage *image,		/* Image to be aligned. */
    int alignment,		/* Number of bytes to which the data should be
				 * aligned (e.g. 2 or 4) */
    int bitOrder)		/* Desired bit order: LSBFirst or MSBFirst. */
{
    long dataWidth;
    char *data, *srcPtr, *destPtr;
    int i, j;

    if (image->bits_per_pixel != 1) {
	Tcl_Panic(
		"TkAlignImageData: Can't handle image depths greater than 1.");
    }

    /*
     * Compute line width for output data buffer.
     */

    dataWidth = image->bytes_per_line;
    if (dataWidth % alignment) {
	dataWidth += (alignment - (dataWidth % alignment));
    }

    data = ckalloc(dataWidth * image->height);

    destPtr = data;
    for (i = 0; i < image->height; i++) {
	srcPtr = &image->data[i * image->bytes_per_line];
	for (j = 0; j < dataWidth; j++) {
	    if (j >= image->bytes_per_line) {
		*destPtr = 0;
	    } else if (image->bitmap_bit_order != bitOrder) {
		*destPtr = xBitReverseTable[(unsigned char)(*(srcPtr++))];
	    } else {
		*destPtr = *(srcPtr++);
	    }
	    destPtr++;
	}
    }
    return data;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
