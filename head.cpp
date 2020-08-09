#include <iostream>
// #include "Cartridge.h"
#include "include/MiniFB_cpp.h"

#define WIDTH      800
#define HEIGHT     600

using namespace std;

int main(int argc, char **argv)
{
    // Cartridge *tdg = createCart("boxes.gb");
    // cout << tdg->title() << endl;    
    // delete tdg;      

    
    int i, noise, carry, seed = 0xbeef;

    unsigned int g_buffer[WIDTH * HEIGHT];
    struct mfb_window *window = mfb_open_ex("Noise Test", WIDTH, HEIGHT, WF_RESIZABLE);
    if (!window)
        return 0;

    mfb_update_state state;
    do {
        for (i = 0; i < WIDTH * HEIGHT; ++i) {
            noise = seed;
            noise >>= 3;
            noise ^= seed;
            carry = noise & 1;
            noise >>= 1;
            seed >>= 1;
            seed |= (carry << 30);
            noise &= 0xFF;
            g_buffer[i] = MFB_RGB(noise, noise, noise); 
        }

        state = mfb_update(window, g_buffer);
        if (state != STATE_OK) {
            window = 0x0;
            break;
        }
    } while(mfb_wait_sync(window));

    return 0;
}