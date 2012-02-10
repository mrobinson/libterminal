#include "LineOrientedVT100Client.h"
#include "Line.h"

#include <cstdio>
#include <math.h>

LineOrientedVT100Client::LineOrientedVT100Client()
    : m_cursorColumn(-1)
    , m_cursorRow(-1)
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
    m_cursorColumn = 0;
    m_cursorRow = numberOfLines() - 1;
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

int LineOrientedVT100Client::calculateHowManyLinesFitWithWrapping(int linesToDraw)
{
    int linesThatFit = 0;
    int currentLine = numberOfLines() - 1;
    int consumedHeight = 0;

    while (consumedHeight < charactersTall() && linesThatFit < linesToDraw) {
        linesThatFit++;
        consumedHeight += ceilf(static_cast<float>(lineAt(currentLine)->numberOfCharacters()) /
                                static_cast<float>(charactersWide()));
        currentLine--;
    }
    return linesThatFit;
}

void LineOrientedVT100Client::renderLine(int lineNumber, int& currentBaseline)
{
    // TODO: We are assuming ASCII here. In the future we need to use a library
    // like ICU to split the line based on characters instead of bytes.
    Line* line = lineAt(lineNumber);
    const char* text = line->chars();
    int lineLength = strlen(text);
    int cursorColumn = m_cursorColumn;

    while (lineLength > 0) {
        int charactersToPaint = std::min(lineLength, charactersWide());
        renderTextAt(text, charactersToPaint, false, 0, currentBaseline);

        if (lineNumber == m_cursorRow) {
            if (cursorColumn < charactersToPaint || charactersToPaint < charactersWide()) {
                renderTextAt("a", 1, true, cursorColumn, currentBaseline);
            } else
                cursorColumn -= charactersToPaint;
        }

        lineLength -= charactersToPaint;
        text += charactersToPaint;
        currentBaseline++;
    }
}

void LineOrientedVT100Client::paint()
{
    int maximumNumberOfLinesToDraw = std::min(numberOfLines(), static_cast<size_t>(charactersWide()));
    int linesToDraw = calculateHowManyLinesFitWithWrapping(maximumNumberOfLinesToDraw);

    int currentBaseline = 0;
    for (size_t i = linesToDraw; i > 0; i--) {
        renderLine(linesToDraw - i, currentBaseline);
    }
}
