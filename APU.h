
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

class Register
{
private:    
    Channel channel;
    uint8_t nrx0;
    uint8_t nrx1;
    uint8_t nrx2;
    uint8_t nrx3;
    uint8_t nrx4;

public:
    Register(Channel channel);    

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

#endif