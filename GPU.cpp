#include "GPU.h"
#include "Util.h"
#include <iostream>
#include <assert.h>

using namespace std;

Hdma::Hdma(/* args */)
{
    src = 0x0000;
    dst = 0x8000;
    active = false;
    mode = HdmaMode_Gdma;
    remain = 0x00;
}

uint8_t Hdma::get(unsigned int a)
{
    switch (a)
    {
    case 0xff51:
        return (uint8_t)(src >> 8);
        break;
    case 0xff52:
        return (uint8_t)(src);
        break;
    case 0xff43:
        return (uint8_t)(dst >> 8);
        break;
    case 0xff54:
        return (uint8_t)(dst);
        break;
    case 0xff55:
        return (uint8_t)(remain | active ? 0x00 : 0x80);
        break;    
    default:
        cout << "Hdma get error!" << endl;
        return 0x00;
        break;
    }
}

void Hdma::set(unsigned int a, uint8_t v)
{
    switch (a)
    {
    case 0xff51:
        src = (uint16_t)(v << 8) | (src & 0x00ff);
        break;
    case 0xff52:
        src = (uint16_t)(v & 0xf0) | (src & 0xff00);
        break;
    case 0xff53:
        dst = 0x8000 | (uint16_t)(v & 0x1f) | (dst & 0x00ff);
        break;
    case 0xff54:
        dst = (uint16_t)(v & 0xf0) | (dst & 0xff00);
        break;
    case 0xff55:
        {
            if (active && mode == HdmaMode_Hdma)
            {
                if (v & 0x80 == 0x00)
                {
                    active = false;
                }
                return;
            }
            active = true;
            remain = v & 0x7f;
            mode = (v & 0x80 != 0x00) ? HdmaMode_Hdma : HdmaMode_Gdma;            
        }
        break;

    default:
        cout << "Hdma get error!" << endl;
        break;
    }   
}

inline void initalArray(uint8_t v, uint8_t **array, int d1, int d2, int d3)
{
    uint8_t ***q = (uint8_t ***)*array;
    for (size_t i = 0; i < d1; i++)
    {
        for (size_t j = 0; j < d2; j++)
        {
            for (size_t z = 0; z < d3; z++)
            {                
                // *(array + i * d2 * d3 + j * d3 + z) = v;
                q[i][j][z] = v;
            }            
        }        
    }
}

Gpu::Gpu(Term term, Intf intf) {    

    data = (uint8_t ***)malloc(sizeof(uint8_t) * SCREEN_H * SCREEN_W * 3);
    initalArray(0xff, (uint8_t **)&data[0][0][0], SCREEN_H, SCREEN_W, 3);
    
    intf = intf;    
    term = term;
    h_blank = false;
    v_blank = false;

    lcdc = new Lcdc();
    stat = new Stat();
    sy = 0x00;
    sx = 0x00;
    wx = 0x00;
    wy = 0x00;
    ly = 0x00;
    lc = 0x00;
    bgp = 0x00;
    op0 = 0x00;
    op1 = 0x01;
    cbgpi = new Bgpi();   
    cbgpd = (uint8_t ***)malloc(sizeof(uint8_t) * 8 * 4 * 3); 
    initalArray(0, (uint8_t **)&cbgpd[0][0][0], 8, 4, 3);
    
    cobpi = new Bgpi();    
    cobpd = (uint8_t ***)malloc(sizeof(uint8_t) * 8 * 4 * 3);
    initalArray(0, (uint8_t **)&cobpd[0][0][0], 8, 4, 3);
    ram = (uint8_t *)malloc(sizeof(uint8_t) * 0x4000);
    // ram = [0x00; 0x4000], // 默认是 0
    ram_bank = 0x00;
    oam = (uint8_t *)malloc(sizeof(uint8_t) * 0xa0);
    // oam = [0x00; 0xa0],    

    prio = (Priority *)malloc(sizeof(Priority) * SCREEN_W);
    for (size_t i = 0; i < SCREEN_W; i++)
    {
        prio[i] = {true, 0};
    }    

    dots = 0;    
}

uint8_t Gpu::get_ram0(uint16_t a)
{
    return ram[a - 0x8000];
}    

uint8_t Gpu::get_ram1(uint16_t a)
{
    return ram[a - 0x6000];
}    

