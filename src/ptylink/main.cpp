#include "pty.h"
#include "VT100.h"
#include "SimpleVT100Client.h"
#include <iostream>

#define	BUFFSIZE 512

int main(int argc, char** argv)
{
    int amount, wamount, towrite;
    char buf[BUFFSIZE];
    VT100 vt100(new SimpleVT100Client());
    Pty* pty = new Pty(&vt100);

    std::cout << "Created new pty: " << pty << std::endl;
    
    while(true) {
        amount = read(STDIN_FILENO, buf, BUFFSIZE);
        towrite = amount;
        while(towrite > 0) {
            wamount = write(pty->masterfd, buf + (amount - towrite), towrite);
            towrite = towrite - wamount;
        }
        sleep(1);
    }

    return 0;
}
