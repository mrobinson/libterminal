#include "SimpleVT100Client.h"

#include <cstdio>

void SimpleVT100Client::appendCharacter(char character)
{
    printf("%c", character);
}

void SimpleVT100Client::changeColor(int, int)
{
}

void SimpleVT100Client::eraseFromCursorToEndOfLine(Direction direction)
{
}
