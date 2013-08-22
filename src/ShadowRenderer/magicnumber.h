/*
  Reference implementations of computing and using the "magic number" approach to dividing
  by constants, including codegen instructions. The unsigned division incorporates the
  "round down" optimization per ridiculous_fish.

  This is free and unencumbered software. Any copyright is dedicated to the Public Domain.
*/

#ifndef MAGICNUMBER_H_
#define MAGICNUMBER_H_

#include <limits.h> //for CHAR_BIT
#include <stdint.h> // for short types...

/* Types used in the computations below. These can be redefined to the types appropriate
   for the desired division type (i.e. uint can be defined as unsigned long long).

   Note that the uint type is used in compute_signed_magic_info, so the uint type must
   not be smaller than the sint type.

typedef unsigned int uint;
typedef signed int sint;
*/

typedef uint16_t uint;
typedef int16_t sint;


/* Computes "magic info" for performing unsigned division by a fixed positive integer D.
   The type 'uint' is assumed to be defined as an unsigned integer type large enough
   to hold both the dividend and the divisor. num_bits can be set appropriately if n is
   known to be smaller than the largest uint; if this is not known then pass
   (sizeof(uint) * CHAR_BIT) for num_bits.

   Assume we have a hardware register of width UINT_BITS, a known constant D which is
   not zero and not a power of 2, and a variable n of width num_bits (which may be
   up to UINT_BITS). To emit code for n/d, use one of the two following sequences
   (here >>> refers to a logical bitshift):

     m = compute_unsigned_magic_info(D, num_bits)
     if m.pre_shift > 0: emit("n >>>= m.pre_shift")
     if m.increment: emit("n = saturated_increment(n)")
     emit("result = (m.multiplier * n) >>> UINT_BITS")
     if m.post_shift > 0: emit("result >>>= m.post_shift")

   or

     m = compute_unsigned_magic_info(D, num_bits)
     if m.pre_shift > 0: emit("n >>>= m.pre_shift")
     emit("result = m.multiplier * n")
     if m.increment: emit("result = result + m.multiplier")
     emit("result >>>= UINT_BITS")
     if m.post_shift > 0: emit("result >>>= m.post_shift")

  The shifts by UINT_BITS may be "free" if the high half of the full multiply
  is put in a separate register.

  saturated_increment(n) means "increment n unless it would wrap to 0," i.e.
    if n == (1 << UINT_BITS)-1: result = n
    else: result = n+1
  A common way to implement this is with the carry bit. For example, on x86:
     add 1
     sbb 0

  Some invariants:
   1: At least one of pre_shift and increment is zero
   2: multiplier is never zero

   This code incorporates the "round down" optimization per ridiculous_fish.
 */

struct magicu_info {
    uint multiplier; // the "magic number" multiplier
    unsigned pre_shift; // shift for the dividend before multiplying
    unsigned post_shift; //shift for the dividend after multiplying
    int increment; // 0 or 1; if set then increment the numerator, using one of the two strategies
};
struct magicu_info compute_unsigned_magic_info(uint D, unsigned num_bits);

#endif /* MAGICNUMBER_H_ */
