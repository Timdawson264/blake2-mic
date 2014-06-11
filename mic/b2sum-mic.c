
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


static uint64_t GetLength(const char* name){
    FILE *f = NULL;
    f = fopen( name, "rb" );
    fseek (f, 0, SEEK_END);
    uint64_t fileLength = ftell ( f );
    fclose(f);
    return fileLength;
}


int main( int argc, char **argv ){
    printf("PHI Version\n");
    unsigned char hash[BLAKE2S_OUTBYTES] = {0};

    int pf[4];
    uint64_t fileLength[4];
    const uint8_t * stream[4] = {NULL,NULL,NULL,NULL}; 
    char * FileNames[4];
    size_t fileNum=1;
    blake2s_state S[1];
        
    if ( argc == 1 ) usage( argv ); // show usage upon no-argument 
    
    while(1){
        //prime streams
        for(size_t x=0;x<4;x++){
            if(stream[x]==NULL && fileNum<argc){
                FileNames[x] = argv[fileNum];
                pf[x] = open( FileNames[x] , O_RDONLY);
                fileLength[x] = GetLength(FileNames[x]);
                stream [x] = mmap(NULL, fileLength[x],  PROT_READ, MAP_PRIVATE , pf[x], 0);
                fileNum++;
                //fprintf(stderr,"ADDED %s,%u\n",FileNames[x],fileLength[x]);
                //Can Do Key Stuff Here
                blake2s_init( S, BLAKE2S_OUTBYTES, x);
                
            }
        }
        if(stream[0] == NULL && stream[1] == NULL && stream[2] == NULL && stream[3] == NULL)
            return 0;
        
        //fprintf(stderr,"STATUS: %u, %u, %u, %u\n",stream[3],stream[2],stream[1],stream[0]);
        
        //Run Hash
        int CHANNEL = blake2s_update( S, stream, fileLength);
        //fprintf(stderr,"Done CHAN: %u\n",CHANNEL);    
        blake2s_final( S, hash, BLAKE2S_OUTBYTES, CHANNEL);   

        //output hash then filename
        for( size_t j = 0; j < BLAKE2S_OUTBYTES; ++j )
            printf( "%02x", hash[j] );
        printf( " %s\n", FileNames[CHANNEL] );
        
        munmap((void*)stream[CHANNEL], fileLength[CHANNEL]);
        close(pf[CHANNEL]);
        stream[CHANNEL]=NULL;
        fileLength[CHANNEL]=-1;
    }
    
    
}


/*
int main( int argc, char **argv )
{


    

    for( size_t i = 1; i < argc; ++i )
    {





        //Hash The Same File in all Streams
        //Finalize all four streams
        for(int x=0;x<4;x++){

        }

        for(size_t x=0;x<4;x++){
           
        }
    }

    return 0;
}

*/