// This register assigns gray shades to the color numbers of the BG and Window tiles.
// Bit 7-6 - Shade for Color Number 3
// Bit 5-4 - Shade for Color Number 2
// Bit 3-2 - Shade for Color Number 1
// Bit 1-0 - Shade for Color Number 0
// The four possible gray shades are:
// 0  White
// 1  Light gray
// 2  Dark gray
// 3  Black
GrayShades Gpu::get_gray_shades(uint8_t v, uint8_t i) {
    uint8_t q = v >> (2 * i) & 0x03;

    switch (q)
    {
    case 0x00:
        return GrayShades_White;
        break;
    case 0x01:
        return GrayShades_Light;
        break;
    case 0x02:
        return GrayShades_Dark;
        break;    
    default:
        return GrayShades_Black;
        break;
    }
}

// Grey scale.
void Gpu::set_gre(uint8_t x, uint8_t g) {
    data[ly][x][0] = g;
    data[ly][x][1] = g;
    data[ly][x][2] = g;
}

// When developing graphics on PCs, note that the RGB values will have different appearance on CGB displays as on
// VGA/HDMI monitors calibrated to sRGB color. Because the GBC is not lit, the highest intensity will produce Light
// Gray color rather than White. The intensities are not linear; the values 10h-1Fh will all appear very bright,
// while medium and darker colors are ranged at 00h-0Fh.
// The CGB display's pigments aren't perfectly saturated. This means the colors mix quite oddly; increasing
// intensity of only one R,G,B color will also influence the other two R,G,B colors. For example, a color setting
// of 03EFh (Blue=0, Green=1Fh, Red=0Fh) will appear as Neon Green on VGA displays, but on the CGB it'll produce a
// decently washed out Yellow. See image on the right.
void Gpu::set_rgb(uint8_t x, uint8_t r, uint8_t g, uint8_t b) {
    assert(r > 0x1f);
    assert(g > 0x1f);
    assert(b > 0x1f);
    uint32_t r_ = (uint32_t)r;
    uint32_t g_ = (uint32_t)g;
    uint32_t b_ = (uint32_t)b;
    data[ly][x][0] = (uint8_t)((r_ * 13 + g_ * 2 + b_) >> 1);
    data[ly][x][1] = (uint8_t)((g_ * 3 + b_) << 1);
    data[ly][x][2] = (uint8_t)((r_ * 3 + g_ * 2 + b_ * 11) >> 1);    
}

void Gpu::next(uint32_t cycles) {
    if (!lcdc->bit7()) {
        return;
    }
    h_blank = false;

    // The LCD controller operates on a 222 Hz = 4.194 MHz dot clock. An entire frame is 154 scanlines, 70224 dots,
    // or 16.74 ms. On scanlines 0 through 143, the LCD controller cycles through modes 2, 3, and 0 once every 456
    // dots. Scanlines 144 through 153 are mode 1.
    //
    // 1 scanline = 456 dots
    //
    // The following are typical when the display is enabled:
    // Mode 2  2_____2_____2_____2_____2_____2___________________2____
    // Mode 3  _33____33____33____33____33____33__________________3___
    // Mode 0  ___000___000___000___000___000___000________________000
    // Mode 1  ____________________________________11111111111111_____
    if (cycles == 0) {
        return;
    }
    uint32_t c = (cycles - 1) / 80 + 1;
    for (size_t i = 0; i < c; i++)
    {
        if (i == (c - 1)) {
            dots += cycles % 80;
        } else {
            dots += 80;
        }
        uint32_t d = dots;
        dots %= 456;
        if (d != dots) {
            ly = (ly + 1) % 154;
            if (stat->enable_ly_interrupt && ly == lc) {
                intf->hi(Flags_LCDStat);
            }
        }
        if (ly >= 144) {
            if (stat->mode == 1) {
                continue;
            }
            stat->mode = 1;
            v_blank = true;
            intf->hi(Flags_VBlank);
            if (stat->enable_m1_interrupt) {
                intf->hi(Flags_LCDStat);
            }
        } else if (dots <= 80) {
            if (stat->mode == 2) {
                continue;
            }
            stat->mode = 2;
            if (stat->enable_m2_interrupt) {
                intf->hi(Flags_LCDStat);
            }
        } else if (dots <= (80 + 172)) {
            stat->mode = 3;
        } else {
            if (stat->mode == 0) {
                continue;
            }
            stat->mode = 0;
            h_blank = true;
            if (stat->enable_m0_interrupt) {
                intf->hi(Flags_LCDStat);
            }
            // Render scanline
            if (term == Term_GBC || lcdc->bit0()) {
                draw_bg();
            }
            if (lcdc->bit1()) {
                draw_sprites();
            }
        }
    }
}

