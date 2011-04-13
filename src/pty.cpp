#include "pty.h"

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
#include <string.h>

namespace Pecera
{

Pty::Pty()
{
}

Pty::~Pty() 
{
    this->masterfd = -1;
    this->readStart = readBuffer;
    this->readEnd = readBuffer + 1;
    this->writeStart = writeBuffer + 1;
    this->writeEnd = writeBuffer;
}

void Pty::closeMasterFd() {
    close(this->masterfd);
    this->masterfd = -1;
}

PtyInitResult Pty::init() {
    std::string* pathToExecute = Pty::getUsersShell();
    if(pathToExecute == NULL) {
	return ERROR_GET_USERS_SHELL;
    }
    PtyInitResult result = init(*pathToExecute);
    free(pathToExecute);
    return result;
}

PtyInitResult Pty::init(const std::string& pathToExecutable) {
    // todo: check?
    pthread_mutex_init(&this->readStartMutex, NULL);
    pthread_mutex_init(&this->readEndMutex, NULL);
    pthread_mutex_init(&this->writeStartMutex, NULL);
    pthread_mutex_init(&this->writeEndMutex, NULL);

    this->readBuffer = (char*)malloc(PTY_BUFFER_SIZE);
    this->writeBuffer = (char*)malloc(PTY_BUFFER_SIZE);

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
	if(dup2(slavefd, STDERR_FILENO) != STDERR_FILENO) {
	    printf("ERROR: dup2 stderr\n");
	    exit(EXIT_FAILURE);	    
	}

	execlp(pathToExecutable.c_str(), pathToExecutable.c_str(), NULL);
	// TODO: better child error handling
	printf("ERROR: exec issues\n");
	exit(EXIT_FAILURE);	    
    }

    return SUCCESS;
}

std::string* Pty::getUsersShell() {
    // getpwuid isn't reentrant
    struct passwd *pass;
    pass = getpwuid(getuid());
    if((pass != NULL) && (pass->pw_shell != NULL)) {
	return new std::string(pass->pw_shell);
    }

    char* shellEnv = getenv("SHELL");
    if(shellEnv != NULL) {
	return new std::string(shellEnv);
    }

    return NULL;
}

