#ifndef MotherBoard_1
#define MotherBoard_1

#include "CPU.h"
#include "Mmunit.h"
#include <string>

using namespace std;

struct MotherBoard
{
    Rtc *cpu;
    Mmunit *mmu;

    MotherBoard(string path) {
        mmu = new Mmunit(path);
        cpu = new Rtc(mmu->term, mmu);
    }

    ~MotherBoard() {
        delete cpu;
        delete mmu;
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