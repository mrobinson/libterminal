#include "VT100Client.h"
#include <QTextEdit>

class QtTerminalWindow : public QTextEdit, public VT100Client {
    Q_OBJECT

public:
    QtTerminalWindow();
    virtual void appendCharacter(char character);
    virtual void changeColor(int color1, int color2);

signals:
    void appendCharacterSignal(char character);

public slots:
    void handleAppendCharacter(char character);

};
