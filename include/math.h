#ifndef INC_MATH_H
#define INC_MATH_H

#define is_pow_of_2(x) (!((x) & ((x) - 1)))

static inline unsigned long next_pow_of_2(unsigned long x)
{
	if (is_pow_of_2(x))
		return x;

	x |= x>>1;
	x |= x>>2;
	x |= x>>4;
	x |= x>>8;
	x |= x>>16;

	return x + 1;
}

unsigned long log_of_2(unsigned long number);
unsigned long pow(unsigned char base, unsigned char index);

#endif
