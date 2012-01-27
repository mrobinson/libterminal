#include "LineOrientedVT100Client.h"
#include "Line.h"

#include <cstdio>

LineOrientedVT100Client::LineOrientedVT100Client()
    : m_cursorColumn(1)
    , m_previousCharacter('\0')
{
    appendNewLine(); // Ye olde first line.
}

LineOrientedVT100Client::~LineOrientedVT100Client()
{
    for (size_t lineNumber = 0; lineNumber < m_lines.size(); lineNumber++)
        delete m_lines[lineNumber];
}

void LineOrientedVT100Client::appendNewLine()
{
    m_lines.push_back(new Line());
    m_cursorColumn = 1;
}

void LineOrientedVT100Client::moveCursor(Direction direction, Granularity granularity)
{
    if (granularity == Character)
        m_cursorColumn += direction;
}

void LineOrientedVT100Client::appendCharacter(char character)
{
    if (character == '\n' && m_previousCharacter == '\r') {
        appendNewLine();
        somethingLargeChanged();
    } else if (character == '\b') {
        moveCursor(Left, Character);
    } else if (character != '\r') {
        moveCursor(Right, Character);
        m_lines.back()->appendCharacter(character);
        characterAppended();
    }
    m_previousCharacter = character;
}

void LineOrientedVT100Client::changeColor(int color1, int color2)
{
    // TODO: Implement.
}

Line* LineOrientedVT100Client::lineAt(size_t line)
{
    return m_lines[line];
}

size_t LineOrientedVT100Client::numberOfLines()
{
    return m_lines.size();
}

void LineOrientedVT100Client::eraseFromCursorToEndOfLine(Direction direction)
{
    //ASSERT(direction == Left || direction == Right || direction == LeftAndRight);
    if (direction == LeftAndRight) {
        m_lines.back()->eraseFromPositionToEndOfLine(m_cursorColumn, Left);
        m_lines.back()->eraseFromPositionToEndOfLine(m_cursorColumn, Right);
        return;
    }
    m_lines.back()->eraseFromPositionToEndOfLine(m_cursorColumn, direction);
    somethingLargeChanged();
}
