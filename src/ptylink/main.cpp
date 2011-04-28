#include "pty.h"
#include "VT100.h"
#include "SimpleVT100Client.h"
#include <iostream>

int main(int argc, char** argv)
{
    VT100 vt100(new SimpleVT100Client());
    Pty* pty = new Pty(&vt100);

    std::cout << "Created new pty: " << pty << std::endl;
    while(true) {
        sleep(1);
    }

    return 0;
}
