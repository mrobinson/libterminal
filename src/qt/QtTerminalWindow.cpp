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

void QtTerminalWindow::renderLine(QPainter& painter, Line* line, int& currentBaseline)
{
    QString text = QString::fromUtf8(line->chars());
    int lineLength = text.length();

    while (lineLength > 0) {
        int charactersToPaint = qMin(lineLength, m_size.width());
        currentBaseline += m_fontMetrics->height();
        painter.drawText(0, currentBaseline, text.left(charactersToPaint));

        lineLength -= charactersToPaint;
        text = text.mid(charactersToPaint);
    }

}

void QtTerminalWindow::calculateHowManyLinesFit(int linesToDraw, int& linesThatFit, int& consumedHeight)
{
    int currentLine = numberOfLines() - 1;
    while (consumedHeight < m_size.height() && linesThatFit < linesToDraw) {
        linesThatFit++;
        consumedHeight += ceilf(static_cast<float>(lineAt(currentLine)->numberOfCharacters()) /
                                static_cast<float>(m_size.width()));
        currentLine--;
    }
}

void QtTerminalWindow::paintEvent(QPaintEvent* event)
{
    if (isHidden())
        return;

    QPainter painter(this);
    painter.fillRect(event->rect(), QBrush(Qt::white));
    painter.setFont(*m_font);

    int totalLines = numberOfLines();
    int linesToDraw = qMin(totalLines, rect().height() / m_fontMetrics->height());

    int linesThatFit = 0;
    int consumedHeight = 0;
    calculateHowManyLinesFit(linesToDraw, linesThatFit, consumedHeight);

    int currentBaseline = 0;
    for (int i = linesThatFit; i > 0; i--) {
        renderLine(painter, lineAt(totalLines - i), currentBaseline);
    }
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
