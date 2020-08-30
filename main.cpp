#include <iostream>
#include "include/MiniFB_cpp.h"
#include "MotherBoard.h"

#define WIDTH      800
#define HEIGHT     600

using namespace std;
static MotherBoard mbrd;
void gb_keyboard_func(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool isPressed)
{
    if (!mbrd.cpu->flip())
    {
        return;
    }
    
    switch (key)
    {
        case KB_KEY_RIGHT:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_Right) : mbrd.mmu->joypad.keyup(JoypadKey_Right);
        }
        break;
        case KB_KEY_UP:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_Up) : mbrd.mmu->joypad.keyup(JoypadKey_Up);
        }
        break;
        case KB_KEY_LEFT:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_Left) : mbrd.mmu->joypad.keyup(JoypadKey_Left);
        }
        break;
        case KB_KEY_DOWN:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_Down) : mbrd.mmu->joypad.keyup(JoypadKey_Down);
        }
        break;
        case KB_KEY_Z:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_A) : mbrd.mmu->joypad.keyup(JoypadKey_A);
        }
        break;
        case KB_KEY_X:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_B) : mbrd.mmu->joypad.keyup(JoypadKey_B);
        }
        break;
        case KB_KEY_SPACE:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_Select) : mbrd.mmu->joypad.keyup(JoypadKey_Select);
        }
        break;
        case KB_KEY_ENTER:
        {
            isPressed ? mbrd.mmu->joypad.keydown(JoypadKey_Start) : mbrd.mmu->joypad.keyup(JoypadKey_Start);
        }
        break;
    
        default:
        break;
    }
}

int main(int argc, char **argv)
{
    // use gameboy::gpu::{SCREEN_H, SCREEN_W};    

    // rog::reg("gameboy");
    // rog::reg("gameboy::cartridge");

    string rom("boxes.gb");
    // bool c_audio = false;
    // int c_scale = 2;    
    {
        // let mut ap = argparse::ArgumentParser::new();
        // ap.set_description("Gameboy emulator");
        // ap.refer(&mut c_audio)
        //     .add_option(&["-a", "--enable-audio"], argparse::StoreTrue, "Enable audio");
        // ap.refer(&mut c_scale).add_option(
        //     &["-x", "--scale-factor"],
        //     argparse::Store,
        //     "Scale the video by a factor of 1, 2, 4, or 8",
        // );
        // ap.refer(&mut rom).add_argument("rom", argparse::Store, "Rom name");
        // ap.parse_args_or_exit();
    }        
    
    // if c_audio {
    //     initialize_audio(&mbrd);
    // }
    mbrd = MotherBoard::power_up(rom);

    int i, noise, carry, seed = 0xbeef;

    uint32_t g_buffer[WIDTH * HEIGHT];
    string rom_name = mbrd.mmu->cartridge->title();
    string title("Gameboy - { "+ rom_name +" }");
    struct mfb_window *window = mfb_open_ex(title.c_str(), WIDTH, HEIGHT, WF_RESIZABLE);
    if (!window)
        return 0;

    mfb_set_keyboard_callback(window, gb_keyboard_func);
    mfb_update_state state;
    do {        
        mbrd.next();
        if (mbrd.check_and_reset_gpu_updated())
        {
            uint16_t i = 0;
            for (size_t y = 0; y < 144; y++)
            {
                for (size_t w = 0; w < 160; i++)
                {
                    uint32_t b = mbrd.mmu->gpu->data[y][w][2];
                    uint32_t g = mbrd.mmu->gpu->data[y][w][1];
                    uint32_t r = mbrd.mmu->gpu->data[y][w][0];
                    uint32_t a = 0xff000000;

                    g_buffer[i++] = a|b|g|r;                    
                }                
            }            
        }
        
        mfb_update(window, g_buffer);
        if (!mfb_is_window_active(window)) {
            window = 0x0;
            break;
        }
    } while(mfb_wait_sync(window));
    // mbrd.mmu->cartridge->save(rom);

    return 0;
}