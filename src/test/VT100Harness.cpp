#include <cstdio>
#include <cstdlib>
#include "../VT100.h"

int main(int argc, const char** argv)
{
    char readBuffer[1024];
    size_t bytesRead = 0;
    while (bytesRead = fread(readBuffer, 1, 1024, stdin)) {
        parseBuffer(readBuffer, readBuffer + bytesRead);
    }
    return 0;
}
