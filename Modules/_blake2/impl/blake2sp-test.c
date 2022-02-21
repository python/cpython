/*
   BLAKE2 reference source code package - optimized C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdio.h>
#include <string.h>
#include "blake2.h"
#include "blake2-kat.h"

int main( int argc, char **argv )
{
  uint8_t key[BLAKE2S_KEYBYTES];
  uint8_t buf[KAT_LENGTH];

  for( size_t i = 0; i < BLAKE2S_KEYBYTES; ++i )
    key[i] = ( uint8_t )i;

  for( size_t i = 0; i < KAT_LENGTH; ++i )
    buf[i] = ( uint8_t )i;

  for( size_t i = 0; i < KAT_LENGTH; ++i )
  {
    uint8_t hash[BLAKE2S_OUTBYTES];
    if( blake2sp( hash, buf, key, BLAKE2S_OUTBYTES, i, BLAKE2S_KEYBYTES ) < 0 ||
        0 != memcmp( hash, blake2sp_keyed_kat[i], BLAKE2S_OUTBYTES ) )
    {
      puts( "error" );
      return -1;
    }
  }

  puts( "ok" );
  return 0;
}

