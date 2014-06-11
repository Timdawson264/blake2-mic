/*
   BLAKE2 reference source code package - optimized C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>
   Modified for the intel PHI in 2014 by Tim Dawson <tkd6@students.waikato.ac.nz>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "blake2-mic.h"
#include "blake2-impl.h"

#include "blake2s-round-mic.h"

#include <immintrin.h>


#define MIN(a,b) (((a)<(b))?(a):(b))
//#define CHANNEL 0

ALIGN( 64 ) static const uint32_t blake2s_IV[8] =
{
  0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
  0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

static const uint8_t blake2s_sigma[10][16] =
{
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
};

/* Scalar compress used for single stream compress (inital block. aka key) */
static int blake2s_compress_scalar( blake2s_state *S, const uint8_t* block, size_t CHANNEL)
{
  uint32_t m[16];
  uint32_t v[16];

  for( size_t i = 0; i < 4; ++i ){
    m[i] =   load32( block + (i*4)      +(16*CHANNEL));
    m[i+4] = load32( block + (i*4) +64  +(16*CHANNEL));
    m[i+8] = load32( block + (i*4) +128 +(16*CHANNEL));
    m[i+12] =load32( block + (i*4) +192 +(16*CHANNEL));
  }
                
  

  for( size_t i = 0; i < 4; ++i ){
    v[i] = S->h[i+(4*CHANNEL)];
    v[i+4] = S->h[i+(4*CHANNEL)+16];
  }
  
  v[ 8] = blake2s_IV[0];
  v[ 9] = blake2s_IV[1];
  v[10] = blake2s_IV[2];
  v[11] = blake2s_IV[3];
  //v[12] = S->t[0] ^ blake2s_IV[4];
  //v[13] = S->t[1] ^ blake2s_IV[5];
  //v[14] = S->f[0] ^ blake2s_IV[6];
  //v[15] = S->f[1] ^ blake2s_IV[7];
  v[12] = S->tf[0+(4*CHANNEL)] ^ blake2s_IV[4];
  v[13] = S->tf[1+(4*CHANNEL)] ^ blake2s_IV[5];
  v[14] = S->tf[2+(4*CHANNEL)] ^ blake2s_IV[6];
  v[15] = S->tf[3+(4*CHANNEL)] ^ blake2s_IV[7];
#ifdef DEBUG
  for(size_t x=0;x<4;x++)
   printf( "sm%u: %x%x%x%x\n", x,m[3+(4*x)],m[2+(4*x)],m[1+(4*x)],m[0+(4*x)]);
   
  printf( "%s: %x%x%x%x\n", "srow1",v[3],v[2],v[1],v[0]);
  printf( "%s: %x%x%x%x\n", "srow2",v[7],v[6],v[5],v[4]);
  printf( "%s: %x%x%x%x\n", "srow3",v[11],v[10],v[9],v[8]);
  printf( "%s: %x%x%x%x\n", "srow4",v[15],v[14],v[13],v[12]);
#endif
  
#define G(r,i,a,b,c,d) \
  do { \
    a = a + b + m[blake2s_sigma[r][2*i+0]]; \
    d = rotr32(d ^ a, 16); \
    c = c + d; \
    b = rotr32(b ^ c, 12); \
    a = a + b + m[blake2s_sigma[r][2*i+1]]; \
    d = rotr32(d ^ a, 8); \
    c = c + d; \
    b = rotr32(b ^ c, 7); \
  } while(0)
#define ROUND(r)  \
  do { \
    G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
    G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
    G(r,2,v[ 2],v[ 6],v[10],v[14]); \
    G(r,3,v[ 3],v[ 7],v[11],v[15]); \
    G(r,4,v[ 0],v[ 5],v[10],v[15]); \
    G(r,5,v[ 1],v[ 6],v[11],v[12]); \
    G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
    G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \
  } while(0)
  ROUND( 0 );
  ROUND( 1 );
  ROUND( 2 );
  ROUND( 3 );
  ROUND( 4 );
  ROUND( 5 );
  ROUND( 6 );
  ROUND( 7 );
  ROUND( 8 );
  ROUND( 9 );

  //for( size_t i = 0; i < 8; ++i )
 //   S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
  
  for( size_t i = 0; i < 4; ++i ){
    S->h[i+(4*CHANNEL)] = S->h[i+(4*CHANNEL)] ^ v[i] ^ v[i + 8] ;
    S->h[i+(4*CHANNEL)+16] = S->h[i+(4*CHANNEL)+16] ^ v[i+4] ^ v[i + 8 + 4] ;
  }


#undef G
#undef ROUND
  return 0;
}



