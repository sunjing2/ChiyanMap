/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.
   
   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.
   
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
*/

#include <stdio.h>
#include "mt.h"

#define SIZE 624
#define PERIOD 397
#define DIFF (SIZE - PERIOD)

static const uint32_t MAGIC = 0x9908b0df;

struct MTState_
{
  uint32_t MT[SIZE];
  size_t index;
  size_t index_fast; /* lazy init progress */
};

typedef struct MTState_ MTState;

#ifdef _MSC_VER
__declspec(thread) static MTState state;
__declspec(thread) uint64_t mt_ws = (uint64_t)-1;
__declspec(thread) int mt_a;
__declspec(thread) int mt_b;
#else
static __thread MTState state;
__thread uint64_t mt_ws = (uint64_t)-1;
__thread int mt_a;
__thread int mt_b;
#endif

#define M32(x) ((x) & 0x80000000)
#define L31(x) ((x) & 0x7FFFFFFF)
#define MIXBITS(u,v) (M32(u) | L31(v))
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v & 1) ? MAGIC : 0))

static inline uint32_t generate()
{
  size_t idx = state.index;
  if (idx == SIZE) idx = 0;

  if (idx >= DIFF)
  {
    if (idx >= SIZE - 1)
      state.MT[SIZE-1] = state.MT[PERIOD-1] ^ TWIST(state.MT[SIZE-1], state.MT[0]);
    else
      state.MT[idx] = state.MT[idx - DIFF] ^ TWIST(state.MT[idx], state.MT[idx+1]);
  }
  else 
  {
    state.MT[idx] = state.MT[idx + PERIOD] ^ TWIST(state.MT[idx], state.MT[idx+1]);
    /* advance lazy init one slot */
    if (state.index_fast < SIZE)
    {
      state.MT[state.index_fast] = 0x6c078965
        * (state.MT[state.index_fast-1] ^ (state.MT[state.index_fast-1] >> 30))
        + state.index_fast;
      state.index_fast++;
    }
  }

  state.index = idx + 1;
  return state.MT[idx];
}

static inline uint32_t temper(uint32_t y)
{
  y ^= y >> 11;
  y ^= (y << 7) & 0x9d2c5680;
  y ^= (y << 15) & 0xefc60000;
  y ^= y >> 18;
  return y;
}

void mt_seed(uint32_t value)
{
  state.MT[0] = value;
  
  for (uint_fast32_t i = 1; i <= PERIOD; ++i)
    state.MT[i] = 0x6c078965 * (state.MT[i - 1] ^ (state.MT[i - 1] >> 30)) + i;
  
  state.index_fast = PERIOD + 1;
  state.index = SIZE;
}

uint32_t mt_next()
{
  if (state.index > SIZE) {
    /* called without mt_seed(): full init with default seed */
    for (uint_fast32_t i = 1; i < SIZE; ++i)
      state.MT[i] = 0x6c078965 * (state.MT[i - 1] ^ (state.MT[i - 1] >> 30)) + i;
    state.index_fast = SIZE;
    state.index = SIZE;
  }
  
  return temper(generate());
}

void mt_get_state(MTRngState *out)
{
  size_t i;
  for (i = 0; i < SIZE; i++)
    out->MT[i] = state.MT[i];
  out->index = state.index;
  out->index_fast = state.index_fast;
  out->ws = mt_ws;
  out->a = mt_a;
  out->b = mt_b;
}

void mt_set_state(const MTRngState *in)
{
  size_t i;
  for (i = 0; i < SIZE; i++)
    state.MT[i] = in->MT[i];
  state.index = in->index;
  state.index_fast = in->index_fast;
  mt_ws = in->ws;
  mt_a = in->a;
  mt_b = in->b;
}
