#ifndef GAUSSIANBLUR_H_
#define GAUSSIANBLUR_H_

typedef struct {
		unsigned char r, g, b, a;
} pixel;

void NEONboxBlur(pixel *src, pixel *dest, unsigned int size, unsigned int blurRad);
void boxBlur(pixel *src, pixel *dest, unsigned int size, unsigned int blurRad);

#endif /* GAUSSIANBLUR_H_ */
