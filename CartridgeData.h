#ifndef CartridgeData_1
#define CartridgeData_1

#include <vector>
#include <iostream>

#include "Cartridge.h"

class RomOnly: public Cartridge
{
private:
    /* data */
public:    
    RomOnly(std::vector <uint8_t> v);    
};

typedef enum {
    BankMode_Rom,
    BankMode_Ram,
} BankMode;

class Mbc1: public Cartridge
{
private:
    std::vector <uint8_t> ram;
    BankMode bank_mode;
    uint8_t bank;
    bool ram_enable;
    std::string save_path;

protected:
    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
    void save(std::string path);

public:    
    Mbc1(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path);
    
    size_t ram_bank();
    size_t rom_bank();
};

class Mbc2: public Cartridge
{
private:
    std::vector <uint8_t> ram;    
    uint8_t rom_bank;
    bool ram_enable;
    std::string save_path;

protected:
    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
    void save(std::string path);

public:    
    Mbc2(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path);
};

class RealTimeClock: public Memory, public Stable
{
private:
    uint8_t s;
    uint8_t m;
    uint8_t h;
    uint8_t dl;
    uint8_t dh;
    uint64_t zero;
    std::string save_path;  

public:
    RealTimeClock(std::string path);
    void tic();

    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
    void save(std::string path);  
};

class Mbc3: public Cartridge
{
private:
    std::vector <uint8_t> ram;    
    RealTimeClock *rtc;
    uint8_t rom_bank;
    uint8_t ram_bank;
    bool ram_enable;
    std::string save_path;

protected:
    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
    void save(std::string path);

public:    
    Mbc3(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path, std::string rtc_path);
    ~Mbc3();
};

class Mbc5: public Cartridge
{
private:
    std::vector <uint8_t> ram;        
    uint8_t rom_bank;
    uint8_t ram_bank;
    bool ram_enable;
    std::string save_path;

protected:
    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
    void save(std::string path);

public:    
    Mbc5(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path);
    ~Mbc5();
};

#endif