#include "Util.h"

uint8_t wrapping_sub(uint8_t a, uint8_t b)
{
    return (a - b)%256;
}

uint8_t wrapping_add(uint8_t a, uint8_t b)
{
    return (a + b)%256;// 2 的 8次方
}

uint16_t wrapping_sub_16(uint16_t a, uint16_t b)
{
    return (a - b)%65536;
}

uint16_t wrapping_add_16(uint16_t a, uint16_t b)
{
    return (a + b)%65536;// 2 的 8次方
}

uint8_t trailing_zeros(uint8_t v)
{
    bool t = v % 2;
    while (!t)
    {
        uint8_t q = v / 2;
        t = q % 2;
        if (!t) v = q;  
    }
    return v;
}