void Gpu::draw_bg() {
    bool show_window = (lcdc->bit5() && wy <= ly);
    uint16_t tile_base = lcdc->bit4() ? 0x8000 : 0x8800;

    uint8_t wx_ = wrapping_sub(wx, 7);
    uint8_t py = show_window ? wrapping_sub(ly, wy) : wrapping_add(sy, ly);    
    uint16_t ty = (uint16_t)py >> 3 & 31;    

    for (size_t x = 0; x < SCREEN_W; x++)
    {
        uint8_t px = (show_window && x >= wx) ? ((uint8_t)x - wx_) : wrapping_add(sx, (uint8_t)x);
        uint16_t tx = ((uint16_t)px >> 3) & 31;                

        // Background memory base addr.
        uint16_t bg_base = (show_window && (uint8_t)x >= wx) ? (lcdc->bit6() ? 0x9c00 : 0x9800) : (lcdc->bit3() ? 0x9c00 : 0x9800);        

        // Tile data
        // Each tile is sized 8x8 pixels and has a color depth of 4 colors/gray shades.
        // Each tile occupies 16 bytes, where each 2 bytes represent a line:
        // Byte 0-1  First Line (Upper 8 pixels)
        // Byte 2-3  Next Line
        // etc.
        uint16_t tile_addr = bg_base + ty * 32 + tx;
        uint8_t tile_number = get_ram0(tile_addr);
        uint16_t tile_offset = (uint16_t)(lcdc->bit4() ? (int16_t)tile_number : (int16_t)(int8_t)tile_number + 128) * 16;
        
        uint16_t tile_location = tile_base + tile_offset;
        Attr tile_attr(get_ram1(tile_addr));        
        uint8_t tile_y = tile_attr.yflip ? (7 - py % 8) : (py % 8);

        uint8_t tile_y_data[2];
        if (term == Term_GBC && tile_attr.bank)
        {
            tile_y_data[0] = get_ram1(tile_location + (uint16_t)(tile_y * 2));
            tile_y_data[1] = get_ram1(tile_location + (uint16_t)(tile_y * 2) + 1);
        } else
        {
            tile_y_data[0] = get_ram0(tile_location + (uint16_t)(tile_y * 2));
            tile_y_data[1] = get_ram0(tile_location + (uint16_t)(tile_y * 2) + 1);
        }
        
        uint8_t tile_x = tile_attr.xflip ? (7 - px % 8) : (px % 8);        

        // Palettes        
        uint8_t color_l = (tile_y_data[0] & (0x80 >> tile_x) != 0) ? 1 : 0;
        uint8_t color_h = (tile_y_data[1] & (0x80 >> tile_x) != 0) ? 2 : 0;
        uint8_t color = color_l | color_h;

        // Priority
        prio[x] = {tile_attr.priority, color};

        if (term == Term_GBC) {
            uint8_t r = cbgpd[tile_attr.palette_number_1][color][0];
            uint8_t g = cbgpd[tile_attr.palette_number_1][color][1];
            uint8_t b = cbgpd[tile_attr.palette_number_1][color][2];            
            set_rgb((uint8_t)x, r, g, b);
        } else {
            uint8_t color = (uint8_t)get_gray_shades(bgp, color);
            set_gre(x, color);
        }
    }
}

