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

    if( argc != 2) {
       fprintf(stderr, "Usage: squash factor\n");
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
	nbmp = nbm;
	for( y=0; y<nh; y++) {
	    for ( x=0; x<nw; x++) {
		r = g = b = 0;
		num = 0;
		*nbmp++ = BM(x,0,y,0);
	    }
	}
	if (nbmp - nbm != nh * nw ) fprintf(stderr, "%d %d\n", nbmp-nbm, nh*nw);
	fprintf(stderr, "Writing %d\n", (nbmp-nbm)*sizeof(long));
	fwrite(nbm, 1, (nbmp-nbm)*sizeof(long), stdout);
    }
    exit(0);
}
