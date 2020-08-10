#include "GPU.h"
#include <iostream>

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