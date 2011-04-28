#include "pty.h"
#include <iostream>

int main(int argc, char** argv)
{
    Pty* pty = new Pty();
    std::cout << "Created new pty: " << pty << std::endl;
    while(true) {

    }
    return 0;
}
