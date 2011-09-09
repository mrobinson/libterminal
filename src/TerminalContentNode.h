#include "VT100Client.h"
#include <vector>
#include <cstring>

class TerminalContentNode {
public:
    enum NodeType{
        Text,
    };

    TerminalContentNode(NodeType);
    NodeType type() { return m_type; }
    char* text() { return m_text; }
    size_t textLength() { return m_textLength; }
    size_t bufferLength() { return m_bufferLength; }

    bool appendCharacter(char);
    void eraseFromPositionToEndOfLine(size_t position, Direction direction);

private:
    void eraseCharacter();

    NodeType m_type;
    char* m_text;
    size_t m_bufferLength;
    size_t m_textLength;
};
