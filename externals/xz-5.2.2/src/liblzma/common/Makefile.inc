##
## Author: Lasse Collin
##
## This file has been put into the public domain.
## You can do whatever you want with this file.
##

liblzma_la_SOURCES += \
	common/common.c \
	common/common.h \
	common/memcmplen.h \
	common/block_util.c \
	common/easy_preset.c \
	common/easy_preset.h \
	common/filter_common.c \
	common/filter_common.h \
	common/hardware_physmem.c \
	common/index.c \
	common/index.h \
	common/stream_flags_common.c \
	common/stream_flags_common.h \
	common/vli_size.c

if COND_MAIN_ENCODER
liblzma_la_SOURCES += \
	common/alone_encoder.c \
	common/block_buffer_encoder.c \
	common/block_buffer_encoder.h \
	common/block_encoder.c \
	common/block_encoder.h \
	common/block_header_encoder.c \
	common/easy_buffer_encoder.c \
	common/easy_encoder.c \
	common/easy_encoder_memusage.c \
	common/filter_buffer_encoder.c \
	common/filter_encoder.c \
	common/filter_encoder.h \
	common/filter_flags_encoder.c \
	common/index_encoder.c \
	common/index_encoder.h \
	common/stream_buffer_encoder.c \
	common/stream_encoder.c \
	common/stream_flags_encoder.c \
	common/vli_encoder.c

if COND_THREADS
liblzma_la_SOURCES += \
	common/hardware_cputhreads.c \
	common/outqueue.c \
	common/outqueue.h \
	common/stream_encoder_mt.c
endif
endif

if COND_MAIN_DECODER
liblzma_la_SOURCES += \
	common/alone_decoder.c \
	common/alone_decoder.h \
	common/auto_decoder.c \
	common/block_buffer_decoder.c \
	common/block_decoder.c \
	common/block_decoder.h \
	common/block_header_decoder.c \
	common/easy_decoder_memusage.c \
	common/filter_buffer_decoder.c \
	common/filter_decoder.c \
	common/filter_decoder.h \
	common/filter_flags_decoder.c \
	common/index_decoder.c \
	common/index_hash.c \
	common/stream_buffer_decoder.c \
	common/stream_decoder.c \
	common/stream_decoder.h \
	common/stream_flags_decoder.c \
	common/vli_decoder.c
endif