int Pty::ptyWrite(const char* buffer, const int count) {
    // rules:
    // when s + 1 = e you can no longer write more data into the buffer
    // when e + 1 = s you can no longer read data out of the buffer
    unsigned int tryCount = 0;
    int endAmount = 0;
    char* wE = this->getWriteEnd();

    while(count > this->getBufferFreeSpace(this->writeBuffer, 
	    this->writeStart, wE)) {
	// todo: investigate a better way here...
	usleep(100);
	if(++tryCount > 10) return -1;
	wE = this->getWriteEnd();
    }

    endAmount = (this->writeBuffer + PTY_BUFFER_SIZE) - this->writeStart;
    if((this->writeStart < this->writeEnd) || (count <= endAmount)) {
	// there is enough space without wrapping around in the rotating buffer
	memcpy(this->writeStart, buffer, count);
	if(this->writeStart == (this->writeBuffer + PTY_BUFFER_SIZE))
	    this->setWriteStart(this->writeBuffer);
	else
	    this->setWriteStart(this->writeStart + count);
    }
    else {
	memcpy(this->writeStart, buffer, endAmount);
	memcpy(this->writeBuffer, buffer + endAmount, count - endAmount);
	this->setWriteStart(writeBuffer + count - endAmount);
    }
    return count;
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

int Pty::ptyRead(char* buffer, const int maxCount) {
    // read start can be modified by another thread so it needs to be read
    // via through a mutex lock to ensure atomic operations.
    // read end can only be modified by this method so only the set operation
    // for read end is mutex protected
    char* rS = this->getReadStart();    
    int availableToRead = this->getBufferUsedSpace(this->readBuffer, 
	rS, this->readEnd);
    int amountToRead = maxCount > availableToRead ? availableToRead : maxCount;
    int endAmount = 0;

    if(availableToRead == 0) 
	return 0;
    
    endAmount = (this->readBuffer + PTY_BUFFER_SIZE) - this->readEnd;
    if((this->readEnd > rS) || (amountToRead <= endAmount)) {
	// there is enough space without wrapping around in the rotating buffer
	memcpy(buffer, this->readEnd, amountToRead);

	if((this->readEnd + amountToRead) == 
	    (this->readBuffer + PTY_BUFFER_SIZE))
	    this->setReadEnd(this->readBuffer);
	else
	    this->setReadEnd(this->readEnd + amountToRead);
    }
    else {
	memcpy(buffer, this->readEnd, endAmount);
	memcpy(buffer, this->readBuffer, amountToRead - endAmount);
	this->setReadEnd(this->readBuffer + amountToRead - endAmount);
    }
    return amountToRead;
}

void Pty::readWriteLoop() {
    int space, amount;
    char *rE, *wS;

    this->runLoop = 0;

    while(this->runLoop == 0) {
	// read... space rS to rE
	rE = this->getReadEnd();
	space = this->getBufferFreeSpace(this->readBuffer, 
	    this->readStart, rE);
	if(space > 0) {
	    if(this->readStart > rE) {
		amount = read(this->masterfd, this->readStart, 
		    (this->readBuffer + PTY_BUFFER_SIZE - this->readStart));

		if((amount + this->readStart) == 
		    (this->readBuffer + PTY_BUFFER_SIZE)) {
		    this->setReadStart(this->readBuffer);
		    space = this->getBufferFreeSpace(this->readBuffer, 
			this->readBuffer, rE);
		    if(space > 0) {
			amount = read(this->masterfd, this->readBuffer, 
			    space);
			this->setReadStart(this->readBuffer + amount);
		    }
		}
	    }
	    // just a straight read.
	    else {
		amount = read(this->masterfd, this->readStart, space);
		this->setReadStart(this->readStart + amount);
	    }
	}

	// write... space wE to wS
	wS = this->getWriteStart();
	space = this->getBufferUsedSpace(this->writeBuffer, wS, this->writeEnd);
	if(space > 0) {
	    if(this->writeEnd > wS) {
		amount = write(this->masterfd, this->writeEnd, 
		    (this->writeBuffer + PTY_BUFFER_SIZE) - this->writeEnd);

		if((amount + this->writeEnd) == 
		    (this->writeBuffer + PTY_BUFFER_SIZE)) {
		    this->setWriteEnd(this->writeBuffer);
		    space = this->getBufferFreeSpace(this->writeBuffer, 
			wS, this->writeBuffer);
		    if(space > 0) {
			amount = write(this->masterfd, this->writeBuffer, 
			    space);
			this->setWriteEnd(this->writeBuffer + amount);
		    }
		}
	    }
	    // just a straight write
	    else {
		amount = write(this->masterfd, this->writeEnd, space);
		this->setWriteEnd(this->writeEnd + amount);
	    }
	}
    }
}

char* Pty::getReadStart() {
    char* value;
    pthread_mutex_lock(&this->readStartMutex);
    value = this->readStart;
    pthread_mutex_unlock(&this->readStartMutex);
    return value;
}

char* Pty::getReadEnd() {
    char* value;
    pthread_mutex_lock(&this->readEndMutex);
    value = this->readEnd;
    pthread_mutex_unlock(&this->readEndMutex);
    return value;
}

char* Pty::getWriteStart() {
    char* value;
    pthread_mutex_lock(&this->writeStartMutex);
    value = this->writeStart;
    pthread_mutex_unlock(&this->writeStartMutex);
    return value;
}

char* Pty::getWriteEnd() {
    char* value;
    pthread_mutex_lock(&this->writeEndMutex);
    value = this->writeEnd;
    pthread_mutex_unlock(&this->writeEndMutex);
    return value;
}

void Pty::setReadStart(char* value) {
    pthread_mutex_lock(&this->readStartMutex);
    this->readStart = value;
    pthread_mutex_unlock(&this->readStartMutex);    
}

void Pty::setReadEnd(char* value) {
    pthread_mutex_lock(&this->readEndMutex);
    this->readEnd = value;
    pthread_mutex_unlock(&this->readEndMutex);    
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

};
