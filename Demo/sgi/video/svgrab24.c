/*
 * svgrab24 - Grab the current video input image into an rgb file.
 *
 * Jack Jansen, CWI, May 93.
 *
 * Adapted from grabone.c
 */

#ident "$Revision$"

#include <stdio.h>
#include <stdlib.h>
#include <svideo.h>
#include <gl/gl.h>
#include <gl/device.h>
#include <getopt.h>
#include <image.h>

main(int argc, char *argv[])
{
    SVhandle V;
    svCaptureInfo ci;
    boolean debug;
    int ch, errflg;
    int bufferSize;
    long *buffer;
    IMAGE *imgfile;
    short *r, *g, *b;
    int x, y;
    char *ProgName = argv[0];

    debug = FALSE;
    ci.format = SV_RGB32_FRAMES;
    ci.width = 0;
    ci.height = 0;
    ci.size = 1;

    argv++; argc--;
    if ( argc > 2 && strcmp(argv[0], "-w") == 0) {
	ci.width = atoi(argv[1]);
	argc -= 2;
	argv += 2;
    }
    if ( argc != 1 ) {
	fprintf(stderr, "Usage: %s [-w width] rgbfilename\n", ProgName);
	exit(1);
    }

    /* Open video device */
    if ((V = svOpenVideo()) == NULL) {
	svPerror("open");
	exit(1);
    }

    if (svQueryCaptureBufferSize(V, &ci, &bufferSize) < 0) {
        svPerror("svQueryCaptureBufferSize");
        exit(1);
    }
    buffer = malloc(bufferSize);

    if (svCaptureOneFrame(V, ci.format, &ci.width, &ci.height, buffer) < 0) {
        svPerror("svCaptureOneFrame");
        exit(1);
    }
    if (debug) {
        printf("captured size: %d by %d\n", ci.width, ci.height);
    }

    if ( (imgfile=iopen(argv[0], "w", RLE(1), 3, ci.width, ci.height, 3)) == 0) {
	perror(argv[1]);
	exit(1);
    }
    r = (short *)malloc(ci.width*sizeof(short));
    g = (short *)malloc(ci.width*sizeof(short));
    b = (short *)malloc(ci.width*sizeof(short));
    if ( !r || !g || !b ) {
	fprintf(stderr, "%s: malloc failed\n", ProgName);
	exit(1);
    }
    for(y=0; y<ci.height; y++) {
	for(x=0; x<ci.width; x++) {
	    unsigned long data = *buffer++;

	    r[x] = data & 0xff;
	    g[x] = (data>>8) & 0xff;
	    b[x] = (data>>16) & 0xff;
	}
	if ( putrow(imgfile, r, y, 0) == 0 ||
	     putrow(imgfile, g, y, 1) == 0 ||
	     putrow(imgfile, b, y, 2) == 0) {
	       fprintf(stderr, "%s: putrow failed\n", ProgName);
	       exit(1);
	}
    }
    iclose(imgfile);
    exit(0);
}
