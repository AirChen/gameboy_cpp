#include "CartridgeData.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <ctime>

using namespace std;

static inline time_t current_time()
{
    time_t timep;
    struct tm *p;

    time(&timep);  /*得到time_t类型的UTC时间*/    
    p = gmtime(&timep); /*得到tm结构的UTC时间*/
    timep = mktime(p); 

    return timep;
}

RomOnly::RomOnly(std::vector <uint8_t> v)
{
    rom = v;
}

Mbc1::Mbc1(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path): ram(a), save_path(path) 
{ 
    bank_mode = BankMode_Rom;
    bank = 0x01;
    ram_enable = false;
    rom = o;
}

size_t Mbc1::ram_bank()
{
    switch (bank_mode)
    {
    case BankMode_Rom:
        return 0x00;
        break;
    case BankMode_Ram:
        return (bank & 0x60) >> 5 ;
        break;
    default:
        break;
    }
    return 0x00;
}

size_t Mbc1::rom_bank()
{
    switch (bank_mode)
    {
    case BankMode_Rom:
        return (bank & 0x7f);
        break;
    case BankMode_Ram:
        return (bank & 0x1f) ;
        break;
    default:
        break;
    }
    return 0x00;
}

uint8_t Mbc1::get(unsigned int a)
{
    if (a >= 0 && a<= 0x3fff)
    {
        return rom[a];
    }

    if (a >= 0x4000 && a <= 0x7fff)
    {
        unsigned int i = rom_bank() * 0x4000 + a - 0x4000;
        return rom[i];
    }
    
    if (a >= 0xa000 && a <= 0xbfff)
    {
        if (ram_enable)
        {
            unsigned int i = ram_bank() * 0x2000 + a - 0xa000;
            return ram[i];
        } else
        {
            return 0x00;
        }
    }
    
    return 0x00;
}

void Mbc1::set(unsigned int a, uint8_t v)
{
    if (a >= 0xa000 && a <= 0xbfff)
    {
        if (ram_enable)
        {
            unsigned int i = ram_bank() * 0x2000 + a - 0xa000;
        }        
    }
    
    if (a >= 0x00 && a <= 0x1fff)
    {
        ram_enable = ((v & 0x0f) == 0x0a);
        if (!ram_enable)
        {
            save(save_path);
        }        
    }
    
    if (a >= 0x2000 && a <= 0x3fff)
    {
        uint8_t n = (v & 0x1f);
        if (n == 0x00)
        {
            n = 0x01;
        }
        bank = (bank & 0x60) | n;
    }
    
    if (a >= 0x4000 && a <= 0x5fff)
    {
        uint8_t n = v & 0x03;
        bank = (bank & 0x9f) | (n << 5);
    }

    if (a >= 0x6000 && a <= 0x7fff)
    {
        if (v == 0x00)
        {
            bank_mode = BankMode_Rom;
        } else if (v == 0x01) 
        {
            bank_mode = BankMode_Ram;
        } else
        {
            cout << "Invalid cartridge type " << v << endl;
        }   
    }
}

void Mbc1::save(std::string path)
{
    if (path.empty())
    {
        return;
    }

    ofstream outFile(path, ios::binary);
    if (!outFile || !outFile.is_open())
    {
        exit(1);
    }
    
    uint8_t *pc = &ram[0];
    outFile.write((char *)pc, ram.size());
    outFile.close();
}

Mbc2::Mbc2(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path): ram(a), save_path(path) 
{ 
    rom_bank = 0x01;
    ram_enable = false;
    rom = o;
}

uint8_t Mbc2::get(unsigned int a)
{
    if (a >= 0 && a<= 0x3fff)
    {
        return rom[a];
    }

    if (a >= 0x4000 && a <= 0x7fff)
    {
        unsigned int i = rom_bank * 0x4000 + a - 0x4000;
        return rom[i];
    }
    
    if (a >= 0xa000 && a <= 0xa1ff)
    {
        if (ram_enable)
        {
            unsigned int i = a - 0xa000;
            return ram[i];
        } else
        {
            return 0x00;
        }
    }
    
    return 0x00;
}

