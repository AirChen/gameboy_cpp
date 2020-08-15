#include "Mmunit.h"

using namespace std;

Mmunit::Mmunit(string path) {
    Cartridge *cart = createCart(path);
    Term tt;
    uint8_t t = cart->get(0x0143) & 0x80;
    if (t == 0x80)
    {
        tt = Term_GBC;
    } else
    {
        tt = Term_GB;
    }
    
    Intf *ii = new Intf();

    cartridge = cart;
    apu = NULL;
    gpu = new Gpu(tt, ii);
    joypad = Joypad::power_up(ii);
    serial = Serial::power_up(ii);
    shift = false;
    speed = Speed_Normal;
    term = tt;
    timer = Timer::power_up(ii);
    inte = 0x00;
    intf = ii;
    hdma = new Hdma();
    hram = (uint8_t *)malloc(sizeof(uint8_t) * 0x7f);
    wram = (uint8_t *)malloc(sizeof(uint8_t) * 0x8000);
    wram_bank = 0x01;

    set(0xff05, 0x00);
    set(0xff06, 0x00);
    set(0xff07, 0x00);
    set(0xff10, 0x80);
    set(0xff11, 0xbf);
    set(0xff12, 0xf3);
    set(0xff14, 0xbf);
    set(0xff16, 0x3f);
    set(0xff16, 0x3f);
    set(0xff17, 0x00);
    set(0xff19, 0xbf);
    set(0xff1a, 0x7f);
    set(0xff1b, 0xff);
    set(0xff1c, 0x9f);
    set(0xff1e, 0xff);
    set(0xff20, 0xff);
    set(0xff21, 0x00);
    set(0xff22, 0x00);
    set(0xff23, 0xbf);
    set(0xff24, 0x77);
    set(0xff25, 0xf3);
    set(0xff26, 0xf1);
    set(0xff40, 0x91);
    set(0xff42, 0x00);
    set(0xff43, 0x00);
    set(0xff45, 0x00);
    set(0xff47, 0xfc);
    set(0xff48, 0xff);
    set(0xff49, 0xff);
    set(0xff4a, 0x00);
    set(0xff4b, 0x00);
}

Mmunit::~Mmunit()
{
    delete gpu;
    delete hdma;

    free(hram);
    free(wram);
}

uint32_t Mmunit::next(uint32_t cycles) {
    uint32_t cpu_divider = (uint32_t)speed;
    uint32_t vram_cycles = run_dma();
    uint32_t gpu_cycles = cycles / cpu_divider + vram_cycles;
    uint32_t cpu_cycles = cycles + vram_cycles * cpu_divider;
    timer.next(cpu_cycles);
    gpu->next(gpu_cycles);
    // apu->as_mut().map_or((), |s| s.next(gpu_cycles));
    return gpu_cycles;
}

void Mmunit::switch_speed() {
    if (shift) {
        if (speed == Speed_Double) {
            speed = Speed_Normal;
        } else {
            speed = Speed_Double;
        }
    }
    shift = false;
}

uint32_t Mmunit::run_dma() {
    if (!hdma->active) {
        return 0;
    }

    switch (hdma->mode)
    {
    case HdmaMode_Gdma:
    {
        uint32_t len = (uint32_t)hdma->remain + 1;
        for (size_t i = 0; i < len; i++)
        {
            run_dma_hrampart();
        }
        hdma->active = false;
        return (len * 8);
    }
        break;
    case HdmaMode_Hdma:
    {
        if (!gpu->h_blank)
        {
            return 0;
        }
        run_dma_hrampart();
        if (hdma->remain == 0x7f)
        {
            hdma->active = false;
        }
        return 8;        
    }
        break;
    
    default:
        break;
    }
    return 0;
}

void Mmunit::run_dma_hrampart() {
    uint16_t mmu_src = hdma->src;    
    for (size_t i = 0; i < 0x10; i++)
    {
        uint8_t b = get(mmu_src + i);
        gpu->set(hdma->dst + i, b);
    }    

    hdma->src += 0x10;
    hdma->dst += 0x10;
    if (hdma->remain == 0) {
        hdma->remain = 0x7f;
    } else {
        hdma->remain -= 1;
    }
}

