#include <stdio.h>

long *bm;
long *nbm;
long h, w;
int nh, nw;
long factor;

#define OC(x,xi) ((x)*factor+(xi))
#define BM(x,xi,y,yi) bm[OC(y,yi)*w+OC(x,xi)]

#define COMP(r,g,b) ((r) | ((g)<<8) | ((b) << 16))

#define R(comp) ((comp) & 0xff)
#define G(comp) (((comp)>>8) & 0xff)
#define B(comp) (((comp)>>16) & 0xff)

#define CHOICEFUNC(np1, np2) ( random() & 1 )

int inlevels = 3*255;
int outlevels = 1;

main(argc, argv)
    char **argv;
{
    char lbuf[100];
    int x, y, xi, yi;
    int num;
    int r, g, b;
    long data;
    int i;
    double greyness;
    int inpixels, outpixels;
    int resid;

    setvbuf(stdout, 0, _IOFBF, 1024*128);
    if( argc != 2) {
       fprintf(stderr, "Usage: tomono factor\n");
       exit(1);
    }
    factor = atoi(argv[1]);
    gets(lbuf);
    if ( sscanf(lbuf, "(%d,%d)", &w, &h) != 2) {
	fprintf(stderr, "%s: bad size spec: %s\n", argv[0], lbuf);
	exit(1);
    }
    nh = h / factor;
    nw = w / factor;
    printf("(%d,%d)\n", nw, nh);
    if ( (bm = (long *)malloc(h*w*sizeof(long))) == 0) {
	fprintf(stderr, "%s: No memory\n", argv[0]);
	exit(1);
    }
    if ( (nbm = (long *)malloc(nh*nw*sizeof(long))) == 0) {
	fprintf(stderr, "%s: No memory\n", argv[0]);
	exit(1);
    }
    while( !feof(stdin) ) {
	gets(lbuf);
	if ( feof(stdin) ) break;
	puts(lbuf);
	fprintf(stderr, "Reading %d\n", h*w*sizeof(long));
	if ( (i=fread(bm, 1, h*w*sizeof(long), stdin)) != h*w*sizeof(long)) {
	    fprintf(stderr, "%s: short read, %d wanted %d\n", argv[0],
		i, h*w*sizeof(long));
	    exit(1);
	}
	/*
	** Compute picture blackness.
	*/
	inpixels = 0;
	inpixels = countpixels(0,0,w,h);
	greyness = (double)inpixels/(h*w*inlevels);
	fprintf(stderr, "%3.1f%% grey\n", 100.0*greyness);
	outpixels = (int)(greyness*outlevels*nh*nw);
	fprintf(stderr, "Inpixels: %d (%d) Outpixels %d\n", inpixels, inpixels/inlevels, outpixels);
	resid = fillpixels(0,0,nw,nh,0,0,w,h,outpixels);
	if ( resid > 1 ) fprintf(stderr, "Residue: %d pixels\n", resid);
	fprintf(stderr, "Writing %d\n", (nh*nw)*sizeof(long));
	fwrite(nbm, 1, (nh*nw)*sizeof(long), stdout);
    }
    exit(0);
}

countpixels(x0,y0,x1,y1)
{
    int x, y, tot, data;

    tot = 0;
    for( y=y0; y<y1; y++)
	for(x=x0; x<x1; x++) {
	    data = bm[y*w+x];
	    tot += R(data);
	    tot += G(data);
	    tot += B(data);
    }
    return tot;
}

fillpixels(x0,y0,x1,y1,ox0,oy0,ox1,oy1,npixels)
{
    int m, om, p1, p2, np1, np2, rp, resid;

    if ( npixels == 0 ) return 0;
    if ( x0+1 >= x1 && y0+1 >= y1 ) {
	if ( npixels ) {
	    nbm[y0*nw+x0] = 0xffffff;
/* 	    fprintf(stderr, "->%d,%d\n", x0,y0); */
	    return npixels - 1;
	}
	return 0;
    }
    if ( x1-x0 < y1-y0 ) {
	if ( y1 - y0 <= 2 )
	    m = y0 + 1;
	else
	    m = y0+1+(random()%(y1-y0-1));
/* 	fprintf(stderr,"%d,%d %d,%d Y %d\n", x0, x1, y0, y1, m); */
	/* om = (oy0+oy1)/2; */ om = m;
	p1 = countpixels(ox0,oy0,ox1,om);
	p2 = countpixels(ox0,om,ox1,oy1);
	np1 = (int)(((float)p1/(p1+p2))*npixels);
	np2 = (int)(((float)p2/(p1+p2))*npixels);
	rp = npixels - np1 - np2;
	if ( rp ) {
	    np1 += rp/2;
	    rp = rp - rp/2;
	    np2 += rp;
	}
	resid = 0;
	if ( CHOICEFUNC(np1, np2) ) {
	    resid = fillpixels(x0,y0,x1,m,ox0,oy0,ox1,om,np1+resid);
	    resid = fillpixels(x0,m,x1,y1,ox0,om,ox1,oy1,np2+resid);
	} else {
	    resid = fillpixels(x0,m,x1,y1,ox0,om,ox1,oy1,np2+resid);
	    resid = fillpixels(x0,y0,x1,m,ox0,oy0,ox1,om,np1+resid);
	}
    } else {
	if ( x1 - x0 <= 2 )
	    m = x0 + 1;
	else
	    m = x0+1+(random()%(x1-x0-1));
/* 	fprintf(stderr,"%d,%d %d,%d X %d\n", x0, x1, y0, y1, m); */
	/* om = (ox0+ox1)/2; */ om = m;
	p1 = countpixels(ox0,oy0,om,oy1);
	p2 = countpixels(om,oy0,ox1,oy1);
	np1 = (int)(((float)p1/(p1+p2))*npixels);
	np2 = (int)(((float)p2/(p1+p2))*npixels);
	rp = npixels - np1 - np2;
	if ( rp ) {
	    np1 += rp/2;
	    rp = rp - rp/2;
	    np2 += rp;
	}
	resid = 0;
	if ( CHOICEFUNC(np1, np2) ) {
	    resid = fillpixels(x0,y0,m,y1,ox0,oy0,om,oy1,np1+resid);
	    resid = fillpixels(m,y0,x1,y1,om,oy0,ox1,oy1,np2+resid);
	} else {
	    resid = fillpixels(m,y0,x1,y1,om,oy0,ox1,oy1,np2+resid);
	    resid = fillpixels(x0,y0,m,y1,ox0,oy0,om,oy1,np1+resid);
	}
    }
    return resid;
}