void Mbc2::set(unsigned int a, uint8_t v)
{
    if (a >= 0xa000 && a <= 0xa1ff)
    {
        if (ram_enable)
        {
            ram[a-0xa000] = v;            
        }        
    }
    
    if (a >= 0x00 && a <= 0x1fff)
    {
        if ((a&0x0100) == 0)
        {
            ram_enable = (v == 0x0a);
        }
    }
    
    if (a >= 0x2000 && a <= 0x3fff)
    {
        if ((a&0x0100) != 0)
        {
            rom_bank = v;
        }
    }
}

void Mbc2::save(std::string path)
{
    if (path.empty())
    {
        return;
    }

    ofstream outFile(path, ios::binary);
    if (!outFile || !outFile.is_open())
    {
        exit(1);
    }
    
    uint8_t *pc = &ram[0];
    outFile.write((char *)pc, ram.size());
    outFile.close();
}

RealTimeClock::RealTimeClock(std::string path): save_path(path)
{
    ifstream inFile(path);
    if (!inFile || !inFile.is_open())
    {
        zero = current_time();
    } else
    {
        uint64_t t;
        inFile.read((char *)&t, sizeof(t));
        zero = t;
        inFile.close();
    }
    
    s = 0;
    m = 0;
    h = 0;
    dl = 0;
    dh = 0;
}

void RealTimeClock::tic()
{
    time_t d = current_time() - zero;
    s = d % 60;
    m = d / 60 % 60;
    h = d / 3600 % 24;

    uint16_t days = d / 3600 / 24;
    dl = days % 256;

    if (days >= 0x0000 && days <= 0x00ff)
    {}
    else if (days >= 0x0100 && days <= 0x01ff)
    {
        dh |= 0x01;
    } else {
        dh |= 0x01;
        dh |= 0x80;
    }
}

uint8_t RealTimeClock::get(unsigned int a)
{
    if (a == 0x08)
    {
        return s;
    }

    if (a == 0x09)
    {
        return m;
    }
    
    if (a == 0x0a)
    {
        return h;
    }
    
    if (a == 0x0b)
    {
        return dl;
    }
    
    if (a == 0x0c)
    {
        return dh;
    }
    
    cout << "No entry" << endl;
    return -1;   
}

void RealTimeClock::set(unsigned int a, uint8_t v)
{
    if (a == 0x08)
    {
        s = v;
    }

    else if (a == 0x09)
    {
        m = v;
    }

    else if (a == 0x0a)
    {
        h = v;
    }

    else if (a == 0x0b)
    {
        dl = v;
    }

    else if (a == 0x0c)
    {
        dh = v;
    }
    
    else
    {
        cout << "No entry" << endl;
    }    
}

void RealTimeClock::save(std::string path)
{
    if (path.empty())
    {
        return;
    }

    ofstream outFile(path, ios::binary);
    if (!outFile || !outFile.is_open())
    {
        exit(1);
    }
    
    outFile.write(reinterpret_cast<char*>(&zero), sizeof(zero));
    outFile.close();
}

Mbc3::Mbc3(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path, std::string rtc_path): ram(a), save_path(path)
{    
    rom = o;
    rtc = new RealTimeClock(rtc_path);
    rom_bank = 1;
    ram_bank = 0;
    ram_enable = false;    
}

Mbc3::~Mbc3()
{
    delete rtc;
}

