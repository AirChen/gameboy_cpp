#include <iostream>
#include "machine.h"

int main(int argc, char **argv)
{
    Machine *machine = new Machine();
    machine->run();
    delete machine;
    
    return 0;
}