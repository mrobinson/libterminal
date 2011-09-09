#include "VT100Client.h"

class SimpleVT100Client : public VT100Client {
public:
    virtual void appendCharacter(char character);
    virtual void changeColor(int color1, int color2);
    virtual void eraseFromCursorToEndOfLine(Direction direction);
};
