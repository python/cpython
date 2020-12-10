###############################################################################
#
# Makefile to build XZ Utils using DJGPP
#
# Author: Lasse Collin
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#
###############################################################################

# For debugging, set comment "#define NDEBUG 1" from config.h to enable
# the assert() macro, set STRIP=rem to disable stripping, and finally
# e.g. CFLAGS="-g -O0".
CC = gcc
STRIP = strip
CPPFLAGS =
CFLAGS = -g -Wall -Wextra -Wfatal-errors -march=i386 -mtune=i686 -O2
LDFLAGS = -lemu

# NOTE: -fgnu89-inline is needed on DJGPP 2.04 beta and GCC >= 4.3.0
# because time.h uses GNU-style "extern inline".
ALL_CFLAGS = -std=gnu99 -fgnu89-inline

ALL_CPPFLAGS = \
	-I. \
	-I../lib \
	-I../src/common \
	-I../src/liblzma/api \
	-I../src/liblzma/common \
	-I../src/liblzma/check \
	-I../src/liblzma/rangecoder \
	-I../src/liblzma/lz \
	-I../src/liblzma/lzma \
	-I../src/liblzma/delta \
	-I../src/liblzma/simple \
	-DHAVE_CONFIG_H

ALL_CPPFLAGS += $(CPPFLAGS)
ALL_CFLAGS += $(CFLAGS)

.PHONY: all
all: xz.exe

SRCS_C = \
	../lib/getopt.c \
	../lib/getopt1.c \
	../src/common/tuklib_cpucores.c \
	../src/common/tuklib_exit.c \
	../src/common/tuklib_mbstr_fw.c \
	../src/common/tuklib_mbstr_width.c \
	../src/common/tuklib_open_stdxxx.c \
	../src/common/tuklib_physmem.c \
	../src/common/tuklib_progname.c \
	../src/liblzma/check/check.c \
	../src/liblzma/check/crc32_table.c \
	../src/liblzma/check/crc64_table.c \
	../src/liblzma/check/sha256.c \
	../src/liblzma/common/alone_decoder.c \
	../src/liblzma/common/alone_encoder.c \
	../src/liblzma/common/block_decoder.c \
	../src/liblzma/common/block_encoder.c \
	../src/liblzma/common/block_header_decoder.c \
	../src/liblzma/common/block_header_encoder.c \
	../src/liblzma/common/block_util.c \
	../src/liblzma/common/common.c \
	../src/liblzma/common/filter_common.c \
	../src/liblzma/common/filter_decoder.c \
	../src/liblzma/common/filter_encoder.c \
	../src/liblzma/common/filter_flags_decoder.c \
	../src/liblzma/common/filter_flags_encoder.c \
	../src/liblzma/common/hardware_physmem.c \
	../src/liblzma/common/index.c \
	../src/liblzma/common/index_decoder.c \
	../src/liblzma/common/index_encoder.c \
	../src/liblzma/common/index_hash.c \
	../src/liblzma/common/stream_decoder.c \
	../src/liblzma/common/stream_encoder.c \
	../src/liblzma/common/stream_flags_common.c \
	../src/liblzma/common/stream_flags_decoder.c \
	../src/liblzma/common/stream_flags_encoder.c \
	../src/liblzma/common/vli_decoder.c \
	../src/liblzma/common/vli_encoder.c \
	../src/liblzma/common/vli_size.c \
	../src/liblzma/delta/delta_common.c \
	../src/liblzma/delta/delta_decoder.c \
	../src/liblzma/delta/delta_encoder.c \
	../src/liblzma/lz/lz_decoder.c \
	../src/liblzma/lz/lz_encoder.c \
	../src/liblzma/lz/lz_encoder_mf.c \
	../src/liblzma/lzma/fastpos_table.c \
	../src/liblzma/lzma/lzma2_decoder.c \
	../src/liblzma/lzma/lzma2_encoder.c \
	../src/liblzma/lzma/lzma_decoder.c \
	../src/liblzma/lzma/lzma_encoder.c \
	../src/liblzma/lzma/lzma_encoder_optimum_fast.c \
	../src/liblzma/lzma/lzma_encoder_optimum_normal.c \
	../src/liblzma/lzma/lzma_encoder_presets.c \
	../src/liblzma/rangecoder/price_table.c \
	../src/liblzma/simple/arm.c \
	../src/liblzma/simple/armthumb.c \
	../src/liblzma/simple/ia64.c \
	../src/liblzma/simple/powerpc.c \
	../src/liblzma/simple/simple_coder.c \
	../src/liblzma/simple/simple_decoder.c \
	../src/liblzma/simple/simple_encoder.c \
	../src/liblzma/simple/sparc.c \
	../src/liblzma/simple/x86.c \
	../src/xz/args.c \
	../src/xz/coder.c \
	../src/xz/file_io.c \
	../src/xz/hardware.c \
	../src/xz/list.c \
	../src/xz/main.c \
	../src/xz/message.c \
	../src/xz/mytime.c \
	../src/xz/options.c \
	../src/xz/signals.c \
	../src/xz/suffix.c \
	../src/xz/util.c
SRCS_ASM = \
	../src/liblzma/check/crc32_x86.S \
	../src/liblzma/check/crc64_x86.S

OBJS_C = $(SRCS_C:.c=.o)
OBJS_ASM = $(SRCS_ASM:.S=.o)
OBJS = $(OBJS_C) $(OBJS_ASM)

getopt.h:
	update ../lib/getopt.in.h getopt.h

$(OBJS): getopt.h

$(OBJS_C): %.o: %.c
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<

$(OBJS_ASM): %.o: %.S
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<

# Make xz.exe not depend on an external DPMI server.
xz.exe: $(OBJS)
	$(CC) $(ALL_CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(STRIP) --strip-all $@
	exe2coff $@
	del $@
	copy /b $(DJGPP:DJGPP.ENV=BIN\CWSDSTUB.EXE) + $(@:.exe=) $@
	del $(@:.exe=)
