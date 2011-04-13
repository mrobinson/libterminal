#include "pty.h"

void *readWriteThread(void* voidPty) {
    //pty->readWriteLoop();
}

int main(int argc, char** argv) {
    Pecera::Pty* pty = new Pecera::Pty();
    pty->init();

    pthread_t read_write_thread; 
  
    pthread_create(&read_write_thread, NULL, readWriteThread, pty);

    pty->readProcessor();
}
