#include <stdio.h>

long *bm;
long h, w;
long factor;

#define OC(x,xi) ((x)*factor+(xi))
#define BM(x,xi,y,yi) bm[OC(y,yi)*w+OC(x,xi)]

#define COMP(r,g,b) ((r) | ((g)<<8) | ((b) << 16))

#define R(comp) ((comp) & 0xff)
#define G(comp) (((comp)>>8) & 0xff)
#define B(comp) (((comp)>>16) & 0xff)

main(argc, argv)
    char **argv;
{
    char lbuf[100];
    int nh, nw;
    int x, y, xi, yi;
    int num;
    int r, g, b;
    long data;
    long *nbm, *nbmp;
    int i;
    int bits, mask, roundbit, addbit;
    int pf;
    int newfmt = 0;

    if( argc != 2 && argc != 3) {
       fprintf(stderr, "Usage: squash factor [bits]\n");
       exit(1);
    }
    factor = atoi(argv[1]);
    if ( argc > 2 ) {
	bits = atoi(argv[2]);
	mask = (1 << bits) - 1;
	mask <<= (8-bits);
	roundbit = 1 << (7-bits);
	addbit = 1 << (8-bits);
	fprintf(stderr, "%x %x %x\n", mask, roundbit, addbit);
    } else {
	mask = 0xff;
	roundbit = 0;
	addbit = 0;
    }
    gets(lbuf);
    if ( strncmp( lbuf, "CMIF", 4) == 0 ) {
	newfmt = 1;
	gets(lbuf);
	if( sscanf(lbuf, "(%d,%d,%d)", &w, &h, &pf) != 3) {
	    fprintf(stderr, "%s: bad size spec: %s\n", argv[0], lbuf);
	    exit(1);
	}
	if ( pf != 0 ) {
	    fprintf(stderr, "%s: packed file\n", argv[0]);
	    exit(1);
	}
    } else {
	if ( sscanf(lbuf, "(%d,%d)", &w, &h) != 2) {
	    fprintf(stderr, "%s: bad size spec: %s\n", argv[0], lbuf);
	    exit(1);
	}
    }
    nh = h / factor;
    nw = w / factor;
    if ( newfmt )
	printf("CMIF video 1.0\n(%d,%d,%d)\n", nw, nh, 0);
    else
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
	nbmp = nbm;
	for( y=0; y<nh; y++) {
	    for ( x=0; x<nw; x++) {
		r = g = b = 0;
		num = 0;
		for( xi=0; xi<factor; xi++ ) {
		    for(yi=0; yi<factor; yi++) {
			if ( y*factor+yi < h && x*factor+xi < w ) {
			    num++;
			    data = BM(x,xi,y,yi);
			    r += R(data);
			    g += G(data);
			    b += B(data);
			}
			else fprintf(stderr, "skip %d %d %d %d\n", x, xi, y, yi);
		    }
		}
		r = r/num; g = g/num; b = b/num;
		if ( (r & mask) != mask && ( r & roundbit) ) r += addbit;
		if ( (g & mask) != mask && ( g & roundbit) ) g += addbit;
		if ( (b & mask) != mask && ( b & roundbit) ) b += addbit;
		data = COMP(r, g, b);
		*nbmp++ = data;
	    }
	}
	if (nbmp - nbm != nh * nw ) fprintf(stderr, "%d %d\n", nbmp-nbm, nh*nw);
	fprintf(stderr, "Writing %d\n", (nbmp-nbm)*sizeof(long));
	fwrite(nbm, 1, (nbmp-nbm)*sizeof(long), stdout);
    }
    exit(0);
}
