#include "gaussianblur.h"
#include <arm_neon.h>
#include <math.h>
#include "magicnumber.h"

#if 1
//
// box blur a square array of pixels (power of 2, actually)
// if we insist on powers of 2, we don't need to special case some end-of-row/col conditions
// to a specific blur width
//
// also, we're using NEON to vectorize our arithmetic.
// we need to do a division along the way, but NEON doesn't support integer division.
// so rather than divide by, say "w", we multiply by magic(w).
// magic(w) is chosen so that the result of multiplying by it will be the same as
// dividing by w, except that the result will be in the high half of the result.
// yes, dorothy... this is what compilers do, too...
void NEONboxBlur(pixel *src, pixel *dest, unsigned int size, unsigned int blurRad) {
	unsigned int wid = 2 * blurRad + 1;

	// because NEON doesn't have integer division, we use "magic constants" that will give
	// use the result of division by multiplication -- the upper half of the result will be
	// (more or less) the result of the division.
	// for this, we need to compute the magic numbers corresponding to a given divisor

	struct magicu_info minfo = compute_unsigned_magic_info(wid, 16);

	int16x8_t preshift  = vdupq_n_s16(-minfo.pre_shift); // negative means shift right
	int32x4_t postshift = vdupq_n_s32(-(minfo.post_shift+16)); // negative means shift right
	uint16x4_t magic    = vdup_n_u16(minfo.multiplier);

//	fprintf(stderr,"width %5d, preshift %d, postshift %d + 16, increment %d, magic %d\n", wid,
//			minfo.pre_shift, minfo.post_shift, minfo.increment, minfo.multiplier);

//	if (minfo.pre_shift > 0) fprintf(stderr,"hey, not an odd number!\n");

	int i, j, k, ch;
	for (i = 0 ; i < size ; i+=8) {
		// first, initialize the sum so that we can loop from 0 to size-1

		// we'll initialize boxsum for index -1, so that we can move into 0 as part of our loop
		uint16x8x4_t boxsum;
		uint8x8x4_t firstpixel = vld4_u8((uint8_t *)(src + 0 * size + i));
		for (ch = 0 ; ch < 4 ; ch++) {
			// boxsum[ch] = blurRad * srcpixel[ch]
			boxsum.val[ch] = vmulq_n_u16(vmovl_u8(firstpixel.val[ch]),(blurRad+1)+1);
		}
		for ( k = 1 ; k < blurRad ; k++) {
			uint8x8x4_t srcpixel = vld4_u8((uint8_t *)(src + k * size + i));
			for (ch = 0 ; ch < 4 ; ch++ ) {
				boxsum.val[ch] = vaddw_u8(boxsum.val[ch], srcpixel.val[ch]);
			}
		}

		int right = blurRad-1;
		int left = -blurRad-1;

		if (minfo.increment) {
			for ( k = 0 ; k < size ; k++) {
				// move to next pixel
				unsigned int l = (left < 0)?0:left; // take off the old left
				left++;
				right++;
				unsigned int r = (right < size)?right:(size-1); // but add the new right

				uint8x8x4_t addpixel = vld4_u8((uint8_t *)(src + r * size + i));
				uint8x8x4_t subpixel = vld4_u8((uint8_t *)(src + l * size + i));
				for (ch = 0 ; ch < 4 ; ch++ ) {
					// boxsum[ch] += addpixel[ch] - subpixel[ch];
					boxsum.val[ch] = vsubw_u8(vaddw_u8(boxsum.val[ch], addpixel.val[ch]), subpixel.val[ch]);
				}

				uint8x8x4_t destpixel;
				for (ch = 0 ; ch < 4 ; ch++ ) { // compute: destpixel = boxsum / wid
					// since 16bit multiplication leads to 32bit results, we need to
					// split our task into two chunks, for the hi and low half of our vector
					// (because otherwise, it won't all fit into 128 bits)

					// this is the meat of the magic division algorithm (see the include file...)
					uint16x8_t bsum_preshifted = vshlq_u16(boxsum.val[ch],preshift);

					// multiply by the magic number
					uint32x4_t res_hi = vmull_u16(vget_high_u16(bsum_preshifted), magic);
					res_hi = vaddw_u16(res_hi, magic);
					// take the high half and post-shift
					uint16x4_t q_hi = vmovn_u32(vshlq_u32(res_hi, postshift));

					// pre-shift and multiply by the magic number
					uint32x4_t res_lo = vmull_u16(vget_low_u16(bsum_preshifted), magic);
					res_lo = vaddw_u16(res_lo, magic);
					// take the high half and post-shift
					uint16x4_t q_lo = vmovn_u32(vshlq_u32(res_lo, postshift));

					destpixel.val[ch] = vqmovn_u16(vcombine_u16(q_lo, q_hi));
				}
				pixel block[8];
				vst4_u8((uint8_t *)&block, destpixel);
				for (j = 0 ; j < 8 ; j++ ) {
					dest[(i + j)*size + k] = block[j];
				}
				//			vst4_u8((uint8_t *)(dest + k * size + i), destpixel);
			}
		} else {
			for ( k = 0 ; k < size ; k++) {
				// move to next pixel
				unsigned int l = (left < 0)?0:left; // take off the old left
				left++;
				right++;
				unsigned int r = (right < size)?right:(size-1); // but add the new right

				uint8x8x4_t addpixel = vld4_u8((uint8_t *)(src + r * size + i));
				uint8x8x4_t subpixel = vld4_u8((uint8_t *)(src + l * size + i));
				for (ch = 0 ; ch < 4 ; ch++ ) {
					// boxsum[ch] += addpixel[ch] - subpixel[ch];
					boxsum.val[ch] = vsubw_u8(vaddw_u8(boxsum.val[ch], addpixel.val[ch]), subpixel.val[ch]);
				}

				uint8x8x4_t destpixel;
				for (ch = 0 ; ch < 4 ; ch++ ) { // compute: destpixel = boxsum / wid
					// since 16bit multiplication leads to 32bit results, we need to
					// split our task into two chunks, for the hi and low half of our vector
					// (because otherwise, it won't all fit into 128 bits)

					// this is the meat of the magic division algorithm (see the include file...)
					uint16x8_t bsum_preshifted = vshlq_u16(boxsum.val[ch],preshift);

					// multiply by the magic number
					// take the high half and post-shift
					uint32x4_t res_hi = vmull_u16(vget_high_u16(bsum_preshifted), magic);
					uint16x4_t q_hi = vmovn_u32(vshlq_u32(res_hi, postshift));

					// multiply by the magic number
					// take the high half and post-shift
					uint32x4_t res_lo = vmull_u16(vget_low_u16(bsum_preshifted), magic);
					uint16x4_t q_lo = vmovn_u32(vshlq_u32(res_lo, postshift));

					destpixel.val[ch] = vqmovn_u16(vcombine_u16(q_lo, q_hi));
				}
				pixel block[8];
				vst4_u8((uint8_t *)&block, destpixel);
				for (j = 0 ; j < 8 ; j++ ) {
					dest[(i + j)*size + k] = block[j];
				}
				//			vst4_u8((uint8_t *)(dest + k * size + i), destpixel);
			}
		}
	}
}

