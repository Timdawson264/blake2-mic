/*
   BLAKE2 reference source code package - optimized C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "blake2.h"
#include "blake2-impl.h"

#include "blake2s-round.h"

#include <immintrin.h>


#define MIN(a,b) (((a)<(b))?(a):(b))

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


/* Some helper functions, not necessarily useful */
static inline int blake2s_set_lastnode( blake2s_state *S )
{
  //S->f[1] = ~0U;
  S->tf[3] = ~0U;
  return 0;
}

static inline int blake2s_clear_lastnode( blake2s_state *S )
{
  //S->f[1] = 0U;
  S->tf[3] = 0U;
  return 0;
}

static inline int blake2s_set_lastblock( blake2s_state *S )
{
  if( S->last_node ) blake2s_set_lastnode( S );
  //S->f[0] = ~0U;
  S->tf[2] = ~0U;
  return 0;
}

static inline int blake2s_clear_lastblock( blake2s_state *S )
{
  if( S->last_node ) blake2s_clear_lastnode( S );
  //S->f[0] = 0U;
  S->tf[2] = 0U;
  return 0;
}

static inline int blake2s_increment_counter( blake2s_state *S, const uint32_t inc )
{
  uint64_t t = ( ( uint64_t )S->tf[1] << 32 ) | S->tf[0];
  t += inc;
  S->tf[0] = ( uint32_t )( t >>  0 );
  S->tf[1] = ( uint32_t )( t >> 32 );
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

static inline int blake2s_init0( blake2s_state *S )
{
  memset( S, 0, sizeof( blake2s_state ) );

  for( int i = 0; i < 8; ++i ) S->h[i] = blake2s_IV[i];

  return 0;
}

/* init2 xors IV with input parameter block */
int blake2s_init_param( blake2s_state *S, const blake2s_param *P )
{
  uint8_t *p, *h, *v;
  //blake2s_init0( S );
  v = ( uint8_t * )( blake2s_IV );
  h = ( uint8_t * )( S->h );//32bytes
  p = ( uint8_t * )( P ); //32bytes
  /* IV XOR ParamBlock */
  memset( S, 0, sizeof( blake2s_state ) );
  //for( int i = 0; i < (BLAKE2S_OUTBYTES) ; ++i ) {
  for( int i = 0; i < 16 ; ++i ) {
    h[i] = v[i] ^ p[i];
    h[i+64] = v[i+16] ^ p[i+16];
  }
  return 0;
}


/* Some sort of default parameter block initialization, for sequential blake2s */
int blake2s_init( blake2s_state *S, const uint8_t outlen )
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
  return blake2s_init_param( S, &P );
}


int blake2s_init_key( blake2s_state *S, const uint8_t outlen, const void *key, const uint8_t keylen )
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

  if( blake2s_init_param( S, &P ) < 0 )
    return -1;

  {
    uint8_t block[BLAKE2S_BLOCKBYTES];
    memset( block, 0, BLAKE2S_BLOCKBYTES );
    memcpy( block, key, keylen );
    blake2s_update( S, block, BLAKE2S_BLOCKBYTES );
    secure_zero_memory( block, BLAKE2S_BLOCKBYTES ); /* Burn the key from stack */
  }
  return 0;
}

static inline void printf128(char* name, __m512i v){
    uint32_t v_a[16];
    _mm512_storeu_si512((void *)(v_a), v);
    printf( "%s: %x%x%x%x\n", name,v_a[3],v_a[2],v_a[1],v_a[0] );
    
}