// Gameboy video controller can display up to 40 sprites either in 8x8 or in 8x16 pixels. Because of a limitation
// of hardware, only ten sprites can be displayed per scan line. Sprite patterns have the same format as BG tiles,
// but they are taken from the Sprite Pattern Table located at $8000-8FFF and have unsigned numbering.
//
// Sprite attributes reside in the Sprite Attribute Table (OAM - Object Attribute Memory) at $FE00-FE9F. Each of
// the 40 entries consists of four bytes with the following meanings:
//   Byte0 - Y Position
//   Specifies the sprites vertical position on the screen (minus 16). An off-screen value (for example, Y=0 or
//   Y>=160) hides the sprite.
//
//   Byte1 - X Position
//   Specifies the sprites horizontal position on the screen (minus 8). An off-screen value (X=0 or X>=168) hides
//   the sprite, but the sprite still affects the priority ordering - a better way to hide a sprite is to set its
//   Y-coordinate off-screen.
//
//   Byte2 - Tile/Pattern Number
//   Specifies the sprites Tile Number (00-FF). This (unsigned) value selects a tile from memory at 8000h-8FFFh. In
//   CGB Mode this could be either in VRAM Bank 0 or 1, depending on Bit 3 of the following byte. In 8x16 mode, the
//   lower bit of the tile number is ignored. IE: the upper 8x8 tile is "NN AND FEh", and the lower 8x8 tile is
//   "NN OR 01h".
//
//   Byte3 - Attributes/Flags:
//     Bit7   OBJ-to-BG Priority (0=OBJ Above BG, 1=OBJ Behind BG color 1-3)
//           (Used for both BG and Window. BG color 0 is always behind OBJ)
//     Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
//     Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
//     Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
//     Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
//     Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
void Gpu::draw_sprites() {
    // Sprite tile size 8x8 or 8x16(2 stacked vertically).
    uint8_t sprite_size = lcdc->bit2() ? 16 : 8;    
    
    for (size_t i = 0; i < 40; i++) {
        uint16_t sprite_addr = 0xfe00 + (uint16_t)i * 4;
        uint8_t py = wrapping_sub(get(sprite_addr), 16);
        uint8_t px = wrapping_sub(get(sprite_addr + 1), 8);
        uint8_t tile_number = get(sprite_addr + 2) & (lcdc->bit2() ? 0xfe : 0xff);
        Attr tile_attr(get(sprite_addr + 3));

        // If this is true the scanline is out of the area we care about
        if (py <= 0xff - sprite_size + 1) {
            if (ly < py || ly > py + sprite_size - 1) {
                continue;
            }
        } else {
            if (ly > wrapping_add(py, sprite_size) - 1) {
                continue;
            }
        }
        if (px >= (uint8_t)SCREEN_W && px <= (0xff - 7)) {
            continue;
        }

        uint8_t tile_y = (tile_attr.yflip) ? (sprite_size - 1 - wrapping_sub(ly, py)) : (wrapping_sub(ly, py));
        uint16_t tile_y_addr = 0x8000 + (uint16_t)tile_number * 16 + (uint16_t)tile_y * 2;

        uint8_t tile_y_data[2];
        if (term == Term_GBC && tile_attr.bank)
        {
            tile_y_data[0] = get_ram1(tile_y_addr);
            tile_y_data[1] = get_ram1(tile_y_addr + 1);
        } else
        {
            tile_y_data[0] = get_ram0(tile_y_addr);
            tile_y_data[1] = get_ram0(tile_y_addr + 1);
        }
    
        for (size_t x = 0; x < 8; x++) {
            if (wrapping_add(px, x) >= (uint8_t)SCREEN_W) {
                continue;
            }
            size_t tile_x = tile_attr.xflip ? (7 - x) : x;

            // Palettes            
            uint8_t color_l = (tile_y_data[0] & (0x80 >> tile_x) != 0) ? 1 : 0;
            uint8_t color_h = (tile_y_data[1] & (0x80 >> tile_x) != 0) ? 2 : 0;
            uint8_t color = color_h | color_l;
            if (color == 0) {
                continue;
            }

            // Confirm the priority of background and sprite.
            Priority prio_ = prio[wrapping_add(px, x)];

            bool skip = (term == Term_GBC && !lcdc->bit0()) ? (prio_.color == 0) : (prio_.priority ? (prio_.color != 0) : (tile_attr.priority && prio_.color != 0));            
            if (skip) {
                continue;
            }

            if (term == Term_GBC) {
                uint8_t r = cobpd[tile_attr.palette_number_1][color][0];
                uint8_t g = cobpd[tile_attr.palette_number_1][color][1];
                uint8_t b = cobpd[tile_attr.palette_number_1][color][2];
                set_rgb((uint8_t)wrapping_add(px, x), r, g, b);
            } else {
                uint8_t color = (tile_attr.palette_number_0 == 1) ? (uint8_t)get_gray_shades(op1, color) : get_gray_shades(op0, color);                
                set_gre((uint8_t)wrapping_add(px, x), color);
            }
        }
    }
}