/* Some helper functions, not necessarily useful */
static inline int blake2s_set_lastnode( blake2s_state *S, size_t CHANNEL )
{
  //S->f[1] = ~0U;
  S->tf[3+(4*CHANNEL)] = ~0U;
  printf("f1: %u\n",S->tf[3+(4*CHANNEL)]);
  return 0;
}

static inline int blake2s_clear_lastnode( blake2s_state *S, size_t CHANNEL )
{
  //S->f[1] = 0U;
  S->tf[3+(4*CHANNEL)] = 0U;
  return 0;
}

static inline int blake2s_set_lastblock( blake2s_state *S , size_t CHANNEL)
{
  if( S->last_node[CHANNEL] ) blake2s_set_lastnode( S, CHANNEL);
  //S->f[0] = ~0U;
  S->tf[2+(4*CHANNEL)] = ~0U;
  return 0;
}

static inline int blake2s_clear_lastblock( blake2s_state *S, size_t CHANNEL )
{
  if( S->last_node[CHANNEL] ) blake2s_clear_lastnode( S, CHANNEL);
  //S->f[0] = 0U;
  S->tf[2+(4*CHANNEL)] = 0U;
  return 0;
}

static inline int blake2s_increment_counter( blake2s_state *S, const uint32_t inc, size_t CHANNEL )
{
  uint64_t t = ( ( uint64_t )S->tf[1+(4*CHANNEL)] << 32 ) | S->tf[0+(4*CHANNEL)];
  t += inc;
  S->tf[0+(4*CHANNEL)] = ( uint32_t )( t >>  0 );
  S->tf[1+(4*CHANNEL)] = ( uint32_t )( t >> 32 );
  return 0;
}


// Parameter-related functions
static inline int blake2s_param_set_digest_length( blake2s_param *P, const uint8_t digest_length )
{
  P->digest_length = digest_length;
  return 0;
}

static inline int blake2s_param_set_fanout( blake2s_param *P, const uint8_t fanout )
{
  P->fanout = fanout;
  return 0;
}

static inline int blake2s_param_set_max_depth( blake2s_param *P, const uint8_t depth )
{
  P->depth = depth;
  return 0;
}

static inline int blake2s_param_set_leaf_length( blake2s_param *P, const uint32_t leaf_length )
{
  P->leaf_length = leaf_length;
  return 0;
}

static inline int blake2s_param_set_node_offset( blake2s_param *P, const uint64_t node_offset )
{
  store48( P->node_offset, node_offset );
  return 0;
}

static inline int blake2s_param_set_node_depth( blake2s_param *P, const uint8_t node_depth )
{
  P->node_depth = node_depth;
  return 0;
}

static inline int blake2s_param_set_inner_length( blake2s_param *P, const uint8_t inner_length )
{
  P->inner_length = inner_length;
  return 0;
}

static inline int blake2s_param_set_salt( blake2s_param *P, const uint8_t salt[BLAKE2S_SALTBYTES] )
{
  memcpy( P->salt, salt, BLAKE2S_SALTBYTES );
  return 0;
}

static inline int blake2s_param_set_personal( blake2s_param *P, const uint8_t personal[BLAKE2S_PERSONALBYTES] )
{
  memcpy( P->personal, personal, BLAKE2S_PERSONALBYTES );
  return 0;
}

