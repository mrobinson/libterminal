#include "pty.h"
#include <iostream>

#define	BUFFSIZE 512

int main(int argc, char** argv)
{
    int amount, wamount, towrite;
    char buf[BUFFSIZE];
    Pty* pty = new Pty();
    std::cout << "Created new pty: " << pty << std::endl;
    
    while(true) {
        amount = read(STDIN_FILENO, buf, BUFFSIZE);
        towrite = amount;
        while(towrite > 0) {
            wamount = write(pty->masterfd, buf + (amount - towrite), towrite);
            towrite = towrite - wamount;
        }
    }
    return 0;
}
