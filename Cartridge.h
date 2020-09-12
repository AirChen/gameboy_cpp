
#ifndef Cartridge_1
#define Cartridge_1

#include <vector>

class Memory
{
protected:
    std::vector<uint8_t> rom;

public:    
    virtual uint8_t get(unsigned int a)
    {
        return rom[a];
    }
    virtual void set(unsigned int a, uint8_t v){};

    virtual uint16_t get_word(unsigned int a)
    {
        return (uint16_t)a | (uint16_t)get(a+1) << 8;
    };

    virtual void set_word(unsigned int a, uint16_t v)
    {
        set(a, (uint8_t)(v & 0xff));
        set(a + 1, (uint8_t)(v >> 8));
    };         
};

class Stable
{
private:
    /* data */
public:    
    virtual void save(std::string path) = 0;// save data to local
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