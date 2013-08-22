
/*
  Reference implementations of computing and using the "magic number" approach to dividing
  by constants, including codegen instructions. The unsigned division incorporates the
  "round down" optimization per ridiculous_fish.

  This is free and unencumbered software. Any copyright is dedicated to the Public Domain.
*/

#include "magicnumber.h"
#include <assert.h>

/* Implementations follow */

struct magicu_info compute_unsigned_magic_info(uint D, unsigned num_bits) {

    //The numerator must fit in a uint
    assert(num_bits > 0 && num_bits <= sizeof(uint) * CHAR_BIT);

    // D must be larger than zero and not a power of 2
    assert(D & (D-1));

    // The eventual result
    struct magicu_info result;

    // Bits in a uint
    const unsigned UINT_BITS = sizeof(uint) * CHAR_BIT;

    // The extra shift implicit in the difference between UINT_BITS and num_bits
    const unsigned extra_shift = UINT_BITS - num_bits;

    // The initial power of 2 is one less than the first one that can possibly work
    const uint initial_power_of_2 = (uint)1 << (UINT_BITS-1);

    // The remainder and quotient of our power of 2 divided by d
    uint quotient = initial_power_of_2 / D, remainder = initial_power_of_2 % D;

    // ceil(log_2 D)
    unsigned ceil_log_2_D;

    // The magic info for the variant "round down" algorithm
    uint down_multiplier = 0;
    unsigned down_exponent = 0;
    int has_magic_down = 0;

    // Compute ceil(log_2 D)
    ceil_log_2_D = 0;
    uint tmp;
    for (tmp = D; tmp > 0; tmp >>= 1)
        ceil_log_2_D += 1;


    // Begin a loop that increments the exponent, until we find a power of 2 that works.
    unsigned exponent;
    for (exponent = 0; ; exponent++) {
        // Quotient and remainder is from previous exponent; compute it for this exponent.
        if (remainder >= D - remainder) {
            // Doubling remainder will wrap around D
            quotient = quotient * 2 + 1;
            remainder = remainder * 2 - D;
        } else {
            // Remainder will not wrap
            quotient = quotient * 2;
            remainder = remainder * 2;
        }

        // We're done if this exponent works for the round_up algorithm.
        // Note that exponent may be larger than the maximum shift supported,
        // so the check for >= ceil_log_2_D is critical.
        if ((exponent + extra_shift >= ceil_log_2_D) || (D - remainder) <= ((uint)1 << (exponent + extra_shift)))
            break;

        // Set magic_down if we have not set it yet and this exponent works for the round_down algorithm
        if (! has_magic_down && remainder <= ((uint)1 << (exponent + extra_shift))) {
            has_magic_down = 1;
            down_multiplier = quotient;
            down_exponent = exponent;
        }
    }

    if (exponent < ceil_log_2_D) {
        // magic_up is efficient
        result.multiplier = quotient + 1;
        result.pre_shift = 0;
        result.post_shift = exponent;
        result.increment = 0;
    } else if (D & 1) {
        // Odd divisor, so use magic_down, which must have been set
        assert(has_magic_down);
        result.multiplier = down_multiplier;
        result.pre_shift = 0;
        result.post_shift = down_exponent;
        result.increment = 1;
    } else {
        // Even divisor, so use a prefix-shifted dividend
        unsigned pre_shift = 0;
        uint shifted_D = D;
        while ((shifted_D & 1) == 0) {
            shifted_D >>= 1;
            pre_shift += 1;
        }
        result = compute_unsigned_magic_info(shifted_D, num_bits - pre_shift);
        assert(result.increment == 0 && result.pre_shift == 0); //expect no increment or pre_shift in this path
        result.pre_shift = pre_shift;
    }
    return result;
}
