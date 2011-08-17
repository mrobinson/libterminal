#include "TerminalContentNode.h"

#include <cstdio>
#include <string>

static const size_t g_textNodeBufferLength = 8000;

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

void TerminalContentNode::appendCharacter(char newCharacter)
{
    //ASSERT(m_type == Text);
    printf("appending %x '%c'\n", newCharacter, newCharacter);
    m_text[m_textLength++] = newCharacter;
}