//TODO - not priority
static inline int blake2s_init0( blake2s_state *S )
{
  memset( S, 0, sizeof( blake2s_state ) );

  for( size_t i = 0; i < 8; ++i ) S->h[i] = blake2s_IV[i];

  return 0;
}

/* init2 xors IV with input parameter block */
int blake2s_init_param( blake2s_state *S, const blake2s_param *P, size_t CHANNEL)
{
  uint8_t *p, *h, *v;
  //blake2s_init0( S );
  v = ( uint8_t * )( blake2s_IV );
  h = ( uint8_t * )( S->h );//32bytes
  p = ( uint8_t * )( P ); //32bytes
  /* IV XOR ParamBlock */
  //memset( S, 0, sizeof( blake2s_state ) );
  //cant blank entire state will distroy other streams
  for( size_t x=0; x<4; x++){
    S->tf[x+(4*CHANNEL)]=0;
  }
  S->buflen[CHANNEL]=0;
  S->last_node[CHANNEL]=0;
  memset( S->buf     +(16*CHANNEL), 0, 16);
  memset( S->buf+64  +(16*CHANNEL), 0, 16);
  memset( S->buf+128 +(16*CHANNEL), 0, 16);
  memset( S->buf+192 +(16*CHANNEL), 0, 16);
  
  for( size_t i = 0; i < 16 ; ++i ) {
    h[i+(16*CHANNEL)] = v[i] ^ p[i];
    h[i+64+(16*CHANNEL)] = v[i+16] ^ p[i+16];
  }
  return 0;
}


/* Some sort of default parameter block initialization, for sequential blake2s */
int blake2s_init( blake2s_state *S, const uint8_t outlen, size_t CHANNEL )
{
  /* Move interval verification here? */
  if ( ( !outlen ) || ( outlen > BLAKE2S_OUTBYTES ) ) return -1;

  const blake2s_param P =
  {
    outlen,
    0,
    1,
    1,
    0,
    {0},
    0,
    0,
    {0},
    {0}
  };
  return blake2s_init_param( S, &P, CHANNEL);
}


int blake2s_init_key( blake2s_state *S, const uint8_t outlen, const void *key, const uint8_t keylen, size_t CHANNEL )
{
  /* Move interval verification here? */
  if ( ( !outlen ) || ( outlen > BLAKE2S_OUTBYTES ) ) return -1;

  if ( ( !key ) || ( !keylen ) || keylen > BLAKE2S_KEYBYTES ) return -1;

  const blake2s_param P =
  {
    outlen,
    keylen,
    1,
    1,
    0,
    {0},
    0,
    0,
    {0},
    {0}
  };

  if( blake2s_init_param( S, &P, CHANNEL ) < 0)
    return -1;

  {
    uint8_t block[BLAKE2S_BLOCKBYTES];
    memset( block, 0, BLAKE2S_BLOCKBYTES );
    memcpy( block, key, keylen );
    
    //using scalar compress so only single channel compressed -- allows for different keys and stream restart
    blake2s_compress_scalar(S, block, CHANNEL);
    //blake2s_update( S, block, BLAKE2S_BLOCKBYTES ); //Tim: Replaced With compressed 
    secure_zero_memory( block, BLAKE2S_BLOCKBYTES ); /* Burn the key from stack */
  }
  return 0;
}

static inline void printf128(char* name, __m512i v){
    uint32_t v_a[16];
    _mm512_storeu_si512((void *)(v_a), v);
    size_t CHANNEL=0;
    printf( "%s: %x%x%x%x\n", name,v_a[3+(4*CHANNEL)],v_a[2+(4*CHANNEL)],v_a[1+(4*CHANNEL)],v_a[0+(4*CHANNEL)] );
}

