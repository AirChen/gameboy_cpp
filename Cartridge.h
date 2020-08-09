
#ifndef Cartridge_1
#define Cartridge_1

#include <vector>

uint8_t wrapping_add(uint8_t a, uint8_t b);
uint8_t wrapping_sub(uint8_t a, uint8_t b);
uint16_t wrapping_sub_16(uint16_t a, uint16_t b);
uint16_t wrapping_add_16(uint16_t a, uint16_t b);

uint8_t trailing_zeros(uint8_t v);

class Memory
{
protected:
    std::vector<uint8_t> rom;

public:    
    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v){};

    uint16_t get_word(unsigned int a);

    uint16_t set_word(unsigned int a, uint16_t v);    
};

class Stable
{
private:
    /* data */
public:    
    void save(std::string path);// save data to local
};

class Cartridge: public Memory, public Stable
{
public:
    std::string title();    
};

bool ensure_header_checksum(Cartridge *cart);
bool ensure_logo(Cartridge *cart);

// Specifies the size of the external RAM in the cartridge (if any).
unsigned int ram_size(uint8_t b);
// Specifies the ROM Size of the cartridge. Typically calculated as "32KB shl N".
unsigned int rom_size(uint8_t b);

Cartridge *createCart(std::string path);

#endif