/*
 * i2v -- image-to-video.
 * Convert an SGI image file to a format that is immediately usable
 * by lrectwrite.
 * The header of the file contains a description (in ASCII)
 * padded to 8196 byte for fast access of the rest of the file.
 *
 * Based upon "showimg.c" by Paul Haeberli.
 * --Guido van Rossum, CWI, Amsterdam
 */
#include <stdio.h>
#include <gl/gl.h>
#include <gl/device.h>
#include <gl/image.h>

unsigned short rs[8192];
unsigned short gs[8192];
unsigned short bs[8192];

IMAGE *image;
int xsize, ysize, zsize;
FILE *fp;

char header[100];
char *progname = "i2v";

main(argc,argv)
int argc;
char **argv;
{
    int y;
    if (argc > 0) progname = argv[0];
    if( argc != 3 ) {
	fprintf(stderr, "usage: %s infile outfile\n", progname);
	exit(2);
    } 
    if( (image=iopen(argv[1],"r")) == NULL ) {
	fprintf(stderr, "%s: can't open input file %s\n",progname, argv[1]);
	exit(1);
    }
    xsize = image->xsize;
    ysize = image->ysize;
    zsize = image->zsize;
    if ((fp = fopen(argv[2], "w")) == NULL) {
	fprintf(stderr,"%s: can't open output file %s\n", progname, argv[2]);
	exit(1);
    }
    fprintf(fp, "CMIF video 1.0\n");
    fprintf(fp, "(%d, %d, %d)\n", xsize, ysize, 0);
    fprintf(fp, "0, %ld\n", (long)xsize * (long)ysize * sizeof(long));
    fflush(fp);
    for(y = 0; y < ysize; y++) {
		if(zsize<3) {
			getrow(image, rs, y, 0);
			writepacked(xsize, rs, rs, rs);
		} else {
			getrow(image, rs, y, 0);
			getrow(image, gs, y, 1);
			getrow(image, bs, y, 2);
			writepacked(xsize, rs, gs, bs);
		}
    }
    exit(0);
}

writepacked(n, rsptr, gsptr, bsptr)
	int n;
	short *rsptr, *gsptr, *bsptr;
{
	long parray[8192];
	long *pptr = parray;
	int i = n;
	while (--i >= 0) {
		*pptr++ = *rsptr++ | (*gsptr++<<8) | (*bsptr++<<16);
	}
	if (fwrite((char *) parray, sizeof(long), n, fp) != n) {
		perror("fwrite");
		exit(1);
	}
}