#include "blake2s-load-mic.h"
static inline int blake2s_compress( blake2s_state *S, const uint8_t* block )
{
	__m512i row1, row2, row3, row4;
	__m512i buf1, buf2, buf3, buf4;

	__m512i t0, t1, t2;
	__m512i ff0, ff1;

    /*
	const __m128i m0 = LOADU( block +  00 );
	const __m128i m1 = LOADU( block +  16 );
	const __m128i m2 = LOADU( block +  32 );
    const __m128i m3 = LOADU( block +  48 );
    */
    
    //Strided Load 128bits in 64byte stride 
    const __m512i m0 = LOADU( block +  00 ); //load 4 messages at once
	const __m512i m1 = LOADU( block +  64 ); //messages will be interleaved on arrival to phi
	const __m512i m2 = LOADU( block +  128 );
    const __m512i m3 = LOADU( block +  192 );
    
    row1 = ff0 = LOADU( &S->h[0] );
	row2 = ff1 = LOADU( &S->h[16] );
	row3 = _mm512_set4_epi32( 0xA54FF53A, 0x3C6EF372, 0xBB67AE85,  0x6A09E667 );
	row4 = _mm512_xor_si512( _mm512_set4_epi32(0x5BE0CD19, 0x1F83D9AB, 0x9B05688C, 0x510E527F), LOADU( &S->tf[0] ) ); //loading s->t and s->f
    
    #if defined(DEBUG)  
        printf128("m0",m0); printf128("m1",m1); printf128("m2",m2); printf128("m3",m3);
        #define ROUND(r)  \
        printf("R%u\n", r);\
        printf128("row1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); \
        LOAD_MSG_ ##r ##_1(buf1); \
        printf128("MSG_1",buf1); \
        G1(row1,row2,row3,row4,buf1); \
        printf128("G1\nrow1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); \
        LOAD_MSG_ ##r ##_2(buf2); \
        printf128("MSG_2",buf2); \
        G2(row1,row2,row3,row4,buf2); \
        printf128("G2\nrow1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); \
        DIAGONALIZE(row1,row2,row3,row4); \
        printf128("DIAG\nrow1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); \
        LOAD_MSG_ ##r ##_3(buf3); \
        printf128("MSG_3",buf3); \
        G1(row1,row2,row3,row4,buf3); \
        printf128("G1\nrow1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); \
        LOAD_MSG_ ##r ##_4(buf4); \
        printf128("MSG_4",buf4); \
        G2(row1,row2,row3,row4,buf4); \
        printf128("G2\nrow1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); \
        UNDIAGONALIZE(row1,row2,row3,row4); \
        printf128("UNDIAG\nrow1",row1); printf128("row2",row2); printf128("row3",row3); printf128("row4",row4); 
    #else
        #define ROUND(r)  \
        LOAD_MSG_ ##r ##_1(buf1); \
        G1(row1,row2,row3,row4,buf1); \
        LOAD_MSG_ ##r ##_2(buf2); \
        G2(row1,row2,row3,row4,buf2); \
        DIAGONALIZE(row1,row2,row3,row4); \
        LOAD_MSG_ ##r ##_3(buf3); \
        G1(row1,row2,row3,row4,buf3); \
        LOAD_MSG_ ##r ##_4(buf4); \
        G2(row1,row2,row3,row4,buf4); \
        UNDIAGONALIZE(row1,row2,row3,row4);
    #endif
    
    
    ROUND( 0 );
    ROUND( 1 );
    ROUND( 2 );
    ROUND( 3 );
    ROUND( 4 );
    ROUND( 5 );
	ROUND( 6 );
    ROUND( 7 );
	ROUND( 8 );
	ROUND( 9 );
	STOREU( &S->h[0], _mm512_xor_si512( ff0, _mm512_xor_si512( row1, row3 ) ) );
	STOREU( &S->h[16], _mm512_xor_si512( ff1, _mm512_xor_si512( row2, row4 ) ) );
    
        
	return 0;
}

