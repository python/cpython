puff: puff.c puff.h
	cc -DTEST -o puff puff.c

test: puff
	puff zeros.raw

clean:
	rm -f puff puff.o
