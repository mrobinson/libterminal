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
    : masterfd(-1)
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
    sem_wait(&this->loopMutex);
    pthread_join(readWriteThread, NULL);
    pthread_mutex_destroy(&this->writeStartMutex);
    pthread_mutex_destroy(&this->writeEndMutex);
    pthread_mutex_destroy(&this->writeHeadMutex);
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
    close(this->masterfd);
    this->masterfd = -1;
}

PtyInitResult Pty::init(const std::string& pathToExecutable) {
    // todo: check?
    pthread_mutex_init(&this->writeStartMutex, NULL);
    pthread_mutex_init(&this->writeEndMutex, NULL);
    sem_init(&this->loopMutex, 0, 1);

    this->readBuffer = (char*)malloc(PTY_READ_BUFFER_SIZE);
    this->writeBuffer = (char*)malloc(PTY_BUFFER_SIZE);
    this->readStart = this->readBuffer;
    this->readEnd = this->readBuffer;
    this->writeStart = this->writeBuffer;
    this->writeHead = this->writeBuffer;
    this->writeEnd = this->writeBuffer;

    // TODO: add some code to not fork is pathToExecutable is invalid.
    int interactive = isatty(STDIN_FILENO);
    char* ptsNameStr;
    pid_t pid;

    struct termios orig_termios;
    struct winsize size;

    if (interactive) {
        if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
            return ERROR_TCGETATTR;
        }
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size) < 0) {
            return ERROR_IOCTL_TIOCGWINSZ;
        }
    }

    if((this->masterfd = posix_openpt(O_RDWR)) < 0) {
        return ERROR_OPENPT;
    }

    // grant access to the slave pseudo-terminal
    if(grantpt(this->masterfd) < 0) {
        this->closeMasterFd();
        return ERROR_GRANTPT;
    }

    // unlock a pseudo-terminal master/slave pair
    if(unlockpt(this->masterfd) < 0) {
        this->closeMasterFd();
        return ERROR_UNLOCKPT;
    }

    // user the reentrant verstion of ptsname...
    // ptsname isn't reentrant
    if((ptsNameStr = ptsname(masterfd)) == NULL) {
        this->closeMasterFd();
        return ERROR_PTSNAME;
    }

    // set masterfd to nonblocking i/o
    int statusFlagsResult = fcntl(masterfd, F_GETFL);
    if ((statusFlagsResult != -1) && !(statusFlagsResult & O_NONBLOCK)) {
        if(fcntl(this->masterfd, F_SETFL, statusFlagsResult | O_NONBLOCK) < 0) {
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
            if (ioctl(slavefd, TIOCSWINSZ, &size) < 0) {
                // TODO: better child error handling
                printf("TIOCSWINSZ error on slave pty\n");
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
    char *localWriteStart, *localWriteEnd, *localWriteHead;
    int space;

    if(count == 0) {
        return 0;
    }

    localWriteEnd = this->getWriteEnd();
    localWriteHead = this->getWriteHead();
    
    if(localWriteHead >= (this->writeBuffer + PTY_BUFFER_SIZE)) {
        localWriteHead = this->writeBuffer;
    }

    if(localWriteHead >= localWriteEnd) {
        space = (this->writeBuffer + PTY_BUFFER_SIZE) - localWriteHead;
        space = space >= count? count : space;
        memcpy(localWriteHead, buffer, space);
        localWriteHead += space;
        this->setWriteEnd(localWriteHead);
        this->setWriteHead(localWriteHead);
        return space + this->ptyWrite(buffer + space, count - space);
    }
    else {
        localWriteStart = this->getWriteStart();
        if(localWriteStart < localWriteHead) {
            localWriteStart = this->writeBuffer + PTY_BUFFER_SIZE;
        }
        space = localWriteStart - localWriteHead;
        space = space >= count? count : space;
        memcpy(localWriteHead, buffer, space);
        localWriteHead += space;
        this->setWriteHead(localWriteHead);
        return space;
    }     

    // full buffer case.
}

int Pty::putChar(const char character) {
    return this->ptyWrite(&character, 1);
}

int Pty::getBufferFreeSpace(const char* buffer, const char* start,
    const char* end) {
    if(start < end)
        return end - start - 1;
    else
        return end - start - 1 + PTY_BUFFER_SIZE;
}

int Pty::getBufferUsedSpace(const char* buffer, const char* start,
    const char* end) {
    return PTY_BUFFER_SIZE - this->getBufferFreeSpace(buffer, start, end) - 2;
}

void Pty::readWriteLoop() {
    int amount, semval;
    char *readCheckpoint, *localWriteStart, *localWriteEnd;

    while(true) {
        sem_getvalue(&this->loopMutex, &semval);
        if(semval <= 0) return;

        amount = read(this->masterfd, this->readEnd,
            PTY_READ_BUFFER_SIZE - (this->readEnd - this->readBuffer));
        
        if(amount > 0) {
            this->readEnd = this->readEnd + amount;
        }

        if(this->readEnd != this->readStart) {
            readCheckpoint = const_cast<char*>(this->emulator->parseBuffer(this->readStart, this->readEnd));
            if(readCheckpoint == this->readEnd) {
                this->readEnd = this->readBuffer;
                this->readStart = this->readBuffer;
            }
            else {
                this->readStart = readCheckpoint;
                if((this->readStart != this->readBuffer) &&
                    (this->readStart - this->readBuffer > PTY_READ_BUFFER_SIZE - 200)) {
                    memcpy(this->readBuffer, this->readStart, this->readEnd - this->readStart);
                    this->readEnd = this->readBuffer + 
                        (this->readEnd - this->readStart);
                    this->readStart = this->readBuffer;
                }
            }
        }

        localWriteStart = this->getWriteStart();
        localWriteEnd = this->getWriteEnd();
        //char* wb;

        if(localWriteStart != localWriteEnd) {
            amount = write(this->masterfd, localWriteStart, 
                localWriteEnd - localWriteStart);
            if(amount > 0) {
                localWriteStart = this->writeStart + amount;
                if(localWriteStart >= (this->writeBuffer + PTY_BUFFER_SIZE)) {
                    // writeStart can never be set as greater than
                    this->resetWritePointers();
                    continue;
                }
                else {
                    this->setWriteStart(localWriteStart);
                }
            }
        }

        pthread_yield();
    }
    
}

char* Pty::getWriteStart() {
    char* value;
    pthread_mutex_lock(&this->writeStartMutex);
    value = this->writeStart;
    pthread_mutex_unlock(&this->writeStartMutex);
    return value;
}

char* Pty::getWriteHead() {
    char* value;
    pthread_mutex_lock(&this->writeHeadMutex);
    value = this->writeHead;
    pthread_mutex_unlock(&this->writeHeadMutex);
    return value;
}

char* Pty::getWriteEnd() {
    char* value;
    pthread_mutex_lock(&this->writeEndMutex);
    value = this->writeEnd;
    pthread_mutex_unlock(&this->writeEndMutex);
    return value;
}

void Pty::setWriteStart(char* value) {
    pthread_mutex_lock(&this->writeStartMutex);
    this->writeStart = value;
    pthread_mutex_unlock(&this->writeStartMutex);
}

void Pty::setWriteEnd(char* value) {
    pthread_mutex_lock(&this->writeEndMutex);
    this->writeEnd = value;
    pthread_mutex_unlock(&this->writeEndMutex);
}

void Pty::setWriteHead(char* value) {
    pthread_mutex_lock(&this->writeHeadMutex);
    this->writeHead = value;
    pthread_mutex_unlock(&this->writeHeadMutex);
}

void Pty::resetWritePointers() {
    char* localWriteHead = this->getWriteHead();
    pthread_mutex_lock(&this->writeEndMutex);
    pthread_mutex_lock(&this->writeStartMutex);
    this->writeStart = this->writeBuffer;
    this->writeEnd = localWriteHead;
    pthread_mutex_unlock(&this->writeStartMutex);
    pthread_mutex_unlock(&this->writeEndMutex);
}

