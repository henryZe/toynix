/* File for math functions */

#include <math.h>

/* calc the log base 2 of a number */
unsigned long log_of_2(unsigned long number)
{
	static unsigned long MultiplyDeBruijnBitPosition2[32] = {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	if (!is_pow_of_2(number))
		number = next_pow_of_2(number) >> 1;

	return MultiplyDeBruijnBitPosition2[((number * 0x077CB531UL) >> 27) & 0x1F];
}

unsigned long pow(unsigned char base, unsigned char index)
{
    unsigned long power = 1;
    for (unsigned char i = 0; i < index; i++) {
        power = base * power;
    }
    return power;
}
