class VT100Client {
public:
    virtual void appendCharacter(char character) = 0;
    virtual void changeColor(int color1, int color2) = 0;
};
