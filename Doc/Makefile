TROFF=		ditroff
MS=		-ms
PREVIEW=	dpv

intro:		intro.dit
		$(PREVIEW) intro.dit

modules:	modules.dit
		$(PREVIEW) modules.dit

paper:		paper.dit
		$(PREVIEW) paper.dit

ref:		ref.dit
		$(PREVIEW) ref.dit

ALL=		intro.dit modules.dit paper.dit ref.dit

all:		$(ALL)

$(ALL):		macros.ms

clean:
		rm -f $(ALL)

.SUFFIXES:	.ms .dit

.ms.dit:
		$(TROFF) $(MS) $*.ms >$@
