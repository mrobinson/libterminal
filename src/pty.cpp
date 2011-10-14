// -*- Mode: C; indent-tabs-mode: nil -*-

#include "pty.h"

#include "VT100.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <cstring>
#include <iostream>

static std::string getUsersShell()
{
    // getpwuid isn't reentrant
    struct passwd* pass = getpwuid(getuid());
    if ((pass != NULL) && (pass->pw_shell != NULL)) {
        return pass->pw_shell;
    }

    if (char* shellEnv = getenv("SHELL")) {
        return shellEnv;
    }

    return std::string();
}

Pty::Pty(VT100* emulator)
    : masterfd_(-1)
    , emulator(emulator)
{
    std::string shellPath = getUsersShell();
    if (shellPath.empty()) {
        fprintf(stderr, "Could not find your shell.");
        exit(ERROR_GET_USERS_SHELL);
    }

    init(shellPath);
    initializeReadWriteLoop();
}

Pty::~Pty()
{
    sem_wait(&loopSemaphore_);
    pthread_join(readWriteThread, NULL);
    pthread_mutex_destroy(&writeMutex_);
}

static void* runReadWriteLoopThread(void* pty)
{
    static_cast<Pty*>(pty)->readWriteLoop();
    return NULL;
}

void Pty::initializeReadWriteLoop()
{
    pthread_create(&readWriteThread, NULL, runReadWriteLoopThread, this);
}

void Pty::closeMasterFd() {
    close(masterfd_);
    masterfd_ = -1;
}

PtyInitResult Pty::init(const std::string& pathToExecutable) {
    // todo: check?
    pthread_mutex_init(&writeMutex_, NULL);
    sem_init(&loopSemaphore_, 0, 1);

    readBuffer_ = (char*)malloc(PTY_READ_BUFFER_SIZE);

    writeBuffer_ = (char*)malloc(PTY_BUFFER_SIZE);
    writeEnd_ = writeBuffer_;

    // TODO: add some code to not fork is pathToExecutable is invalid.
    int interactive = isatty(STDIN_FILENO);
    char* ptsNameStr;
    pid_t pid;

    struct termios orig_termios;

    if (interactive) {
        if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
            return ERROR_TCGETATTR;
        }
    }

    if((masterfd_ = posix_openpt(O_RDWR)) < 0) {
        return ERROR_OPENPT;
    }

    // grant access to the slave pseudo-terminal
    if(grantpt(masterfd_) < 0) {
        this->closeMasterFd();
        return ERROR_GRANTPT;
    }

    // unlock a pseudo-terminal master/slave pair
    if(unlockpt(masterfd_) < 0) {
        this->closeMasterFd();
        return ERROR_UNLOCKPT;
    }

    // user the reentrant verstion of ptsname...
    // ptsname isn't reentrant
    if((ptsNameStr = ptsname(masterfd_)) == NULL) {
        this->closeMasterFd();
        return ERROR_PTSNAME;
    }

    // set masterfd to nonblocking i/o
    int statusFlagsResult = fcntl(masterfd_, F_GETFL);
    if ((statusFlagsResult != -1) && !(statusFlagsResult & O_NONBLOCK)) {
        if(fcntl(masterfd_, F_SETFL, statusFlagsResult | O_NONBLOCK) < 0) {
            this->closeMasterFd();
            return ERROR_SET_NONBLOCK;
        }
    }

    if((pid = fork()) < 0) {
        this->closeMasterFd();
        return ERROR_FORK;
    }
    else if(pid == 0) { // child
        // creates a session and sets the process group ID
        if(setsid() < 0) {
            // TODO: better child error handling
            printf("ERROR: setsid\n");
            this->closeMasterFd();
            exit(EXIT_FAILURE);
        }

        int slavefd;

        // open slavefd
        if((slavefd = open(ptsNameStr, O_RDWR)) < 0) {
            // TODO: better child error handling
            printf("ERROR: setsid\n");
            this->closeMasterFd();
            exit(EXIT_FAILURE);
        }

        this->closeMasterFd();

        if (interactive) {
            if (tcsetattr(slavefd, TCSANOW, &orig_termios) < 0) {
                // TODO: better child error handling
                printf("tcsetattr error on slave pty\n");
                exit(EXIT_FAILURE);
            }
        }

        // Slave becomes stdin/stdout/stderr of child.
        if(dup2(slavefd, STDIN_FILENO) != STDIN_FILENO) {
            printf("ERROR: dup2 stdin\n");
            exit(EXIT_FAILURE);
        }

        if(dup2(slavefd, STDOUT_FILENO) != STDOUT_FILENO) {
            printf("ERROR: dup2 stdout\n");
            exit(EXIT_FAILURE);
        }

        std::cerr << "3 " << slavefd << ":" << STDERR_FILENO << std::endl;
        if(dup2(slavefd, STDERR_FILENO) != STDERR_FILENO) {
            printf("ERROR: dup2 stderr\n");
            exit(EXIT_FAILURE);
        }

        if (slavefd != STDIN_FILENO && slavefd != STDOUT_FILENO &&
            slavefd != STDERR_FILENO) {
            close(slavefd);
        }

        execlp(pathToExecutable.c_str(), pathToExecutable.c_str(), NULL);
        // TODO: better child error handling
        printf("ERROR: exec issues\n");
        exit(EXIT_FAILURE);
    }

    return SUCCESS;
}

