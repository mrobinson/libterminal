#include "VT100Client.h"
#include <vector>
#include <cstring>
#include <string>

class Line {
public:
    Line();

    void appendCharacter(char);
    void eraseFromPositionToEndOfLine(size_t position, Direction direction);

    char characterAt(size_t i) { return _chars[i]; }
    char numberOfCharacters(size_t i) { return _chars.size(); }
    const char* chars() { return _chars.c_str(); }

private:
    std::string _chars;
};
