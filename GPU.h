#ifndef GPU_1
#define GPU_1

#include "Cartridge.h"
#include "CPU.h"
#include <vector>

typedef enum {
    // When using this transfer method, all data is transferred at once. The execution of the program is halted until
    // the transfer has completed. Note that the General Purpose DMA blindly attempts to copy the data, even if the
    // CD controller is currently accessing VRAM. So General Purpose DMA should be used only if the Display is disabled,
    // or during V-Blank, or (for rather short blocks) during H-Blank. The execution of the program continues when the
    // transfer has been completed, and FF55 then contains a value of FFh.
    HdmaMode_Gdma,
    // The H-Blank DMA transfers 10h bytes of data during each H-Blank, ie. at LY=0-143, no data is transferred during
    // V-Blank (LY=144-153), but the transfer will then continue at LY=00. The execution of the program is halted
    // during the separate transfers, but the program execution continues during the 'spaces' between each data block.
    // Note that the program should not change the Destination VRAM bank (FF4F), or the Source ROM/RAM bank (in case
    // data is transferred from bankable memory) until the transfer has completed! (The transfer should be paused as
    // described below while the banks are switched) Reading from Register FF55 returns the remaining length (divided
    // by 10h, minus 1), a value of 0FFh indicates that the transfer has completed. It is also possible to terminate
    // an active H-Blank transfer by writing zero to Bit 7 of FF55. In that case reading from FF55 will return how many
    // $10 "blocks" remained (minus 1) in the lower 7 bits, but Bit 7 will be read as "1". Stopping the transfer
    // doesn't set HDMA1-4 to $FF.
    HdmaMode_Hdma,
} HdmaMode;

class Hdma: public Memory
{
public:
    // These two registers specify the address at which the transfer will read data from. Normally, this should be
    // either in ROM, SRAM or WRAM, thus either in range 0000-7FF0 or A000-DFF0. [Note : this has yet to be tested on
    // Echo RAM, OAM, FEXX, IO and HRAM]. Trying to specify a source address in VRAM will cause garbage to be copied.
    // The four lower bits of this address will be ignored and treated as 0.    
    uint16_t src;
    // These two registers specify the address within 8000-9FF0 to which the data will be copied. Only bits 12-4 are
    // respected; others are ignored. The four lower bits of this address will be ignored and treated as 0.    
    uint16_t dst;
    bool active;     
    HdmaMode mode;
    uint8_t remain;    

public:
    Hdma(/* args */);    
    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
};

class Lcdc
{
public:
    uint8_t data;
public:
    Lcdc() { data = 0b01001000; };    

    // LCDC.7 - LCD Display Enable
    // This bit controls whether the LCD is on and the PPU is active. Setting it to 0 turns both off, which grants
    // immediate and full access to VRAM, OAM, etc.
    bool bit7() { return (data & 0b10000000 != 0x00); }

    // LCDC.6 - Window Tile Map Display Select
    // This bit controls which background map the Window uses for rendering. When it's reset, the $9800 tilemap is used,
    // otherwise it's the $9C00 one.
    bool bit6() { return (data & 0b01000000 != 0x00); }

    // LCDC.5 - Window Display Enable
    // This bit controls whether the window shall be displayed or not. (TODO : what happens when toggling this
    // mid-scanline ?) This bit is overridden on DMG by bit 0 if that bit is reset.
    // Note that on CGB models, setting this bit to 0 then back to 1 mid-frame may cause the second write to be ignored.
    bool bit5() { return (data & 0b00100000 != 0x00); }

    // LCDC.4 - BG & Window Tile Data Select
    // This bit controls which addressing mode the BG and Window use to pick tiles.
    // Sprites aren't affected by this, and will always use $8000 addressing mode.
    bool bit4() { return (data & 0b00010000 != 0x00); }

    // LCDC.3 - BG Tile Map Display Select
    // This bit works similarly to bit 6: if the bit is reset, the BG uses tilemap $9800, otherwise tilemap $9C00.
    bool bit3() { return (data & 0b00001000 != 0x00); }

    // LCDC.2 - OBJ Size
    // This bit controls the sprite size (1 tile or 2 stacked vertically).
    // Be cautious when changing this mid-frame from 8x8 to 8x16 : "remnants" of the sprites intended for 8x8 could
    // "leak" into the 8x16 zone and cause artifacts.
    bool bit2() { return (data & 0b00000100 != 0x00); }

    // LCDC.1 - OBJ Display Enable
    // This bit toggles whether sprites are displayed or not.
    // This can be toggled mid-frame, for example to avoid sprites being displayed on top of a status bar or text box.
    // (Note: toggling mid-scanline might have funky results on DMG? Investigation needed.)
    bool bit1() { return (data & 0b00000010 != 0x00); }


