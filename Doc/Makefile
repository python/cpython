TBL=		tbl
TROFF=		ditroff
MS=		-ms
FUNNYTAB=	$$HOME/lib/funnytab
PREVIEW=	dpv -f $(FUNNYTAB)
PSDIT=		psdit

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
		rm -f *.dit *.ps core @* ,* 

.SUFFIXES:	# Remove default suffixes

.SUFFIXES:	.ms .dit .ps

.ms.dit:
		$(TBL) $*.ms | $(TROFF) $(MS) >$@

.dit.ps:
		$(PSDIT) <$*.dit >$@

.ms.ps:
		$(TBL) $*.ms | $(TROFF) $(MS) | $(PSDIT) >$@
