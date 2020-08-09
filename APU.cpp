#include "APU.h"
#include <assert.h>
#include "CartridgeData.h"

Register_1::Register_1(Channel ch)
{
    channel = ch;
    nrx0 = 0x00;
    nrx1 = (channel == Channel_Square1 || channel == Channel_Square2) ? 0x40 : 0x00;
    nrx2 = 0x00;
    nrx3 = 0x00;
    nrx4 = 0x00;
}

uint8_t Register_1::get_sweep_period()
{
    assert(channel != Channel_Square1);     
    return (nrx0 >> 4) & 0x07;
}

bool Register_1::get_negate()
{
    assert(channel != Channel_Square1);
    return (nrx0 & 0x08) != 0x00;
}
    
uint8_t Register_1::get_shift()
{
    assert(channel != Channel_Square1);
    return nrx0 & 0x07;
}    

bool Register_1::get_dac_power()
{
    assert(channel != Channel_Wave);
    return (nrx0 & 0x80) != 0x00;
}

uint8_t Register_1::get_duty()
{
    assert(!((channel == Channel_Square1) || (channel == Channel_Square2)));
    return nrx1 >> 6;
}

uint16_t Register_1::get_length_load()
{
    if (channel == Channel_Wave)
    {
        return (1 << 8) - nrx1;
    }
    return (1 << 6) - (nrx1 & 0x3f);
}

uint8_t Register_1::get_starting_volume()
{
    assert(channel == Channel_Wave);
    return nrx2 >> 4;
}

uint8_t Register_1::get_volume_code()
{
    assert(channel != Channel_Wave);
    return (nrx2 >> 5) & 0x03;
}

bool Register_1::get_envelope_add_mode()
{
    assert(channel == Channel_Wave);
    return (nrx2 & 0x08) != 0x00;
}

uint8_t Register_1::get_period()
{
    assert(channel == Channel_Wave);
    return (nrx2 & 0x07);
}

uint16_t Register_1::get_frequency()
{
    assert(channel == Channel_Noise);
    return ((nrx4 & 0x07) << 8) | nrx3;
}

void Register_1::set_frequency(uint16_t f)
{
    assert(channel == Channel_Noise);
    uint8_t h = (f >> 8) & 0x07;
    uint8_t l = f;
    nrx4 = (nrx4 & 0xf8) | h;
    nrx3 = l;
}

uint8_t Register_1::get_clock_shift()
{
    assert(channel != Channel_Noise);
    return (nrx3 >> 4);
}

bool Register_1::get_width_mode_of_lfsr()
{
    assert(channel != Channel_Noise);
    return (nrx3 & 0x08) != 0x00;
}

uint8_t Register_1::get_dividor_code()
{
    assert(channel != Channel_Noise);
    return (nrx3 & 0x07);
}    

bool Register_1::get_trigger()
{
    return (nrx4 & 0x80) != 0x00;
}

void Register_1::set_trigger(bool b)
{
    if (b)
    {
        nrx4 |= 0x80;
    } else
    {
        nrx4 &= 0x7f;
    }
}

bool Register_1::get_length_enable()
{
    return (nrx4 & 0x40) != 0x00;
}    

uint8_t Register_1::get_l_vol()
{
    assert(channel != Channel_Mixer);
    return (nrx0 >> 4) & 0x07;
}

uint8_t Register_1::get_r_vol()
{
    assert(channel != Channel_Mixer);
    return (nrx0 & 0x07);
}

bool Register_1::get_power()
{
    assert(channel != Channel_Mixer);
    return (nrx2 & 0x80) != 0x00;
}

FrameSequencer::FrameSequencer()
{
    step = 0x00;
}

uint8_t FrameSequencer::next()
{
    step += 1;
    step %= 8;
    return step;
}

LengthCounter::LengthCounter(Register_1 *r): reg(r)
{
    n = 0x0000;
}

void LengthCounter::next()
{
    if (reg->get_length_enable() && n != 0) {
        n -= 1;
        if (n == 0) {
            reg->set_trigger(false);
        }
    }
}

void LengthCounter::reload()
{
    if (n == 0x0000) {
        n = (reg->channel == Channel_Wave) ? (1 << 8) : (1 << 6);
    }
}

Clock::Clock(uint32_t p)
{
    period = p;
    n = 0x00;
}

uint32_t Clock::next(uint32_t cycles)
{
    n += cycles;
    uint32_t rs = n /period;
    n = n % period;
    return rs;
}

VolumeEnvelope::VolumeEnvelope(Register_1 *r): reg(r)
{
    timer = new Clock(8);
    volume = 0x00;
}

void VolumeEnvelope::reload() {
    uint8_t p = reg->get_period();
    // The volume envelope and sweep timers treat a period of 0 as 8.
    timer->period = (p == 0) ? 8 : p;
    volume = reg->get_starting_volume();
}

void VolumeEnvelope::next() {
    if (reg->get_period() == 0) {
        return;
    }
    if (timer->next(1) == 0x00) {
        return;
    };
    // If this new volume within the 0 to 15 range, the volume is updated
    uint8_t v =  (reg->get_envelope_add_mode()) ? wrapping_add(volume, 1) : wrapping_sub(volume, 1);    
    if (v <= 15) {
        volume = v;
    }
}

FrequencySweep::FrequencySweep(Register_1 *r): reg(r)
{
    timer = new Clock(8);
    enable = false;
    shadow = 0x00;
    newfeq = 0x00;
}

void FrequencySweep::reload() {
    shadow = reg->get_frequency();
    uint8_t p = reg->get_sweep_period();
    // The volume envelope and sweep timers treat a period of 0 as 8.
    timer->period = (p == 0) ?  8 : (uint32_t)p;
    enable = (p != 0x00 || reg->get_shift() != 0x00);
    if (reg->get_shift() != 0x00) {
        frequency_calculation();
        overflow_check();
    }
}

void FrequencySweep::frequency_calculation() {
    uint16_t offset = (shadow >> reg->get_shift());
    if (reg->get_negate()) {
        newfeq = wrapping_sub_16(shadow, offset);
    } else {
        newfeq = wrapping_add_16(shadow, offset);
    }
}

void FrequencySweep::overflow_check() {
    if (newfeq >= 2048) {
        reg->set_trigger(false);
    }
}

void FrequencySweep::next() {
    if (!enable || reg->get_sweep_period() == 0) {
        return;
    }
    if (timer->next(1) == 0x00) {
        return;
    }
    frequency_calculation();
    overflow_check();

    if (newfeq < 2048 && reg->get_shift() != 0) {
        reg->set_frequency(newfeq);
        shadow = newfeq;
        frequency_calculation();
        overflow_check();
    }
}