    // LCDC.0 - BG/Window Display/Priority
    // LCDC.0 has different meanings depending on Gameboy type and Mode:
    // Monochrome Gameboy, SGB and CGB in Non-CGB Mode: BG Display
    // When Bit 0 is cleared, both background and window become blank (white), and the Window Display Bit is ignored in
    // that case. Only Sprites may still be displayed (if enabled in Bit 1).
    // CGB in CGB Mode: BG and Window Master Priority
    // When Bit 0 is cleared, the background and window lose their priority - the sprites will be always displayed on
    // top of background and window, independently of the priority Flagss in OAM and BG Map attributes.
    bool bit0() { return (data & 0b00000001 != 0x00); }
};

// LCD Status Register.
class Stat {
public:
    // Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
    bool enable_ly_interrupt;
    // Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
    bool enable_m2_interrupt;
    // Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
    bool enable_m1_interrupt;
    // Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
    bool enable_m0_interrupt;
    // Bit 1-0 - Mode Flags       (Mode 0-3, see below) (Read Only)
    //    0: During H-Blank
    //    1: During V-Blank
    //    2: During Searching OAM
    //    3: During Transferring Data to LCD Driver
    uint8_t mode;
public:
    Stat() {
        enable_ly_interrupt = false;
        enable_m2_interrupt = false;
        enable_m1_interrupt = false;
        enable_m0_interrupt = false;
        mode = 0x00;
    };
};

// This register is used to address a byte in the CGBs Background Palette Memory. Each two byte in that memory define a
// color value. The first 8 bytes define Color 0-3 of Palette 0 (BGP0), and so on for BGP1-7.
//  Bit 0-5   Index (00-3F)
//  Bit 7     Auto Increment  (0=Disabled, 1=Increment after Writing)
// Data can be read/written to/from the specified index address through Register FF69. When the Auto Increment bit is
// set then the index is automatically incremented after each <write> to FF69. Auto Increment has no effect when
// <reading> from FF69, so the index must be manually incremented in that case. Writing to FF69 during rendering still
// causes auto-increment to occur.
// Unlike the following, this register can be accessed outside V-Blank and H-Blank.
class Bgpi {
public:
    uint8_t i;
    bool auto_increment;
public:
    Bgpi() 
    {
        i = 0x00;
        auto_increment = false;
    }

    uint8_t get()
    {
        uint8_t a = auto_increment ? 0x80 : 0x00;
        return a | i;
    }

    void set(uint8_t v)
    {
        auto_increment = (v & 0x80 != 0x00);
        i = v & 0x3f;
    }
};

typedef enum {
    GrayShades_White = 0xff,
    GrayShades_Light = 0xc0,
    GrayShades_Dark = 0x60,
    GrayShades_Black = 0x00,
} GrayShades;

// Bit7   OBJ-to-BG Priority (0=OBJ Above BG, 1=OBJ Behind BG color 1-3)
//     (Used for both BG and Window. BG color 0 is always behind OBJ)
// Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
// Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
// Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
// Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
// Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
class Attr {    
public:
    bool priority;
    bool yflip;
    bool xflip;
    uint8_t palette_number_0;
    bool bank;
    uint8_t palette_number_1;
public:
    Attr(uint8_t u)
    {
        priority = (u & (1 << 7) != 0);
        yflip = (u & (1 << 6) != 0);
        xflip = (u & (1 << 5) != 0);
        palette_number_0 = u & (1 << 4);
        bank = (u & (1 << 3) != 0);
        palette_number_1 = u & 0x07;
    }
};

typedef enum {
    Flags_VBlank  = 0,
    Flags_LCDStat = 1,
    Flags_Timer   = 2,
    Flags_Serial  = 3,
    Flags_Joypad  = 4,
} Flags;

struct Intf {
    uint8_t data;

    Intf() {
        data = 0x00;
    };

    void hi(Flags flags) {
        data |= 1 << (uint8_t)flags;
    }
};

static uint8_t SCREEN_W = 160;
static uint8_t SCREEN_H = 144;

typedef struct {
    bool priority;
    uint8_t color;
} Priority;

class Gpu: public Memory {

public:
    // Digital image with mode RGB. Size = 144 * 160 * 3.
    // 3---------
    // ----------
    // ----------
    // ---------- 160
    //        144
    uint8_t data[144][160][3];
    Intf *intf;
    Term term;
    bool h_blank;
    bool v_blank;

    Lcdc *lcdc;
    Stat *stat;