int Pty::ptyWrite(const char* buffer, const int count) {
    if (count <= 0) {
        return 0;
    }

    pthread_mutex_lock(&writeMutex_);

    ssize_t writeCount = 0;
    // if the buffer is empty, try to write

    if (writeBuffer_ == writeEnd_) {
        writeCount = write(masterfd_, buffer, count);
        if(count == writeCount) {
            pthread_mutex_unlock(&writeMutex_);
            return count;
        }
    }

    // add to buffer anything that couldn't be written to the fd
    int bufferSpace = PTY_BUFFER_SIZE - (writeEnd_ - writeBuffer_);
    int toCopyAmount = count - writeCount;
    if(toCopyAmount > bufferSpace) {
        // @TODO log error out of buffer space
        toCopyAmount = bufferSpace;
    }

    memcpy(writeEnd_, buffer + writeCount, toCopyAmount);
    writeEnd_ += toCopyAmount;

    pthread_mutex_unlock(&writeMutex_);
    return toCopyAmount + writeCount;
}

int Pty::putChar(const char character) {
    return this->ptyWrite(&character, 1);
}

void Pty::readWriteLoop() {
    int amount, semval;

    while(true) {
        sem_getvalue(&loopSemaphore_, &semval);
        if(semval <= 0) return;

        amount = read(masterfd_, readBuffer_, PTY_READ_BUFFER_SIZE);

        if(amount > 0) {
            this->emulator->parseBuffer(readBuffer_,
                readBuffer_ + amount);
        }

        pthread_mutex_lock(&writeMutex_);

        // if the buffer is empty, try to write
        if (writeBuffer_ != writeEnd_) {
            ssize_t totalWriteAvailable = writeEnd_ - writeBuffer_;
            ssize_t writeCount =
                write(masterfd_, writeBuffer_, totalWriteAvailable);
            // didn't write it all, memmove to beginning
            if(totalWriteAvailable != writeCount) {
                memmove(writeBuffer_, writeBuffer_ + writeCount,
                    totalWriteAvailable - writeCount);
                // subtract the amount written from write end
                writeEnd_ -= writeCount;
            }
            else {
                // reset the write end;
                writeEnd_ = writeBuffer_;
            }
        }

        pthread_mutex_unlock(&writeMutex_);

        pthread_yield();
    }
}

void Pty::setSize(int columns, int rows)
{
    printf("set size: %i %i\n", columns, rows);
    struct winsize size = { rows, columns, 0, 0};
    if (ioctl(masterfd_, TIOCSWINSZ, &size) < 0)
        printf("TIOCSWINSZ error on master pty\n");
}
