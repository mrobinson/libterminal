#include "LineOrientedVT100Client.h"

#include <cstdio>

LineOrientedVT100Client::LineOrientedVT100Client()
    : m_previousCharacter('\0')
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
}

void LineOrientedVT100Client::appendCharacter(char character)
{
    if (character == '\n' && m_previousCharacter == '\r') {
        appendNewLine();
        somethingLargeChanged();
    } else if (character != '\r') {
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
