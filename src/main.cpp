#include "pty.h"

#include <iostream>

int main(int argc, char** argv) {
    Pecera::Pty* pty = new Pecera::Pty();
    Pecera::PtyInitResult result = pty->init();
    std::cout << "PtyInitResult: " << result << std::endl;

    //pthread_t read_write_thread;

    pty->readWriteLoop();
    //pthread_create(&read_write_thread, NULL, readWriteThread, pty);

    return 0;
}
