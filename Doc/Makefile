TBL=		tbl
TROFF=		ditroff
MS=		-ms
FUNNYTAB=	$$HOME/lib/funnytab
PREVIEW=	dpv -f $(FUNNYTAB)

tut:		tut.dit
		$(PREVIEW) tut.dit

mod:		mod.dit
		$(PREVIEW) mod.dit

paper:		paper.dit
		$(PREVIEW) paper.dit

ref:		ref.dit
		$(PREVIEW) ref.dit

ALL=		tut.dit mod.dit paper.dit ref.dit

all:		$(ALL)

$(ALL):		macros.ms

clean:
		rm -f $(ALL)

.SUFFIXES:	.ms .dit

.ms.dit:
		$(TBL) $*.ms | $(TROFF) $(MS) >$@
