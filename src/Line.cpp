#include "Line.h"
#include <cstdio>

static const size_t g_textNodeBufferLength = 80;

Line::Line()
{
    _chars.reserve(80);
}

void Line::appendCharacter(char newCharacter)
{
    _chars.push_back(newCharacter);
}

void Line::eraseFromPositionToEndOfLine(size_t position, Direction direction)
{
    if (_chars.size() == 0)
        return;

    // ASSERT(direction == Forward || direction == Backward);
    if (direction == Right) {
        _chars.erase(_chars.begin() + position, _chars.end());
    } else {
        _chars.erase(_chars.begin(), _chars.begin() + position);
    }
}
