/* $CJKCodecs: tweak_gbk.h,v 1.1.1.1 2003/09/24 17:47:00 perky Exp $ */

#define GBK_PREDECODE(dc1, dc2, assi) \
	if ((dc1) == 0xa1 && (dc2) == 0xaa) (assi) = 0x2014; \
	else if ((dc1) == 0xa8 && (dc2) == 0x44) (assi) = 0x2015; \
	else if ((dc1) == 0xa1 && (dc2) == 0xa4) (assi) = 0x00b7;
#define GBK_PREENCODE(code, assi) \
	if ((code) == 0x2014) (assi) = 0xa1aa; \
	else if ((code) == 0x2015) (assi) = 0xa844; \
	else if ((code) == 0x00b7) (assi) = 0xa1a4;