/* inlen now in bytes */
int blake2s_update( blake2s_state *S, const uint8_t ** in, uint64_t * inlen)
{
    //const uint8_t *in[] = {in0,in1,in2,in3};
    //uint64_t inlen[] = {inlen0,inlen1,inlen2,inlen3};
    
    //fprintf(stderr,"Starting Update: %u-%u\n",inlen[0],in[0][0]);
    

    
  while( inlen[0] > 0 && inlen[1] > 0 && inlen[2] > 0 && inlen[3] > 0)
  {
    if( inlen[0] > BLAKE2S_BLOCKBYTES && inlen[1] > BLAKE2S_BLOCKBYTES && inlen[2] > BLAKE2S_BLOCKBYTES && inlen[3] > BLAKE2S_BLOCKBYTES )
    {

    //INTERLEAVE_BUF_FILL;
    //fprintf(stderr,"STATUS_MEM: %u, %u, %u, %u\n",in[3],in[2],in[1],in[0]);
    if(in[0]!=NULL){             
        memcpy( S->buf,        in[0], 16);
        memcpy( S->buf+64,     in[0]+16, 16);
        memcpy( S->buf+128,    in[0]+32, 16);
        memcpy( S->buf+192,    in[0]+48, 16);
        in[0] += BLAKE2S_BLOCKBYTES;//move though input
        inlen[0] -= BLAKE2S_BLOCKBYTES;
    }
    if(in[1]!=NULL){
        memcpy( S->buf+16,     in[1], 16);
        memcpy( S->buf+64+16,  in[1]+16, 16);
        memcpy( S->buf+128+16, in[1]+32, 16);
        memcpy( S->buf+192+16, in[1]+48, 16);
        in[1] += BLAKE2S_BLOCKBYTES;//move though input
        inlen[1] -= BLAKE2S_BLOCKBYTES;
    }
    if(in[2]!=NULL){
        memcpy( S->buf+32,     in[2], 16);
        memcpy( S->buf+64+32 , in[2]+16, 16);
        memcpy( S->buf+128+32, in[2]+32, 16);
        memcpy( S->buf+192+32, in[2]+48, 16);
        in[2] += BLAKE2S_BLOCKBYTES;//move though input
        inlen[2] -= BLAKE2S_BLOCKBYTES;
    }
    if(in[3]!=NULL){
        memcpy( S->buf+48,     in[3], 16);
        memcpy( S->buf+64+48,  in[3]+16, 16);
        memcpy( S->buf+128+48, in[3]+32, 16);
        memcpy( S->buf+192+48, in[3]+48, 16);
        in[3] += BLAKE2S_BLOCKBYTES;//move though input
        inlen[3] -= BLAKE2S_BLOCKBYTES;
    }  
        
        
      blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES, 0);
      blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES, 1);
      blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES, 2);
      blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES, 3);
      
      blake2s_compress( S, S->buf ); // Compress

    }
    else // inlen <= fill
    {
        
        for(int CHANNEL=0;CHANNEL<4;CHANNEL++){
            //fprintf(stderr,"Checking Streams: %u-%u\n",CHANNEL,inlen[CHANNEL]);
            if(!(inlen[CHANNEL]> BLAKE2S_BLOCKBYTES)){
                if(in[CHANNEL]==NULL){
                     inlen[CHANNEL]=-1;//Make Channel Quite
                     continue;
                 }
                
                // !!!!!!!! Should Make this work for case when streams dont end at same time !!!!!!!!
                //Fix for update being called again on block boundry - use to be in final but no double block buffer any more

                if(S->buflen[CHANNEL]==BLAKE2S_BLOCKBYTES){
                    blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES, CHANNEL);
                    blake2s_compress_scalar( S, S->buf, CHANNEL);
                    S->buflen[CHANNEL] -= BLAKE2S_BLOCKBYTES;
                }
                
                S->buflen[CHANNEL] += inlen[CHANNEL]; 
                
                memcpy(S->buf     +(16*CHANNEL), in[CHANNEL],    MIN(16,inlen[CHANNEL]) );
                inlen[CHANNEL]-= MIN(16,inlen[CHANNEL]);
                memcpy(S->buf+64  +(16*CHANNEL), in[CHANNEL]+16, MIN(16,inlen[CHANNEL]));
                inlen[CHANNEL]-= MIN(16,inlen[CHANNEL]);
                memcpy(S->buf+128 +(16*CHANNEL), in[CHANNEL]+32, MIN(16,inlen[CHANNEL]));
                inlen[CHANNEL]-= MIN(16,inlen[CHANNEL]);
                memcpy(S->buf+192 +(16*CHANNEL), in[CHANNEL]+48, MIN(16,inlen[CHANNEL]));
                inlen[CHANNEL]-= MIN(16,inlen[CHANNEL]);
                
                

                in[CHANNEL] += S->buflen[CHANNEL];
                //inlen[CHANNEL] = 0;
                
                //Return so it can be finalized or continued
                return CHANNEL;
            }
            
        }
    }

  } 
  return 0;
}

