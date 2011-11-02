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
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* resizeEvent);

    virtual void characterAppended();
    virtual void somethingLargeChanged();
    virtual void bell();

    void paintEvent(QPaintEvent*);
    void renderLine(QPainter& painter, Line* line, int& currentBaseline);
    void setFont(QFont*);

private:
    Pty* m_pty;
    char m_previousCharacter;
    QFontMetrics* m_fontMetrics;
    QFont* m_font;
    QSize m_size;

    void calculateHowManyLinesFit(int linesToDraw, int& linesThatFit, int& consumedHeight);

signals:
    void updateNeeded();

public slots:
    void handleUpdateNeeded();

};
