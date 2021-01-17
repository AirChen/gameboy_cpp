#include "machine.h"
#include "include/MiniFB_cpp.h"
#include "MotherBoard.h"
#include <iostream>

#define WIDTH      160
#define HEIGHT     144

using namespace std;
static MotherBoard* m_mbrd = new MotherBoard("mem_timing.gb");

Machine::~Machine() {
    delete m_mbrd;
    m_mbrd = nullptr;
}

static void gb_keyboard_func(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool isPressed)
{
    if (!m_mbrd->cpu->flip())
    {
        return;
    }
    
    switch (key)
    {
        case KB_KEY_RIGHT:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_Right) : m_mbrd->mmu->joypad.keyup(JoypadKey_Right);
        }
        break;
        case KB_KEY_UP:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_Up) : m_mbrd->mmu->joypad.keyup(JoypadKey_Up);
        }
        break;
        case KB_KEY_LEFT:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_Left) : m_mbrd->mmu->joypad.keyup(JoypadKey_Left);
        }
        break;
        case KB_KEY_DOWN:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_Down) : m_mbrd->mmu->joypad.keyup(JoypadKey_Down);
        }
        break;
        case KB_KEY_Z:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_A) : m_mbrd->mmu->joypad.keyup(JoypadKey_A);
        }
        break;
        case KB_KEY_X:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_B) : m_mbrd->mmu->joypad.keyup(JoypadKey_B);
        }
        break;
        case KB_KEY_SPACE:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_Select) : m_mbrd->mmu->joypad.keyup(JoypadKey_Select);
        }
        break;
        case KB_KEY_ENTER:
        {
            isPressed ? m_mbrd->mmu->joypad.keydown(JoypadKey_Start) : m_mbrd->mmu->joypad.keyup(JoypadKey_Start);
        }
        break;
    
        default:
        break;
    }
}

void Machine::run() {
    int i, noise, carry, seed = 0xbeef;

    uint32_t g_buffer[WIDTH * HEIGHT];
    string rom_name = m_mbrd->mmu->cartridge->title();
    string title("Gameboy - { "+ rom_name +" }");
    struct mfb_window *window = mfb_open_ex(title.c_str(), WIDTH, HEIGHT, WF_RESIZABLE);
    if (!window)
    {
        cout << "with out windows" << endl;
        return;
    }        

    mfb_set_keyboard_callback(window, gb_keyboard_func);
    mfb_update_state state;
    do {        
        m_mbrd->next();
        if (m_mbrd->check_and_reset_gpu_updated())
        {
            uint16_t i = 0;
            for (size_t y = 0; y < 144; y++)
            {
                for (size_t w = 0; w < 160; i++)
                {
                    uint32_t b = m_mbrd->mmu->gpu->data[y][w][2];
                    uint32_t g = m_mbrd->mmu->gpu->data[y][w][1];
                    uint32_t r = m_mbrd->mmu->gpu->data[y][w][0];
                    uint32_t a = 0xff000000;

                    g_buffer[i++] = a|b|g|r;                    
                }                
            }            
        }
        
        mfb_update(window, g_buffer);
        // if (!mfb_is_window_active(window)) {
        //     window = 0x0;
        //     cout << "windows not active" << endl;
        //     break;
        // }
    } while(mfb_wait_sync(window));
}