    // Scroll Y (R/W), Scroll X (R/W)
    // Specifies the position in the 256x256 pixels BG map (32x32 tiles) which is to be displayed at the upper/left LCD
    // display position. Values in range from 0-255 may be used for X/Y each, the video controller automatically wraps
    // back to the upper (left) position in BG map when drawing exceeds the lower (right) border of the BG map area.
    uint8_t sy;
    uint8_t sx;
    // Window Y Position (R/W), Window X Position minus 7 (R/W)
    uint8_t wy;
    uint8_t wx;
    // The LY indicates the vertical line to which the present data is transferred to the LCD Driver. The LY can take
    // on any value between 0 through 153. The values between 144 and 153 indicate the V-Blank period. Writing will
    // reset the counter.
    uint8_t ly;
    // The Gameboy permanently compares the value of the LYC and LY registers. When both values are identical, the
    // coincident bit in the STAT register becomes set, and (if enabled) a STAT interrupt is requested.
    uint8_t lc;

    // This register assigns gray shades to the color numbers of the BG and Window tiles.
    uint8_t bgp;
    // This register assigns gray shades for sprite palette 0. It works exactly as BGP (FF47), except that the lower
    // two bits aren't used because sprite data 00 is transparent.
    uint8_t op0;
    // This register assigns gray shades for sprite palette 1. It works exactly as BGP (FF47), except that the lower
    // two bits aren't used because sprite data 00 is transparent.
    uint8_t op1;

    Bgpi *cbgpi;
    // This register allows to read/write data to the CGBs Background Palette Memory, addressed through Register FF68.
    // Each color is defined by two bytes (Bit 0-7 in first byte).
    //     Bit 0-4   Red Intensity   (00-1F)
    //     Bit 5-9   Green Intensity (00-1F)
    //     Bit 10-14 Blue Intensity  (00-1F)
    // Much like VRAM, data in Palette Memory cannot be read/written during the time when the LCD Controller is
    // reading from it. (That is when the STAT register indicates Mode 3). Note: All background colors are initialized
    // as white by the boot ROM, but it's a good idea to initialize at least one color yourself (for example if you
    // include a soft-reset mechanic).
    //
    // Note: Type [[[u8; 3]; 4]; 8] equals with [u8; 64].
    uint8_t cbgpd[8][4][3];

    Bgpi *cobpi;
    uint8_t cobpd[8][4][3];

    uint8_t *ram;
    uint8_t ram_bank;
    // VRAM Sprite Attribute Table (OAM)
    // Gameboy video controller can display up to 40 sprites either in 8x8 or in 8x16 pixels. Because of a limitation of
    // hardware, only ten sprites can be displayed per scan line. Sprite patterns have the same format as BG tiles, but
    // they are taken from the Sprite Pattern Table located at $8000-8FFF and have unsigned numbering.
    // Sprite attributes reside in the Sprite Attribute Table (OAM - Object Attribute Memory) at $FE00-FE9F. Each of the 40
    // entries consists of four bytes with the following meanings:
    // Byte0 - Y Position
    // Specifies the sprites vertical position on the screen (minus 16). An off-screen value (for example, Y=0 or
    // Y>=160) hides the sprite.
    //
    // Byte1 - X Position
    // Specifies the sprites horizontal position on the screen (minus 8). An off-screen value (X=0 or X>=168) hides the
    // sprite, but the sprite still affects the priority ordering - a better way to hide a sprite is to set its
    // Y-coordinate off-screen.
    //
    // Byte2 - Tile/Pattern Number
    // Specifies the sprites Tile Number (00-FF). This (unsigned) value selects a tile from memory at 8000h-8FFFh. In
    // CGB Mode this could be either in VRAM Bank 0 or 1, depending on Bit 3 of the following byte. In 8x16 mode, the
    // lower bit of the tile number is ignored. IE: the upper 8x8 tile is "NN AND FEh", and the lower 8x8 tile
    // is "NN OR 01h".
    //
    // Byte3 - Attributes/Flagss:
    // Bit7   OBJ-to-BG Priority (0=OBJ Above BG, 1=OBJ Behind BG color 1-3)
    //        (Used for both BG and Window. BG color 0 is always behind OBJ)
    // Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
    // Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
    // Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
    // Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
    // Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
    uint8_t *oam;
    
    Priority *prio;

    // The LCD controller operates on a 222 Hz = 4.194 MHz dot clock. An entire frame is 154 scanlines, 70224 dots, or
    // 16.74 ms. On scanlines 0 through 143, the LCD controller cycles through modes 2, 3, and 0 once every 456 dots.
    // Scanlines 144 through 153 are mode 1.
    uint32_t dots;

public:
    Gpu(Term term, Intf *intf);
    ~Gpu();

    uint8_t get_ram0(uint16_t a);
    uint8_t get_ram1(uint16_t a);    
    GrayShades get_gray_shades(uint8_t v, uint8_t i);

    // Grey scale.
    void set_gre(uint8_t x, uint8_t g);
    void set_rgb(uint8_t x, uint8_t r, uint8_t g, uint8_t b);

    void next(uint32_t cycles);

    void draw_bg();
    void draw_sprites();

    uint8_t get(unsigned int a);
    void set(unsigned int a, uint8_t v);
};

#endif