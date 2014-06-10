/*
   BLAKE2 reference source code package - b2sum tool

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#include "blake2.h"

/* This will help compatibility with coreutils */
int blake2s_stream( FILE *stream, void *resstream )
{
  int ret = -1;
  size_t sum, n;
  blake2s_state S[1];
  static const size_t buffer_length = 32768;
  uint8_t *buffer = ( uint8_t * )malloc( buffer_length );

  if( !buffer ) return -1;

  blake2s_init( S, BLAKE2S_OUTBYTES );

  while( 1 )
  {
    sum = 0;

    while( 1 )
    {
      n = fread( buffer + sum, 1, buffer_length - sum, stream );
      sum += n;

      if( buffer_length == sum )
        break;

      if( 0 == n )
      {
        if( ferror( stream ) )
          goto cleanup_buffer;

        goto final_process;
      }

      if( feof( stream ) )
        goto final_process;
    }

    blake2s_update( S, buffer, buffer_length );
  }

final_process:;

  if( sum > 0 ) blake2s_update( S, buffer, sum );

  blake2s_final( S, resstream, BLAKE2S_OUTBYTES );
  ret = 0;
cleanup_buffer:
  free( buffer );
  return ret;
}



typedef int ( *blake2fn )( FILE *, void * );


static void usage( char **argv )
{
  fprintf( stderr, "Usage: %s [FILE]...\n", argv[0] );
  exit( 111 );
}


int main( int argc, char **argv )
{
  blake2fn blake2_stream = blake2s_stream;
  size_t outlen   = BLAKE2B_OUTBYTES;
  unsigned char hash[BLAKE2B_OUTBYTES] = {0};
  int c;
  opterr = 1;

  if ( argc == 1 ) usage( argv ); /* show usage upon no-argument */
  
  blake2_stream = blake2s_stream;
  outlen = BLAKE2S_OUTBYTES;
  printf("SSE/ref Version\n");

  for( int i = 1; i < argc; ++i )
  {
    FILE *f = NULL;
    f = fopen( argv[i], "rb" );

    if( !f )
    {
      fprintf( stderr, "Could not open `%s': %s\n", argv[i], strerror( errno ) );
      goto end0;
    }

    if( blake2_stream( f, hash ) < 0 )
    {
      fprintf( stderr, "Failed to hash `%s'\n", argv[i] );
      goto end1;
    }

    for( int j = 0; j < outlen; ++j )
      printf( "%02x", hash[j] );

    printf( " %s\n", argv[i] );
end1:
    fclose( f );
end0: ;
  }

  return 0;
}

