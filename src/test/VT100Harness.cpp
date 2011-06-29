#include <cstdio>
#include <cstdlib>
#include "SimpleVT100Client.h"
#include "VT100.h"

int main(int argc, const char** argv)
{
    VT100 vt100(new SimpleVT100Client());
    char readBuffer[1024];
    size_t bytesRead = 0;
    while ((bytesRead = fread(readBuffer, 1, 1024, stdin))) {
        vt100.parseBuffer(readBuffer, readBuffer + bytesRead);
    }
    return 0;
}
