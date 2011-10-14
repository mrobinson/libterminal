#ifndef PECERA_PTY_H
#define PECERA_PTY_H

#include <string>
#include <pthread.h>
#include <semaphore.h>

enum PtyInitResult {
    SUCCESS,
    ERROR_GET_USERS_SHELL,
    ERROR_TCGETATTR,
    ERROR_IOCTL_TIOCGWINSZ,
    ERROR_OPENPT,
    ERROR_GRANTPT,
    ERROR_UNLOCKPT,
    ERROR_PTSNAME,
    ERROR_SET_NONBLOCK,
    ERROR_FORK
};

enum PtyUnputResult {
    UNPUT_SUCCESS,
    UNPUT_ERROR_UNDERFLOW
};

const int PTY_BUFFER_SIZE = 1024;
const int PTY_READ_BUFFER_SIZE = 1024;

class VT100;

class Pty {
public:
    Pty(VT100* emulator);
    ~Pty();
    PtyInitResult init(const std::string& pathToExecutable);
    int ptyWrite(const char* buffer, const int count);
    int putChar(const char character);
    void setSize(int columns, int rows);

    void readWriteLoop();

    int masterfd_;

private:
    VT100* emulator;
    char* readBuffer_;

    char* writeBuffer_;
    char* writeEnd_;
    pthread_mutex_t writeMutex_;

    pthread_t readWriteThread;
    sem_t loopSemaphore_;

    void closeMasterFd();
    void initializeReadWriteLoop();
};

#endif
