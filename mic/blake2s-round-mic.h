/*
   BLAKE2 reference source code package - optimized C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#pragma once
#ifndef __BLAKE2S_ROUND_H__
#define __BLAKE2S_ROUND_H__

#define LOAD(p)  _mm512_load_epi32( (void *)(p) )
#define STORE(p,r) _mm512_store_epi32((void *)(p), r)

#define LOADU(p)  _mm512_load_epi32( (void *)(p) )
#define STOREU(p,r) _mm512_store_epi32((void *)(p), r)

#define TOF(reg) _mm512_castsi512_ps((reg))
#define TOI(reg) _mm512_castps_si512((reg))

#define LIKELY(x) __builtin_expect((x),1)


/*
#define _mm_roti_epi32(r, c) ( \
                (8==-(c)) ? _mm_shuffle_epi8(r,r8) \
              : (16==-(c)) ? _mm_shuffle_epi8(r,r16) \
              : _mm_xor_si128(_mm_srli_epi32( (r), -(c) ),_mm_slli_epi32( (r), 32-(-(c)) )) )
*/


#define roti_epi32(r, c) _mm512_xor_si512( _mm512_srli_epi32( r, -(c) ) , _mm512_slli_epi32( r, 32-(-(c)) ) ) 



#define G1(row1,row2,row3,row4,buf) \
  row1 = _mm512_add_epi32( _mm512_add_epi32( row1, buf), row2 ); \
  row4 = _mm512_xor_si512( row4, row1 ); \
  row4 = roti_epi32(row4, -16); \
  row3 = _mm512_add_epi32( row3, row4 );   \
  row2 = _mm512_xor_si512( row2, row3 ); \
  row2 = roti_epi32(row2, -12);

#define G2(row1,row2,row3,row4,buf) \
  row1 = _mm512_add_epi32( _mm512_add_epi32( row1, buf), row2 ); \
  row4 = _mm512_xor_si512( row4, row1 ); \
  row4 = roti_epi32(row4, -8); \
  row3 = _mm512_add_epi32( row3, row4 );   \
  row2 = _mm512_xor_si512( row2, row3 ); \
  row2 = roti_epi32(row2, -7);

#define DIAGONALIZE(row1,row2,row3,row4) \
  row4 = _mm512_shuffle_epi32( row4, _MM_SHUFFLE(2,1,0,3) ); \
  row3 = _mm512_shuffle_epi32( row3, _MM_SHUFFLE(1,0,3,2) ); \
  row2 = _mm512_shuffle_epi32( row2, _MM_SHUFFLE(0,3,2,1) );

#define UNDIAGONALIZE(row1,row2,row3,row4) \
  row4 = _mm512_shuffle_epi32( row4, _MM_SHUFFLE(0,3,2,1) ); \
  row3 = _mm512_shuffle_epi32( row3, _MM_SHUFFLE(1,0,3,2) ); \
  row2 = _mm512_shuffle_epi32( row2, _MM_SHUFFLE(2,1,0,3) );


#include "blake2s-load-mic.h"


 
#endif

