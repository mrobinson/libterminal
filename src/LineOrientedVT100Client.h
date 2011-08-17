#include "TerminalContentNode.h"
#include "VT100Client.h"
#include <vector>

class LineOrientedVT100Client : public VT100Client {
public:
    LineOrientedVT100Client();
    virtual ~LineOrientedVT100Client();

    virtual void appendCharacter(char character);
    virtual void changeColor(int color1, int color2);

protected:
    virtual void characterAppended() = 0;
    virtual void somethingLargeChanged() = 0;

    std::vector<TerminalContentNode*>* lineAt(size_t line);
    size_t numberOfLines();

private:
    void appendNewLine();

    std::vector<std::vector<TerminalContentNode*>* > m_lines;
    std::vector<TerminalContentNode*>* m_currentLine;
    char m_previousCharacter;
};
