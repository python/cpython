errmap.h:	generrmap.exe
	.\generrmap.exe > errmap.h

genermap.exe:	generrmap.c
	cl generrmap.c
