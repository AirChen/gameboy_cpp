#ifndef MotherBoard_1
#define MotherBoard_1

#include "CPU.h"
#include "Mmunit.h"
#include <string>

using namespace std;

typedef struct MotherBoard
{
    Rtc *cpu;
    Mmunit *mmu;

    static MotherBoard power_up(string path)
    {
        MotherBoard board;
        board.mmu = new Mmunit(path);
        board.cpu = new Rtc(board.mmu->term, board.mmu);
        return board;
    }

    uint32_t next()
    {
        cout << "pc ->> " << cpu->cpu->reg->pc << endl;

        if (mmu->get(cpu->cpu->reg->pc) == 0x10)
        {
            mmu->switch_speed();
        }

        uint32_t cycles = cpu->next();
        mmu->next(cycles);
        return cycles;
    }

    bool check_and_reset_gpu_updated()
    {
        bool result = mmu->gpu->v_blank;
        mmu->gpu->v_blank = false;
        return result;
    }
};

#endif