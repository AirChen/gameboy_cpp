#ifndef Util_1
#define Util_1

#include <vector>

uint8_t wrapping_add(uint8_t a, uint8_t b);
uint8_t wrapping_sub(uint8_t a, uint8_t b);
uint16_t wrapping_sub_16(uint16_t a, uint16_t b);
uint16_t wrapping_add_16(uint16_t a, uint16_t b);

uint8_t trailing_zeros(uint8_t v);

#endif