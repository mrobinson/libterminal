#ifndef LineOrientedVT100Client_h
#define LineOrientedVT100Client_h

#include "TerminalContentNode.h"
#include "VT100Client.h"
#include <vector>
#include <cstring>

class LineOrientedVT100Client : public VT100Client {
public:
    LineOrientedVT100Client();
    virtual ~LineOrientedVT100Client();

    virtual void appendCharacter(char character);
    virtual void changeColor(int color1, int color2);
    virtual void eraseFromCursorToEndOfLine(Direction direction);

protected:
    virtual void characterAppended() = 0;
    virtual void somethingLargeChanged() = 0;

    std::vector<TerminalContentNode*>* lineAt(size_t line);
    size_t numberOfLines();

private:
    void appendNewLine();
    void moveCursor(Direction direction, Granularity granularity);

    std::vector<std::vector<TerminalContentNode*>* > m_lines;
    std::vector<TerminalContentNode*>* m_currentLine;
    char m_previousCharacter;
    size_t m_cursorColumn;
};

#endif // LineOrientedVT100Client_h
