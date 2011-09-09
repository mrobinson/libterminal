#include "LineOrientedVT100Client.h"

#include <cstdio>

LineOrientedVT100Client::LineOrientedVT100Client()
    : m_previousCharacter('\0')
    , m_cursorColumn(1)
{
    m_lines.push_back(new std::vector<TerminalContentNode*>());
    m_lines.back()->push_back(new TerminalContentNode(TerminalContentNode::Text));
}

LineOrientedVT100Client::~LineOrientedVT100Client()
{
    for (size_t lineNumber = 0; lineNumber < m_lines.size(); lineNumber++) {
        const std::vector<TerminalContentNode*>* line = m_lines[lineNumber];
        for (size_t node = 0; node < line->size(); node++) {
            delete line->at(node);
        }
        delete line;
    }
}

void LineOrientedVT100Client::appendNewLine()
{
    m_lines.push_back(new std::vector<TerminalContentNode*>());
    m_lines.back()->push_back(new TerminalContentNode(TerminalContentNode::Text));
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
        printf("backspace\n");
        moveCursor(Left, Character);
    } else if (character != '\r') {
        moveCursor(Right, Character);
        if (!m_lines.back()->back()->appendCharacter(character)) {
            m_lines.back()->push_back(new TerminalContentNode(TerminalContentNode::Text));
            m_lines.back()->back()->appendCharacter(character);
        }
        characterAppended();
    }
    m_previousCharacter = character;
}

void LineOrientedVT100Client::changeColor(int color1, int color2)
{
    // TODO: Implement.
}

std::vector<TerminalContentNode*>* LineOrientedVT100Client::lineAt(size_t line)
{
    return m_lines[line];
}

size_t LineOrientedVT100Client::numberOfLines()
{
    return m_lines.size();
}

void LineOrientedVT100Client::eraseFromCursorToEndOfLine(Direction direction)
{
    // ASSERT(direction == Forward || direction == Backward);
    m_lines.back()->back()->eraseFromPositionToEndOfLine(m_cursorColumn, direction);
    somethingLargeChanged();
}
