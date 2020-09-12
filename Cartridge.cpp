#include "Cartridge.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>
#include "CartridgeData.h"
#include "Util.h"

using namespace std;   

string Cartridge::title()
{
    string str;
    size_t h = (get(0x0143) == 0x80) ? 0x013e : 0x0143;    
    for (size_t i = 0x0134; i < h; i++)
    {        
        str.push_back(get(i));
    }
    return str;
}

static uint8_t NINTENDO_LOGO[] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
    0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
    0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
}; // 48

bool ensure_logo(Cartridge *cart)
{
    for (size_t i = 0; i < 48; i++)
    {
        if (cart->get(0x0104 + i) != NINTENDO_LOGO[i])
        {
            cout << "Nintendo logo is incorrect" << endl;
            return false;
        }
    }
    return true;
}

bool ensure_header_checksum(Cartridge *cart)
{
    unsigned char v = 0;
    for (size_t i = 0x0134; i < 0x014d; i++)
    {
        unsigned char temp = wrapping_sub(v, cart->get(i));
        v = wrapping_sub(temp, 1);
    }

    if (v != cart->get(0x014d))
    {
        cout << "Cartridge's header checksum is incorrect" << endl;
        return false;
    }
        
    return true;
} 

unsigned int rom_size(uint8_t b)
{
    unsigned int bank = 16384;
    switch (b)
    {
    case 0x00:
        return bank * 2;
        break;
    case 0x01:
        return bank * 4;
        break;
    case 0x02:
        return bank * 8;
        break;

    case 0x03:
        return bank * 16;
        break;
    case 0x04:
        return bank * 32;
        break;
    case 0x05:
        return bank * 64;
        break;
    
    case 0x06:
        return bank * 128;
        break;
    case 0x07:
        return bank * 256;
        break;
    case 0x08:
        return bank * 512;
        break;

    case 0x52:
        return bank * 72;
        break;
    case 0x53:
        return bank * 80;
        break;
    case 0x54:
        return bank * 96;
        break;

    default:    
        cout << "Unsupported rom size: " << b << endl;
        break;
    }
    return 0;
}

unsigned int ram_size(uint8_t b)
{    
    switch (b)
    {
    case 0x00:
        return 0;
        break;
    case 0x01:
        return 1024 * 2;
        break;
       
    case 0x02:
        return 1024 * 8;
        break;
    case 0x03:
        return 1024 * 32;
        break;

    case 0x04:
        return 1024 * 128;
        break;
    case 0x05:
        return 1024 * 64;
        break;

    default:
        cout << "Unsupported ram size: " << b << endl;
        break;
    }
    return 0;
}

vector<uint8_t> ram_read(string path, unsigned int size)
{
    ifstream inFile(path, ios::binary);
    if (!inFile)
    {
        uint8_t v[size];
        vector<uint8_t> ram(v, v+size);
        return ram;
    }
	
	unsigned char temp;    
    vector<uint8_t> ram;
	while (inFile.read((char*)&temp, 1))
	{		
        ram.push_back(temp);
	}	
	inFile.close();

    return ram;
}

Cartridge *createCart(std::string path)
{
    ifstream inFile(path, ios::binary);
    if (!inFile)
    {
        exit(1);
    }

    if (false) {
        // 获取文件大小、文件名
        long long Beg = inFile.tellg();
        inFile.seekg(0, ios::end);
        long long End = inFile.tellg();
        long long fileSize = End - Beg;
        inFile.seekg(0, ios::beg);
        cout << "File Size: " << fileSize / 1024.0 << "KB" << endl;
    }
	
	unsigned char temp;    
    vector<uint8_t> rom;
	while (inFile.read((char*)&temp, 1))
	{		
        rom.push_back(temp);
	}
	// 关闭文件流
	inFile.close();

    if (rom.size() < 0x0150)
    {
        cout << "Missing required information area which located at 0100-014F" << endl;
    }
    
    unsigned int rom_max = rom_size(rom[0x0148]);
    if (rom.size() > rom_max)
    {
        cout << "Rom size more than " << rom_max << endl;
    }

    Cartridge *cart = NULL;
    unsigned int category = rom[0x0147];
    if (category == 0x00)
    {
        cart = new RomOnly(rom);
    }

    if (category == 0x01)
    {
        vector<uint8_t> ram;
        cart = new Mbc1(rom, ram, "");
    }

    if (category == 0x02)
    {
        unsigned int i = ram_size(rom[0x0149]);
        uint8_t v[i];
        vector<uint8_t> ram(v, v+i);
        cart = new Mbc1(rom, ram, "");
    }
    
    if (category == 0x03)
    {
        unsigned int i = ram_size(rom[0x0149]);
        string path("sav");        
        vector<uint8_t> ram = ram_read(path, i);
        cart = new Mbc1(rom, ram, path);
    }

    if (category == 0x05)
    {        
        uint8_t v[512];
        vector<uint8_t> ram(v, v+512);
        cart = new Mbc2(rom, ram, "");
    }

    if (category == 0x06)
    {
        unsigned int i = 512;
        string path("sav");        
        vector<uint8_t> ram = ram_read(path, i);
        cart = new Mbc2(rom, ram, path);
    }

    if (category == 0x0f)
    {
        vector<uint8_t> ram;
        cart = new Mbc3(rom, ram, "sav", "rtc");
    }

    if (category == 0x10)
    {
        unsigned int i = ram_size(rom[0x0149]);
        string path("sav");        
        vector<uint8_t> ram = ram_read(path, i);
        cart = new Mbc3(rom, ram, path, "rtc");
    }
    
    if (category == 0x11)
    {
        unsigned int i = ram_size(rom[0x0149]);        
        vector<uint8_t> ram;
        cart = new Mbc3(rom, ram, "", "");
    }

    if (category == 0x12)
    {
        unsigned int i = ram_size(rom[0x0149]);
        uint8_t v[i];
        vector<uint8_t> ram(v, v+i);
        cart = new Mbc3(rom, ram, "", "");
    }

    if (category == 0x13)
    {
        unsigned int i = ram_size(rom[0x0149]);
        string path("sav");        
        vector<uint8_t> ram = ram_read(path, i);
        cart = new Mbc3(rom, ram, path, "");
    }
    
    if (category == 0x19)
    {            
        vector<uint8_t> ram;
        cart = new Mbc5(rom, ram, "");
    }

    if (category == 0x1a)
    {
        unsigned int i = ram_size(rom[0x0149]);
        uint8_t v[i];        
        vector<uint8_t> ram(v, v+i);
        cart = new Mbc5(rom, ram, "");
    }

    if (category == 0x1b)
    {
        unsigned int i = ram_size(rom[0x0149]);
        string path("sav");        
        vector<uint8_t> ram = ram_read(path, i);
        cart = new Mbc5(rom, ram, path);
    }

        // 0xff => {
        //     let ram_max = ram_size(rom[0x0149]);
        //     let sav_path = path.as_ref().to_path_buf().with_extension("sav");
        //     let ram = ram_read(sav_path.clone(), ram_max);
        //     Box::new(HuC1::power_up(rom, ram, sav_path))
        // }
    
    
    if (ensure_logo(cart))
    {
        cout << "Nintendo logo is ok" << endl;
    }
    
    if (ensure_header_checksum(cart))
    {
        cout << "Cartridge's header checksum is ok" << endl;
    }    

    cout << "title: " << cart->title() << endl;

    return cart;
}