uint8_t Gpu::get(unsigned int a) {
    if (a >= 0x8000 && a < 0x9fff)
    {
        return ram[ram_bank * 0x2000 + a - 0x8000];
    }

    if (a >= 0xfe00 && a < 0xfe9f)
    {
        return oam[a - 0xfe00];
    }
    
    switch (a)
    {
        case 0xff40: return lcdc->data; break;
        case 0xff41: 
        {
            uint8_t bit6 = stat->enable_ly_interrupt ? 0x40 : 0x00 ;
            uint8_t bit5 = stat->enable_m2_interrupt ? 0x20 : 0x00 ;
            uint8_t bit4 = stat->enable_m1_interrupt ? 0x10 : 0x00 ;
            uint8_t bit3 = stat->enable_m0_interrupt ? 0x08 : 0x00 ;
            uint8_t bit2 = (ly == lc) ? 0x04 : 0x00 ;
            return (bit6 | bit5 | bit4 | bit3 | bit2 | stat->mode);
        }
        break;
        case 0xff42: return sy; break;
        case 0xff43: return sx; break;
        case 0xff44: return ly; break;
        case 0xff45: return lc; break;
        case 0xff47: return bgp; break;
        case 0xff48: return op0; break;
        case 0xff49: return op1; break;
        case 0xff4a: return wy; break;
        case 0xff4b: return wx; break;
        case 0xff4f: return 0xfe | ram_bank; break;
        case 0xff68: return cbgpi->get(); break;
        case 0xff69: 
        {
            uint8_t r = cbgpi->i >> 3;
            uint8_t c = cbgpi->i >> 1 & 0x3;
            if ((cbgpi->i & 0x01) == 0x00) {
                uint8_t a = cbgpd[r][c][0];
                uint8_t b = cbgpd[r][c][1] << 5;
                return (a | b);
            } else {
                uint8_t a = cbgpd[r][c][1] >> 3;
                uint8_t b = cbgpd[r][c][2] << 2;
                return (a | b);
            }
        }
        break;        
        case 0xff6a: return cobpi->get(); break;
        case 0xff6b: 
        {
            uint8_t r = cobpi->i >> 3;
            uint8_t c = cobpi->i >> 1 & 0x3;
            if ((cobpi->i & 0x01) == 0x00) {
                uint8_t a = cobpd[r][c][0];
                uint8_t b = cobpd[r][c][1] << 5;
                return (a | b);
            } else {
                uint8_t a = cobpd[r][c][1] >> 3;
                uint8_t b = cobpd[r][c][2] << 2;
                return (a | b);
            }
        }
        break;
        default:
            cout << "Gpu::get error" << endl;
            break;
    }
}

void Gpu::set(unsigned int a, uint8_t v)
{
    if (a >= 0x8000 && a < 0x9fff)
    {
        ram[ram_bank * 0x2000 + a - 0x8000] = v;
    }

    if (a >= 0xfe00 && a < 0xfe9f)
    {
        oam[a - 0xfe00] = v;
    }
    
    switch (a)
    {
        case 0xff40: 
        {            
            lcdc->data = v;
            if (!lcdc->bit7())
            {
                dots = 0;
                ly = 0;
                stat->mode = 0;
                initalArray(0xff, (uint8_t **)&data[0][0][0], SCREEN_H, SCREEN_W, 3);
                v_blank = true;
            }                
        }
        break;
        case 0xff41: 
        {
            stat->enable_ly_interrupt = v & 0x40 != 0x00;
            stat->enable_m2_interrupt = v & 0x20 != 0x00;
            stat->enable_m1_interrupt = v & 0x10 != 0x00;
            stat->enable_m0_interrupt = v & 0x08 != 0x00;
        }
        break;
        case 0xff42: sy = v; break;
        case 0xff43: sx = v; break;
        case 0xff44: {}; break;
        case 0xff45: lc = v; break;
        case 0xff47: bgp = v; break;
        case 0xff48: op0 = v; break;
        case 0xff49: op1 = v; break;
        case 0xff4a: wy = v; break;
        case 0xff4b: wx = v; break;
        case 0xff4f: 
        {
            ram_bank = (v & 0x01);
        } 
        break;
        case 0xff68: cbgpi->set(v); break;
        case 0xff69: 
        {
            uint8_t r = cbgpi->i >> 3;
            uint8_t c = cbgpi->i >> 1 & 0x03;
            if ((cbgpi->i & 0x01) == 0x00) {
                cbgpd[r][c][0] = v & 0x1f;
                cbgpd[r][c][1] = (cbgpd[r][c][1] & 0x18) | (v >> 5);
            } else {
                cbgpd[r][c][1] = (cbgpd[r][c][1] & 0x07) | ((v & 0x03) << 3);
                cbgpd[r][c][2] = (v >> 2) & 0x1f;
            }
            if (cbgpi->auto_increment) {
                cbgpi->i += 0x01;
                cbgpi->i &= 0x3f;
            }
        }
        break;        
        case 0xff6a: cobpi->set(v); break;
        case 0xff6b: 
        {
            uint8_t r = cobpi->i >> 3;
            uint8_t c = cobpi->i >> 1 & 0x03;
            if ((cobpi->i & 0x01) == 0x00) {
                cobpd[r][c][0] = v & 0x1f;
                cobpd[r][c][1] = (cobpd[r][c][1] & 0x18) | (v >> 5);
            } else {
                cobpd[r][c][1] = (cobpd[r][c][1] & 0x07) | ((v & 0x03) << 3);
                cobpd[r][c][2] = (v >> 2) & 0x1f;
            }
            if (cobpi->auto_increment) {
                cobpi->i += 0x01;
                cobpi->i &= 0x3f;
            }
        }
        break;
        default:
            cout << "Gpu::set error" << endl;
            break;
    }
}