#include "blake2s-load-sse41.h"
static inline int blake2s_compress( blake2s_state *S, const uint8_t block[BLAKE2S_BLOCKBYTES] )
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
int blake2s_update( blake2s_state *S, const uint8_t *in, uint64_t inlen )
{
  while( inlen > 0 )
  {
    if( inlen > BLAKE2S_BLOCKBYTES )
    {
      memcpy( S->buf, in, 16); //interleaved Fill buffer
      memcpy( S->buf+64, in+16, 16);
      memcpy( S->buf+128, in+32, 16);
      memcpy( S->buf+192, in+48, 16);
            
      S->buflen += BLAKE2S_BLOCKBYTES;
      blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES );
      blake2s_compress( S, S->buf ); // Compress
      S->buflen -= BLAKE2S_BLOCKBYTES;
      in += BLAKE2S_BLOCKBYTES;//move though input
      inlen -= BLAKE2S_BLOCKBYTES;
    }
    else // inlen <= fill
    {
        //Fix for update being called again on block boundry - use to be in final but no double block buffer any more
      if(S->buflen==BLAKE2S_BLOCKBYTES){
        blake2s_increment_counter( S, BLAKE2S_BLOCKBYTES );
        blake2s_compress( S, S->buf );
        S->buflen -= BLAKE2S_BLOCKBYTES;
      }
      memcpy( S->buf, in, 16); //interleaved Fill buffer
      memcpy( S->buf+64, in+16, 16);
      memcpy( S->buf+128, in+32, 16);
      memcpy( S->buf+192, in+48, 16);
      
      S->buflen += inlen; 
      in += inlen;
      inlen -= inlen;
    }
    
  }

  return 0;
}

/* Is this correct? */
int blake2s_final( blake2s_state *S, uint8_t *out, uint8_t outlen )
{
  uint8_t buffer[BLAKE2S_OUTBYTES];
  
  blake2s_increment_counter( S, ( uint32_t )S->buflen );
  blake2s_set_lastblock( S );
  
  fprintf(stderr,"pad1 %u\n", (S->buflen < 16) ? 16 - S->buflen : 0);
  fprintf(stderr,"pad2 %u\n", (S->buflen < 32) ? MIN(32 - S->buflen,16) : 0);
  fprintf(stderr,"pad3 %u\n", (S->buflen < 48) ? MIN(48 - S->buflen,16) : 0);
  fprintf(stderr,"pad4 %u\n", (S->buflen < 64) ? MIN(64 - S->buflen,16) : 0);
  
  memset( S->buf     + S->buflen, 0, (S->buflen < 16) ? 16 - S->buflen : 0 ); /* Padding - Zero */
  memset( S->buf+63  + S->buflen, 0, (S->buflen < 32) ? MIN(32 - S->buflen,16) : 0 ); /* Padding - Zero */
  memset( S->buf+127 + S->buflen, 0, (S->buflen < 48) ? MIN(48 - S->buflen,16) : 0 ); /* Padding - Zero */
  memset( S->buf+191 + S->buflen, 0, (S->buflen < 64) ? MIN(64 - S->buflen,16) : 0 ); /* Padding - Zero */
  
  
  fprintf(stderr,"FINAL Buflen: %u\n",S->buflen);
  blake2s_compress( S, S->buf );

  for( int i = 0; i < 4; ++i ){ /* Output full hash to temp buffer - endianess */
    store32( buffer + (4*i), S->h[i] );
    store32( buffer + (4*(i+4)) , S->h[i+16] );
  }
  
  memcpy( out, buffer, outlen );
  return 0;
}

/* inlen, at least, should be uint64_t. Others can be size_t. */
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

#if defined(SUPERCOP)
int crypto_hash( unsigned char *out, unsigned char *in, unsigned long long inlen )
{
  return blake2s( out, in, NULL, BLAKE2S_OUTBYTES, inlen, 0 );
}
#endif

#if defined(BLAKE2S_SELFTEST)
#include <string.h>
#include "blake2-kat.h"
int main( int argc, char **argv )
{
  uint8_t key[BLAKE2S_KEYBYTES];
  uint8_t buf[KAT_LENGTH];

  for( size_t i = 0; i < BLAKE2S_KEYBYTES; ++i )
    key[i] = ( uint8_t )i; //key is 0 to BLAKE2S_KEYBYTES

  for( size_t i = 0; i < KAT_LENGTH; ++i )
    buf[i] = ( uint8_t )i; //test data is just sequace of numbers

  for( size_t i = 0; i < KAT_LENGTH; ++i )
  {
    uint8_t hash[BLAKE2S_OUTBYTES];

    if( blake2s( hash, buf, key, BLAKE2S_OUTBYTES, i, BLAKE2S_KEYBYTES ) < 0 ||
        0 != memcmp( hash, blake2s_keyed_kat[i], BLAKE2S_OUTBYTES ) )
    {
      puts( "error" );
      return -1;
    }
  }

  puts( "ok" );
  return 0;
}
#endif