/* Is this correct? */
int blake2s_final( blake2s_state *S, uint8_t *out, uint8_t outlen, size_t CHANNEL )
{
  //fprintf(stderr,"FINAL: %u\n",CHANNEL);
  uint8_t buffer[BLAKE2S_OUTBYTES];
  
  blake2s_increment_counter( S, (uint32_t)S->buflen[CHANNEL], CHANNEL);
  blake2s_set_lastblock( S, CHANNEL );
  
  //TODO Make Better !!
  memset( S->buf     +(16*CHANNEL) + MIN(S->buflen[CHANNEL],16), 0, (S->buflen[CHANNEL] < 16) ? 16 - S->buflen[CHANNEL] : 0 ); /* Padding - Zero */
  S->buflen[CHANNEL] = (S->buflen[CHANNEL]>16) ? (S->buflen[CHANNEL]-16) : 0;
  memset( S->buf+64  +(16*CHANNEL) + MIN(S->buflen[CHANNEL],16), 0, (S->buflen[CHANNEL] < 16) ? 16 - S->buflen[CHANNEL] : 0 ); /* Padding - Zero */
  S->buflen[CHANNEL] = (S->buflen[CHANNEL]>16) ? (S->buflen[CHANNEL]-16) : 0;
  memset( S->buf+128 +(16*CHANNEL) + MIN(S->buflen[CHANNEL],16), 0, (S->buflen[CHANNEL] < 16) ? 16 - S->buflen[CHANNEL] : 0 ); /* Padding - Zero */ 
  S->buflen[CHANNEL] = (S->buflen[CHANNEL]>16) ? (S->buflen[CHANNEL]-16) : 0;
  memset( S->buf+192 +(16*CHANNEL) + MIN(S->buflen[CHANNEL],16), 0, (S->buflen[CHANNEL] < 16) ? 16 - S->buflen[CHANNEL] : 0 ); /* Padding - Zero */
  
  
  
  
  //HMMM
  blake2s_compress_scalar( S, S->buf, CHANNEL); //Finish off this one stream
  //blake2s_compress( S, S->buf ); //Finish off this one stream

  for( size_t i = 0; i < 4; ++i ){ /* Output full hash to temp buffer - endianess */
    store32( buffer + (4*i), S->h[i+(4*CHANNEL)] );
    store32( buffer + (4*(i+4)) , S->h[i+16+(4*CHANNEL)] );
  }
  
  memcpy( out, buffer, outlen );
  return 0;
}

/* inlen, at least, should be uint64_t. Others can be size_t. */
#if 0 //Should Fix this Function -- Low Priority
int blake2s( uint8_t *out, const void *in, const void *key, const uint8_t outlen, const uint64_t inlen, uint8_t keylen )
{
  blake2s_state S[1];

  /* Verify parameters */
  if ( NULL == in ) return -1;

  if ( NULL == out ) return -1;

  if ( NULL == key ) keylen = 0; /* Fail here instead if keylen != 0 and key == NULL? */

  if( keylen > 0 )
  {
    if( blake2s_init_key( S, outlen, key, keylen ) < 0 ) return -1;
  }
  else
  {
    if( blake2s_init( S, outlen ) < 0 ) return -1;
  }

  blake2s_update( S, ( uint8_t * )in, inlen );
  blake2s_final( S, out, outlen );
  return 0;
}
#endif