#endif


//
// box blur a square array of pixels (power of 2, actually)
// to a specific blur width
void boxBlur(pixel *src, pixel *dest, unsigned int size, unsigned int blurRad) {
	// for now, just copy
	int wid = 2 * blurRad + 1;

	int i, j, k;
	for (i = 0 ; i < size ; i++) {
//		for ( j = 0 ; j < size ; j++) {
//			dest[j + size * i] = src[i + size * j];
//		}
		unsigned int rowi = i * size;
		int right = blurRad;
		int left = -blurRad;

		int rsum = right * src[rowi].r;
		int gsum = right * src[rowi].g;
		int bsum = right * src[rowi].b;
		int asum = right * src[rowi].a;

		for ( k = 0 ; k <= right ; k++) {
			rsum += src[rowi + k].r;
			gsum += src[rowi + k].g;
			bsum += src[rowi + k].b;
			asum += src[rowi + k].a;
		}

		for ( j = 0 ; j < size ; j++) {
			unsigned int colj = j * size;
			unsigned int oner = rsum/wid;
			unsigned int oneg = gsum/wid;
			unsigned int oneb = bsum/wid;
			unsigned int onea = asum/wid;
			dest[colj + i].r = (oner > 255) ? 255 : oner;
			dest[colj + i].g = (oneg > 255) ? 255 : oneg;
			dest[colj + i].b = (oneb > 255) ? 255 : oneb;
			dest[colj + i].a = (onea > 255) ? 255 : onea;
			// move to next pixel
			unsigned int l = (left < 0)?0:left; // take off the old left
			left++;
			right++;
			unsigned int r = (right < size)?right:(size-1); // but add the new right
			pixel *add = src + rowi + r;
			pixel *sub = src + rowi + l;
			rsum += add->r;
			gsum += add->g;
			bsum += add->b;
			asum += add->a;
			rsum -= sub->r;
			gsum -= sub->g;
			bsum -= sub->b;
			asum -= sub->a;
		}
	}
}
