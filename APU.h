
#ifndef APU_1
#define APU_1

#include <vector>

typedef enum{
    Channel_Square1,
    Channel_Square2,
    Channel_Wave,
    Channel_Noise,
    Channel_Mixer,
} Channel; 

// Name Addr 7654 3210 Function
// -----------------------------------------------------------------
//        Square 1
// NR10 FF10 -PPP NSSS Sweep period, negate, shift
// NR11 FF11 DDLL LLLL Duty, Length load (64-L)
// NR12 FF12 VVVV APPP Starting volume, Envelope add mode, period
// NR13 FF13 FFFF FFFF Frequency LSB
// NR14 FF14 TL-- -FFF Trigger, Length enable, Frequency MSB
//
//        Square 2
//      FF15 ---- ---- Not used
// NR21 FF16 DDLL LLLL Duty, Length load (64-L)
// NR22 FF17 VVVV APPP Starting volume, Envelope add mode, period
// NR23 FF18 FFFF FFFF Frequency LSB
// NR24 FF19 TL-- -FFF Trigger, Length enable, Frequency MSB
//
//        Wave
// NR30 FF1A E--- ---- DAC power
// NR31 FF1B LLLL LLLL Length load (256-L)
// NR32 FF1C -VV- ---- Volume code (00=0%, 01=100%, 10=50%, 11=25%)
// NR33 FF1D FFFF FFFF Frequency LSB
// NR34 FF1E TL-- -FFF Trigger, Length enable, Frequency MSB
//
//        Noise
//      FF1F ---- ---- Not used
// NR41 FF20 --LL LLLL Length load (64-L)
// NR42 FF21 VVVV APPP Starting volume, Envelope add mode, period
// NR43 FF22 SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
// NR44 FF23 TL-- ---- Trigger, Length enable
//
//        Control/Status
// NR50 FF24 ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
// NR51 FF25 NW21 NW21 Left enables, Right enables
// NR52 FF26 P--- NW21 Power control/status, Channel length statuses
//
//        Not used
//      FF27 ---- ----
//      .... ---- ----
//      FF2F ---- ----
//
//        Wave Table
//      FF30 0000 1111 Samples 0 and 1
//      ....
//      FF3F 0000 1111 Samples 30 and 31
class Register_1
{
private:    
    Channel channel;
    uint8_t nrx0;
    uint8_t nrx1;
    uint8_t nrx2;
    uint8_t nrx3;
    uint8_t nrx4;

public:
    Register_1(Channel channel);    

    uint8_t get_sweep_period();

    bool get_negate();

    uint8_t get_shift();

    bool get_dac_power();

    uint8_t get_duty();

    uint16_t get_length_load();

    uint8_t get_starting_volume();    

    uint8_t get_volume_code();    

    bool get_envelope_add_mode();

    uint8_t get_period();    

    uint16_t get_frequency();    

    void set_frequency(uint16_t f);    

    uint8_t get_clock_shift();    

    bool get_width_mode_of_lfsr();    

    uint8_t get_dividor_code();    

    bool get_trigger();    

    void set_trigger(bool b);    

    bool get_length_enable();    

    uint8_t get_l_vol();    

    uint8_t get_r_vol();    

    bool get_power();    
};

// Frame Sequencer
// The frame sequencer generates low frequency clocks for the modulation units. It is clocked by a 512 Hz timer.
//
// Step   Length Ctr  Vol Env     Sweep
// ---------------------------------------
// 0      Clock       -           -
// 1      -           -           -
// 2      Clock       -           Clock
// 3      -           -           -
// 4      Clock       -           -
// 5      -           -           -
// 6      Clock       -           Clock
// 7      -           Clock       -
// ---------------------------------------
// Rate   256 Hz      64 Hz       128 Hz
class FrameSequencer
{
private:
    uint8_t step;
public:
    FrameSequencer();
    uint8_t next();
};

// A length counter disables a channel when it decrements to zero. It contains an internal counter and enabled flag.
// Writing a byte to NRx1 loads the counter with 64-data (256-data for wave channel). The counter can be reloaded at any
// time.
// A channel is said to be disabled when the internal enabled flag is clear. When a channel is disabled, its volume unit
// receives 0, otherwise its volume unit receives the output of the waveform generator. Other units besides the length
// counter can enable/disable the channel as well.
// Each length counter is clocked at 256 Hz by the frame sequencer. When clocked while enabled by NRx4 and the counter
// is not zero, it is decremented. If it becomes zero, the channel is disabled.
class LengthCounter
{
private:
    Register_1 *reg;
    uint16_t n;
public:
    LengthCounter(Register_1 *r);    

    void next();
    void reload();
};


class Clock
{
private:
    uint32_t period;
    uint32_t n;
public:
    Clock(uint32_t p);    
    uint32_t next(uint32_t cycles);
};

// A volume envelope has a volume counter and an internal timer clocked at 64 Hz by the frame sequencer. When the timer
// generates a clock and the envelope period is not zero, a new volume is calculated by adding or subtracting
// (as set by NRx2) one from the current volume. If this new volume within the 0 to 15 range, the volume is updated,
// otherwise it is left unchanged and no further automatic increments/decrements are made to the volume until the
// channel is triggered again.
// When the waveform input is zero the envelope outputs zero, otherwise it outputs the current volume.
// Writing to NRx2 causes obscure effects on the volume that differ on different Game Boy models (see obscure behavior).
class VolumeEnvelope
{
private:
    Register_1 *reg;
    Clock *timer;
    uint8_t volume;
public:
    VolumeEnvelope(Register_1 *r);   
    void next();
    void reload(); 
};

// The first square channel has a frequency sweep unit, controlled by NR10. This has a timer, internal enabled flag,
// and frequency shadow register. It can periodically adjust square 1's frequency up or down.
// During a trigger event, several things occur:
//
//   - Square 1's frequency is copied to the shadow register.
//   - The sweep timer is reloaded.
//   - The internal enabled flag is set if either the sweep period or shift are non-zero, cleared otherwise.
//   - If the sweep shift is non-zero, frequency calculation and the overflow check are performed immediately.
//
// Frequency calculation consists of taking the value in the frequency shadow register, shifting it right by sweep
// shift, optionally negating the value, and summing this with the frequency shadow register to produce a new
// frequency. What is done with this new frequency depends on the context.
//
// The overflow check simply calculates the new frequency and if this is greater than 2047, square 1 is disabled.
// The sweep timer is clocked at 128 Hz by the frame sequencer. When it generates a clock and the sweep's internal
// enabled flag is set and the sweep period is not zero, a new frequency is calculated and the overflow check is
// performed. If the new frequency is 2047 or less and the sweep shift is not zero, this new frequency is written back
// to the shadow frequency and square 1's frequency in NR13 and NR14, then frequency calculation and overflow check are
// run AGAIN immediately using this new value, but this second new frequency is not written back.
// Square 1's frequency can be modified via NR13 and NR14 while sweep is active, but the shadow frequency won't be
// affected so the next time the sweep updates the channel's frequency this modification will be lost.
class FrequencySweep
{
private:
    Register_1 *reg;
    Clock *timer;
    bool enable;
    uint16_t shadow;
    uint16_t newfeq;

public:
    FrequencySweep(Register_1 *r);    
    void next();
    void overflow_check();
    void frequency_calculation();
    void reload();
};


#endif