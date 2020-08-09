#include "APU.h"
#include <assert.h>

Register::Register(Channel ch)
{
    channel = ch;
    nrx0 = 0x00;
    nrx1 = (channel == Channel_Square1 || channel == Channel_Square2) ? 0x40 : 0x00;
    nrx2 = 0x00;
    nrx3 = 0x00;
    nrx4 = 0x00;
}

uint8_t Register::get_sweep_period()
{
    assert(channel != Channel_Square1);     
    return (nrx0 >> 4) & 0x07;
}

bool Register::get_negate()
{
    assert(channel != Channel_Square1);
    return (nrx0 & 0x08) != 0x00;
}
    
uint8_t Register::get_shift()
{
    assert(channel != Channel_Square1);
    return nrx0 & 0x07;
}    

bool Register::get_dac_power()
{
    assert(channel != Channel_Wave);
    return (nrx0 & 0x80) != 0x00;
}

uint8_t Register::get_duty()
{
    assert(!((channel == Channel_Square1) || (channel == Channel_Square2)));
    return nrx1 >> 6;
}

uint16_t Register::get_length_load()
{
    if (channel == Channel_Wave)
    {
        return (1 << 8) - nrx1;
    }
    return (1 << 6) - (nrx1 & 0x3f);
}

uint8_t Register::get_starting_volume()
{
    assert(channel == Channel_Wave);
    return nrx2 >> 4;
}

uint8_t Register::get_volume_code()
{
    assert(channel != Channel_Wave);
    return (nrx2 >> 5) & 0x03;
}

bool Register::get_envelope_add_mode()
{
    assert(channel == Channel_Wave);
    return (nrx2 & 0x08) != 0x00;
}

uint8_t Register::get_period()
{
    assert(channel == Channel_Wave);
    return (nrx2 & 0x07);
}

uint16_t Register::get_frequency()
{
    assert(channel == Channel_Noise);
    return ((nrx4 & 0x07) << 8) | nrx3;
}

void Register::set_frequency(uint16_t f)
{
    assert(channel == Channel_Noise);
    uint8_t h = (f >> 8) & 0x07;
    uint8_t l = f;
    nrx4 = (nrx4 & 0xf8) | h;
    nrx3 = l;
}

uint8_t Register::get_clock_shift()
{
    assert(channel != Channel_Noise);
    return (nrx3 >> 4);
}

bool Register::get_width_mode_of_lfsr()
{
    assert(channel != Channel_Noise);
    return (nrx3 & 0x08) != 0x00;
}

uint8_t Register::get_dividor_code()
{
    assert(channel != Channel_Noise);
    return (nrx3 & 0x07);
}    

bool Register::get_trigger()
{
    return (nrx4 & 0x80) != 0x00;
}

void Register::set_trigger(bool b)
{
    if (b)
    {
        nrx4 |= 0x80;
    } else
    {
        nrx4 &= 0x7f;
    }
}

bool Register::get_length_enable()
{
    return (nrx4 & 0x40) != 0x00;
}    

uint8_t Register::get_l_vol()
{
    assert(channel != Channel_Mixer);
    return (nrx0 >> 4) & 0x07;
}

uint8_t Register::get_r_vol()
{
    assert(channel != Channel_Mixer);
    return (nrx0 & 0x07);
}

bool Register::get_power()
{
    assert(channel != Channel_Mixer);
    return (nrx2 & 0x80) != 0x00;
}