#ifndef Mmunit_1
#define Mmunit_1

#include <iostream>
#include <vector>
#include <chrono>
#include <string>

#include "Cartridge.h"
#include "CartridgeData.h"

#include "APU.h"
#include "GPU.h"
#include "Util.h"

typedef enum {
    JoypadKey_Right  = 0b00000001,
    JoypadKey_Left   = 0b00000010,
    JoypadKey_Up     = 0b00000100,
    JoypadKey_Down   = 0b00001000,
    JoypadKey_A      = 0b00010000,
    JoypadKey_B      = 0b00100000,
    JoypadKey_Select = 0b01000000,
    JoypadKey_Start  = 0b10000000,
} JoypadKey;

typedef struct Joypad
{
    Intf *intf;
    uint8_t matrix;
    uint8_t select;

    static Joypad power_up(Intf *i)
    {
        Joypad pad;
        pad.intf = i;
        pad.matrix = 0xff;
        pad.select = 0x00;
        return pad;
    }

    void keydown(JoypadKey key) {
        matrix &= !(uint8_t)key;
        intf->hi(Flags_Joypad);
    }

    void keyup(JoypadKey key) {
        matrix |= (uint8_t)key;
    }

    uint8_t get(uint16_t a) {
        // assert_eq!(a, 0xff00);
        if ((select & 0b00010000) == 0x00) {
            return select | (matrix & 0x0f);
        }
        if ((select & 0b00100000) == 0x00) {
            return select | (matrix >> 4);
        }
        return select;
    }

    void set(uint16_t a, uint8_t v) {
        // assert_eq!(a, 0xff00);
        select = v;
    }

} Joypad;

typedef struct Serial {
    Intf *_intf;

    // Before a transfer, it holds the next byte that will go out.
    // During a transfer, it has a blend of the outgoing and incoming bytes. Each cycle, the leftmost bit is shifted
    // out (and over the wire) and the incoming bit is shifted in from the other side:
    uint8_t data;
    // Bit 7 - Transfer Start Flag (0=No transfer is in progress or requested, 1=Transfer in progress, or requested)
    // Bit 1 - Clock Speed (0=Normal, 1=Fast) ** CGB Mode Only **
    // Bit 0 - Shift Clock (0=External Clock, 1=Internal Clock)
    uint8_t control;

    static Serial power_up(Intf *i)
    {
        Serial serial;
        serial._intf = i;
        serial.data = 0x00;
        serial.control = 0x00;
    }

    uint8_t get(uint16_t a) {
        switch (a)  
        {
        case 0xff01:
            return data;
            break;
        case 0xff02:
            return control;
            break;
        
        default:
            std::cout << "Only supports addresses 0xff01, 0xff02" << std::endl;
            break;
        }
        return 0;
    }

    void set(uint16_t a, uint8_t v) {
        switch (a)
        {
        case 0xff01:
            data = v;
            break;
        case 0xff02:
            control = v;
            break;
        
        default:
            std::cout << "Only supports addresses 0xff01, 0xff02" << std::endl;
            break;
        }
    }

} Serial;

typedef struct {
    // This register is incremented at rate of 16384Hz (~16779Hz on SGB). Writing any value to this register resets it
    // to 00h.
    // Note: The divider is affected by CGB double speed mode, and will increment at 32768Hz in double speed.
    uint8_t     div;
    // This timer is incremented by a clock frequency specified by the TAC register ($FF07). When the value overflows
    // (gets bigger than FFh) then it will be reset to the value specified in TMA (FF06), and an interrupt will be
    // requested, as described below.
    uint8_t     tima;
    // When the TIMA overflows, this data will be loaded.
    uint8_t     tma;
    //  Bit  2   - Timer Enable
    //  Bits 1-0 - Input Clock Select
    //             00: CPU Clock / 1024 (DMG, CGB:   4096 Hz, SGB:   ~4194 Hz)
    //             01: CPU Clock / 16   (DMG, CGB: 262144 Hz, SGB: ~268400 Hz)
    //             10: CPU Clock / 64   (DMG, CGB:  65536 Hz, SGB:  ~67110 Hz)
    //             11: CPU Clock / 256  (DMG, CGB:  16384 Hz, SGB:  ~16780 Hz)
    uint8_t     tac;    
} Register_t;

// Each time when the timer overflows (ie. when TIMA gets bigger than FFh), then an interrupt is requested by
// setting Bit 2 in the IF Register (FF0F). When that interrupt is enabled, then the CPU will execute it by calling
// the timer interrupt vector at 0050h.
typedef struct Timer {
    Intf *intf;
    Register_t reg;
    Clock *div_clock;
    Clock *tma_clock;

    static Timer power_up(Intf *ii)
    {
        Timer timer;
        timer.intf = ii;
        timer.reg;
        timer.div_clock = new Clock(256);
        timer.tma_clock = new Clock(1024);
        return timer;
    }

    uint8_t get(uint16_t a)
    {
        switch (a)
        {
        case 0xff04:
            return reg.div;
            break;
        case 0xff05:
            return reg.tima;
            break;
        case 0xff06:
            return reg.tma;
            break;
        case 0xff07:
            return reg.tac;
            break;
        
        default:
            std::cout << "Unsupported address" << std::endl;
            break;
        }
        return 0;
    }

    void set(uint16_t a, uint8_t v) {
        switch (a)
        {
        case 0xff04:
        {
            reg.div = 0x00;   
            div_clock->n = 0x00;
        }
            break;
        case 0xff05: reg.tima = v; break;
        case 0xff06: reg.tma = v; break;
        case 0xff07:
        {
            if ((reg.tac & 0x03) != (v & 0x03))
            {
                tma_clock->n = 0x00;
                reg.tima = reg.tma;

                uint8_t q = v & 0x03;
                if (q == 0x00) tma_clock->period = 1024;
                if (q == 0x01) tma_clock->period = 16;
                if (q == 0x02) tma_clock->period = 64;
                if (q == 0x03) tma_clock->period = 256;                                
            }
            reg.tac = v;
        }
        break;        
        
        default:
            std::cout << "Unsupported address" << std::endl;
            break;
        }
    }

    void next(uint32_t cycles) {
        // Increment div at rate of 16384Hz. Because the clock cycles is 4194304, so div increment every 256 cycles.
        reg.div = wrapping_add(reg.div, (uint8_t)div_clock->next(cycles));

        // Increment tima at rate of Clock / freq
        // Timer Enable
        if ((reg.tac & 0x04) != 0x00) {
            uint32_t n = tma_clock->next(cycles);
            for (size_t i = 0; i < n; i++)
            {
                reg.tima = wrapping_add(reg.tima, 1);
                if (reg.tima == 0x00)
                {
                    reg.tima = reg.tma;
                    intf->hi(Flags_Timer);
                }                
            }
        }
    }

} Timer;

typedef enum {
    Speed_Normal = 0x01,
    Speed_Double = 0x02,
} Speed;

class Mmunit: public Memory {
public:
    Cartridge *cartridge;
    Apu *apu;
    Gpu *gpu;
    Joypad joypad;
    Serial serial;
    bool shift;
    Speed speed;
    Term term;
    Timer timer;

    uint8_t inte;
    Intf *intf;
    Hdma *hdma;
    uint8_t *hram;
    uint8_t *wram;    
    uint8_t wram_bank;


    Mmunit(std::string path);
    ~Mmunit();

    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);

    uint32_t next(uint32_t cycles);
    void switch_speed();
    uint32_t run_dma();
    void run_dma_hrampart();
};

#endif