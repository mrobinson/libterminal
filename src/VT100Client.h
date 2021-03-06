#ifndef VT100Client_h
#define VT100Client_h

enum Direction { Left = -1, Right = 1, LeftAndRight = 2, Up = 3, Down = 4};
enum Granularity { Character };

class VT100Client {
public:
    virtual void appendCharacter(char character) = 0;
    virtual void changeColor(int color1, int color2) = 0;
    virtual void eraseFromCursorToEndOfLine(Direction direction) = 0;
    virtual void bell() = 0;
};

#endif // VT100Client_h