uint8_t Mmunit::get(unsigned int a)
{
    if (a >= 0x0000 && a <= 0x7fff) { return cartridge->get(a); }
    if (a >= 0x8000 && a <= 0x9fff) { return gpu->get(a); }
    if (a >= 0xa000 && a <= 0xbfff) { return cartridge->get(a); }
    if (a >= 0xc000 && a <= 0xcfff) { return wram[a  - 0xc000]; }
    if (a >= 0xd000 && a <= 0xdfff) { return wram[a  - 0xd000 + 0x1000 * wram_bank]; }
    if (a >= 0xe000 && a <= 0xefff) { return wram[a  - 0xe000]; }
    if (a >= 0xf000 && a <= 0xfdff) { return wram[a  - 0xf000 + 0x1000 * wram_bank]; }
    if (a >= 0xfe00 && a <= 0xfe9f) { return gpu->get(a); }
    if (a >= 0xfea0 && a <= 0xfeff) { return 0x00; }
    if (a == 0xff00) return joypad.get(a);
    if (a >= 0xff01 && a <= 0xff02) { return serial.get(a); }
    if (a >= 0xff04 && a <= 0xff07) { return timer.get(a); }
    if (a == 0xff0f) return intf->data;
    if (a >= 0xff10 && a <= 0xff3f)
    {
        if (apu)
        {
            return apu->get(a);
        }
        return 0x00;
    }
    
    if (a == 0xff4d) {
        uint8_t a = (speed == Speed_Double) ? 0x80 : 0x00;
        uint8_t b = shift ? 0x01 : 0x00;
        return (a | b);
    }

    if ((a >= 0xff40 && a <= 0xff45) || (a >= 0xff47 && a <= 0xff4b) || a == 0xff4f)
    {
        return gpu->get(a);
    }
    
    if (a >= 0xff51 && a <= 0xff55) { return hdma->get(a); }
    if (a >= 0xff68 && a <= 0xff6b) { return gpu->get(a); }
    if (a == 0xff70) return wram_bank;
    if (a >= 0xff80 && a <= 0xfffe) { return hram[a  - 0xff80]; }
    if (a == 0xffff) return inte;
    
    return 0x00;
}

void Mmunit::set(unsigned int a, uint8_t v)
{
    if (a >= 0x0000 && a <= 0x7fff) { cartridge->set(a, v); }
    if (a >= 0x8000 && a <= 0x9fff) { gpu->set(a, v); }
    if (a >= 0xa000 && a <= 0xbfff) { cartridge->set(a, v); }
    if (a >= 0xc000 && a <= 0xcfff) { wram[a  - 0xc000] = v; }
    if (a >= 0xd000 && a <= 0xdfff) { wram[a  - 0xd000 + 0x1000 * wram_bank] = v; }
    if (a >= 0xe000 && a <= 0xefff) { wram[a  - 0xe000] = v; }
    if (a >= 0xf000 && a <= 0xfdff) { wram[a  - 0xf000 + 0x1000 * wram_bank] = v; }
    if (a >= 0xfe00 && a <= 0xfe9f) { gpu->set(a, v); }
    if (a >= 0xfea0 && a <= 0xfeff) { 0x00; }
    if (a == 0xff00) joypad.set(a, v);
    if (a >= 0xff01 && a <= 0xff02) { serial.set(a, v); }
    if (a >= 0xff04 && a <= 0xff07) { timer.set(a, v); }    
    // 0xff10..=0xff3f => self.apu.as_mut().map_or((), |s| s.set(a, v)),
    if (a == 0xff46)
    {
        uint16_t base = (uint16_t)v << 8;
        for (size_t i = 0; i < 0xa0; i++)
        {
            uint16_t b = get(base + i);
            set(0xfe00 + i, b);
        }        
    }
    
    if (a == 0xff4d) {
        shift = ((v & 0x01) == 0x01);
    }
        
    if ((a >= 0xff40 && a <= 0xff45) || (a >= 0xff47 && a <= 0xff4b) || a == 0xff4f)
    {
        gpu->set(a, v);
    }
    
    if (a >= 0xff51 && a <= 0xff55) { hdma->set(a, v); }
    if (a >= 0xff68 && a <= 0xff6b) { gpu->set(a, v); }
    if (a == 0xff0f) intf->data = v;    
    if (a == 0xff70) {
        uint8_t n = v & 0x7;
        if (n == 0)
        {
            wram_bank = 1;
        } else
        {
            wram_bank = n;
        }        
    };
    if (a >= 0xff80 && a <= 0xfffe) { hram[a  - 0xff80] = v; }
    if (a == 0xffff) inte = v;
}