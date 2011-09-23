#include "TerminalContentNode.h"
#include <cstdio>

static const size_t g_textNodeBufferLength = 80;

TerminalContentNode::TerminalContentNode(NodeType type)
    : m_type(type)
    , m_text(0)
    , m_bufferLength(0)
    , m_textLength(0)
{
    if (type == Text) {
        m_text = new char[g_textNodeBufferLength];
        m_bufferLength = g_textNodeBufferLength;
    }
}

bool TerminalContentNode::appendCharacter(char newCharacter)
{
    //ASSERT(m_type == Text);
    if (m_bufferLength - 1 == m_textLength)
        return false;

    m_text[m_textLength++] = newCharacter;
    return true;
}

void TerminalContentNode::eraseFromPositionToEndOfLine(size_t position, Direction direction)
{
    if (m_textLength <= 0)
        return;

    // ASSERT(direction == Forward || direction == Backward);
    if (direction == Right) {
        m_textLength = position - 1;
        printf("new text length: %li\n", m_textLength);
        return;
    }

    m_textLength = m_textLength - position - 1;
    memmove(m_text, m_text + position + 1, m_textLength);
}