uint8_t Mbc3::get(unsigned int a)
{
    if (a >= 0 && a<= 0x3fff)
    {
        return rom[a];
    }

    if (a >= 0x4000 && a <= 0x7fff)
    {
        unsigned int i = rom_bank * 0x4000 + a - 0x4000;
        return rom[i];
    }
    
    if (a >= 0xa000 && a <= 0xbfff)
    {
        if (ram_enable)
        {
            if (ram_bank <= 0x03)
            {
                unsigned int i = ram_bank * 0x2000 + a - 0xa000;
                return ram[i];
            }
            
            return rtc->get(ram_bank);            
        } else
        {
            return 0x00;
        }
    }
    
    return 0x00;
}

void Mbc3::set(unsigned int a, uint8_t v)
{
    if (a >= 0xa000 && a <= 0xbfff)
    {
        if (ram_enable)
        {
            if (ram_bank <= 0x03)
            {
                unsigned int i = ram_bank * 0x2000 + a - 0xa000;
                ram[i] = v;
            } else
            {
                rtc->set(ram_bank, v);
            }                     
        }        
    }
    
    if (a >= 0x00 && a <= 0x1fff)
    {
        ram_enable = ((v & 0x0f) == 0x0a);
    }
    
    if (a >= 0x2000 && a <= 0x3fff)
    {
        unsigned int n = (v & 0x7f);
        if (n == 0x00)
        {
            n = 0x01;
        }
        rom_bank = n;        
    }

    if (a >= 0x4000 && a <= 0x5fff)
    {
        unsigned int n = (v & 0x0f);
        ram_bank = n;
    }

    if (a >= 0x6000 && a <= 0x7fff)
    {
        if ((v & 0x01) != 0)
        {
            rtc->tic();
        }        
    }
}

void Mbc3::save(std::string path)
{
    if (path.empty())
    {
        return;
    }

    ofstream outFile(path, ios::binary);
    if (!outFile || !outFile.is_open())
    {
        exit(1);
    }
    
    uint8_t *pc = &ram[0];
    outFile.write((char *)pc, ram.size());
    outFile.close();
}

Mbc5::Mbc5(std::vector<uint8_t> o, std::vector<uint8_t> a, std::string path): ram(a), save_path(path)
{
    rom = o;
    rom_bank = 1;
    ram_bank = 0;
    ram_enable = false;    
}

uint8_t Mbc5::get(unsigned int a)
{
    if (a >= 0 && a<= 0x3fff)
    {
        return rom[a];
    }

    if (a >= 0x4000 && a <= 0x7fff)
    {
        unsigned int i = rom_bank * 0x4000 + a - 0x4000;
        return rom[i];
    }
    
    if (a >= 0xa000 && a <= 0xbfff)
    {
        if (ram_enable)
        {
            unsigned int i = ram_bank * 0x2000 + a - 0xa000;            
            return ram[i];            
        } else
        {
            return 0x00;
        }
    }
    
    return 0x00;
}

void Mbc5::set(unsigned int a, uint8_t v)
{
    if (a >= 0xa000 && a <= 0xbfff)
    {
        if (ram_enable)
        {
            unsigned int i = ram_bank * 0x2000 + a - 0xa000;
            ram[i] = v;                   
        }        
    }
    
    if (a >= 0x00 && a <= 0x1fff)
    {
        ram_enable = ((v & 0x0f) == 0x0a);
    }
    
    if (a >= 0x2000 && a <= 0x2fff)
    {
        rom_bank  = (rom_bank & 0x100) | v;        
    }

    if (a >= 0x3000 && a <= 0x3fff)
    {
        rom_bank = (rom_bank & 0x0ff) | ((v & 0x01) << 8);
    }

    if (a >= 0x4000 && a <= 0x5fff)
    {
        ram_bank = (v & 0x0f);        
    }
}

void Mbc5::save(std::string path)
{
    if (path.empty())
    {
        return;
    }

    ofstream outFile(path, ios::binary);
    if (!outFile || !outFile.is_open())
    {
        exit(1);
    }
    
    uint8_t *pc = &ram[0];
    outFile.write((char *)pc, ram.size());
    outFile.close();
}