#include "LineOrientedVT100Client.h"
#include <QFrame>
#include <QPainter>

class Pty;

class QtTerminalWindow : public QFrame, public LineOrientedVT100Client {
    Q_OBJECT

public:
    QtTerminalWindow();
    void setPty(Pty* pty);

protected:
    void paintEvent(QPaintEvent*);
    void setFont(QFont*);

    // LineOrientedVT100Client implementation
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* resizeEvent);

    virtual void characterAppended();
    virtual void somethingLargeChanged();
    virtual void bell();

    virtual int charactersWide();
    virtual int charactersTall();
    virtual void renderTextAt(const char* text, size_t numberOfCharacters, bool isCursor, int x, int y);

private:
    Pty* m_pty;
    char m_previousCharacter;
    QFontMetrics* m_fontMetrics;
    QFont* m_font;
    QSize m_size;
    QPainter* m_currentPainter;


signals:
    void updateNeeded();

public slots:
    void handleUpdateNeeded();

};
