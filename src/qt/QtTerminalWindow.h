#include "LineOrientedVT100Client.h"
#include <QFrame>
#include <QPainter>

class Pty;

class QtTerminalWindow : public QFrame, public LineOrientedVT100Client {
    Q_OBJECT

public:
    QtTerminalWindow();
    void setPty(Pty* pty) { m_pty = pty; }

protected:
    virtual void keyPressEvent(QKeyEvent* event);

    virtual void characterAppended();
    virtual void somethingLargeChanged();
    void paintEvent(QPaintEvent*);
    void renderLine(QPainter& painter, Line* line, int& currentBaseline);

private:
    Pty* m_pty;
    char m_previousCharacter;
    QFontMetrics* m_fontMetrics;
    QFont* m_font;

signals:
    void updateNeeded();

public slots:
    void handleUpdateNeeded();

};
