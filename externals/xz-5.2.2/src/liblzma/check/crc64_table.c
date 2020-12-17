///////////////////////////////////////////////////////////////////////////////
//
/// \file       crc64_table.c
/// \brief      Precalculated CRC64 table with correct endianness
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "common.h"

// Having the declaration here silences clang -Wmissing-variable-declarations.
extern const uint64_t lzma_crc64_table[4][256];

#ifdef WORDS_BIGENDIAN
#	include "crc64_table_be.h"
#else
#	include "crc64_table_le.h"
#endif
