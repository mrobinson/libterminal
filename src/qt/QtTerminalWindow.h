#include "VT100Client.h"
#include <QTextEdit>

class Pty;

class QtTerminalWindow : public QTextEdit, public VT100Client {
    Q_OBJECT

public:
    QtTerminalWindow();
    void setPty(Pty* pty) { m_pty = pty; }
    virtual void appendCharacter(char character);
    virtual void changeColor(int color1, int color2);

protected:
    virtual void keyPressEvent(QKeyEvent* event);

private:
    Pty* m_pty;
    char m_previousCharacter;

signals:
    void appendCharacterSignal(char character);

public slots:
    void handleAppendCharacter(char character);

};
