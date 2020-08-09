#include <iostream>
#include "Cartridge.h"

using namespace std;

int main(int argc, char **argv)
{
    Cartridge *tdg = createCart("boxes.gb");

    cout << tdg->title() << endl;    

    delete tdg;      

    return 0;
}