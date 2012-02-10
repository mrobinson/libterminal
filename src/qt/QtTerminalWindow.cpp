#include "QtTerminalWindow.h"
#include "Line.h"
#include "../pty.h"
#include <QKeyEvent>
#include <QPainter>
#include <QtGlobal>
#include <cstdio>
#include <cmath>

QtTerminalWindow::QtTerminalWindow()
    : QFrame(NULL)
    , m_pty(NULL)
    , m_previousCharacter('\0')
    , m_fontMetrics(0)
    , m_font(0)
    , m_size(80, 25)
{
    QFont* newFont = new QFont("Inconsolata");
    newFont->setFixedPitch(true);
    setFont(newFont);

    connect(this, SIGNAL(updateNeeded()),
        this, SLOT(handleUpdateNeeded()));
}

void QtTerminalWindow::setFont(QFont* font)
{
    delete m_font;
    delete m_fontMetrics;

    m_font = font;
    m_fontMetrics = new QFontMetrics(*m_font);

    // It seems that the maxWidth of the font is off by one pixel for many fixed
    // width fonts. We likely just need to lay the font out manually, but this is
    // a good hack for now.
    QSize increment(m_fontMetrics->maxWidth() - 1, m_fontMetrics->height());
    setSizeIncrement(increment);
    resize(increment.width() * m_size.width(), increment.height() * m_size.height());

}

void QtTerminalWindow::handleUpdateNeeded()
{
    // TODO: Be smarter.
    update();
}

void QtTerminalWindow::characterAppended()
{
    emit updateNeeded();
}

void QtTerminalWindow::somethingLargeChanged()
{
    emit updateNeeded();
}

void QtTerminalWindow::renderTextAt(const char* text, size_t numberOfCharacters, bool isCursor, int x, int y)
{
    QString string = QString::fromUtf8(text, numberOfCharacters);
    int realX = sizeIncrement().width() * x;
    int realY = sizeIncrement().height() * (y + 1);
    m_currentPainter->drawText(realX, realY, text);
}

int QtTerminalWindow::charactersWide()
{
    return m_size.width();
}

int QtTerminalWindow::charactersTall()
{
    return m_size.height();
}

void QtTerminalWindow::paintEvent(QPaintEvent* event)
{
    if (isHidden())
        return;

    QPainter painter(this);
    m_currentPainter = &painter;

    painter.fillRect(event->rect(), QBrush(Qt::white));
    painter.setFont(*m_font);

    paint();

    m_currentPainter = 0;
    QFrame::paintEvent(event);
}

void QtTerminalWindow::keyPressEvent(QKeyEvent* event)
{
    if (!m_pty)
        return;

    // This is really dumb, but it's only temporary.
    QString eventString = event->text();
    if (eventString.isEmpty())
       return;
    QByteArray utf8String = eventString.toUtf8();
    m_pty->ptyWrite(utf8String.data(), utf8String.length());
}

void QtTerminalWindow::setPty(Pty* pty)
{
    m_pty = pty;
    m_pty->setSize(m_size.width(), m_size.height());
}

void QtTerminalWindow::resizeEvent(QResizeEvent* resizeEvent)
{
    if (m_pty) {
        m_cursorColumn = 1;
        m_size = QSize(size().width() / sizeIncrement().width(),
                       size().height() / sizeIncrement().height());
        m_pty->setSize(m_size.width(), m_size.height());
    }
    QWidget::resizeEvent(resizeEvent);
}

void QtTerminalWindow::bell()
{
    printf("BING!\n");
}
