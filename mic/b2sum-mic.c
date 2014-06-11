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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "blake2-mic.h"


static void usage( char **argv )
{
  fprintf( stderr, "Usage: %s [FILE]...\n", argv[0] );
  exit( 111 );
}


int main( int argc, char **argv )
{
  size_t outlen   = BLAKE2S_OUTBYTES;
  unsigned char hash[BLAKE2S_OUTBYTES] = {0};
  int c;
   
  printf("PHI Version\n");

  

  if ( argc == 1 ) usage( argv ); /* show usage upon no-argument */

  for( size_t i = 1; i < argc; ++i )
  {
    FILE *f = NULL;
    f = fopen( argv[i], "rb" );
    fseek (f, 0, SEEK_END);
    uint64_t fileLength[] = {ftell ( f ),-1,-1,-1};
    fclose(f);
    int pf = open( argv[i] , O_RDONLY);
    void* stream[] = {mmap(NULL, fileLength[0],  PROT_READ, MAP_PRIVATE , pf, 0),NULL,NULL,NULL};
    
    blake2s_state S[1];
    blake2s_init( S, BLAKE2S_OUTBYTES, 0);
    blake2s_init( S, BLAKE2S_OUTBYTES, 1);
    blake2s_init( S, BLAKE2S_OUTBYTES, 2);
    blake2s_init( S, BLAKE2S_OUTBYTES, 3);
    //Hash The Same File in all Streams
    //Finalize all four streams
    
    for(size_t x=1;x<4;x++){
        stream[x]=stream[0];
        fileLength[x]=fileLength[0];
    }
    
    for(int x=0;x<4;x++){
        int CHANNEL = blake2s_update( S, stream, fileLength);
        fprintf(stderr,"Done CHAN: %u\n",CHANNEL);    
        blake2s_final( S, hash, BLAKE2S_OUTBYTES, CHANNEL);   
        
        //output hash then filename
        for( size_t j = 0; j < outlen; ++j )
            printf( "%02x", hash[j] );
        printf( " %s\n", argv[i] );
        
        //Null Stream - TODO Next File
        stream[CHANNEL]=NULL;
        fileLength[CHANNEL]=-1;
    }

    
    munmap(stream[0], fileLength[0]);

  }

  return 0;
}




