#include "SimpleVT100Client.h"

#include <cstdio>

void SimpleVT100Client::appendCharacter(char character)
{
    printf("append char: %c\n", character);
}

void SimpleVT100Client::changeColor(int, int)
{
}
