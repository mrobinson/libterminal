#ifndef LineOrientedVT100Client_h
#define LineOrientedVT100Client_h

#include "VT100Client.h"
#include <vector>
#include <cstring>

class Line;

class LineOrientedVT100Client : public VT100Client {
public:
    LineOrientedVT100Client();
    virtual ~LineOrientedVT100Client();

    virtual void appendCharacter(char character);
    virtual void changeColor(int color1, int color2);
    virtual void eraseFromCursorToEndOfLine(Direction direction);

    int calculateHowManyLinesFitWithWrapping(int linesToDraw);

protected:
    virtual void characterAppended() = 0;
    virtual void somethingLargeChanged() = 0;
    virtual int charactersWide() = 0;
    virtual int charactersTall() = 0;
    virtual void renderTextAt(const char* text, size_t numberOfCharacters, bool isCursor, int x, int y) = 0;

    Line* lineAt(size_t line);
    size_t numberOfLines();
    int m_cursorColumn;
    int m_cursorRow;

    void renderLine(int lineNumber, int& currentBaseline);
    void paint();

private:
    void appendNewLine();
    void moveCursor(Direction direction, Granularity granularity);

    std::vector<Line*> m_lines;
    char m_previousCharacter;
};

#endif // LineOrientedVT100Client_h
