##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

EXTRA_DIST += lzma/fastpos_tablegen.c

liblzma_la_SOURCES += lzma/lzma_common.h

if COND_FILTER_LZMA1
liblzma_la_SOURCES += \
	lzma/lzma_encoder_presets.c
endif

if COND_ENCODER_LZMA1
liblzma_la_SOURCES += \
	lzma/fastpos.h \
	lzma/lzma_encoder.h \
	lzma/lzma_encoder.c \
	lzma/lzma_encoder_private.h \
	lzma/lzma_encoder_optimum_fast.c \
	lzma/lzma_encoder_optimum_normal.c

if !COND_SMALL
liblzma_la_SOURCES += lzma/fastpos_table.c
endif
endif

if COND_DECODER_LZMA1
liblzma_la_SOURCES += \
	lzma/lzma_decoder.c \
	lzma/lzma_decoder.h
endif

if COND_ENCODER_LZMA2
liblzma_la_SOURCES += \
	lzma/lzma2_encoder.c \
	lzma/lzma2_encoder.h
endif

if COND_DECODER_LZMA2
liblzma_la_SOURCES += \
	lzma/lzma2_decoder.c \
	lzma/lzma2_decoder.h
endif
