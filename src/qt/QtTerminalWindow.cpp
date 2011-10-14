#include "QtTerminalWindow.h"
#include "Line.h"
#include "../pty.h"
#include <QKeyEvent>
#include <QPainter>
#include <QtGlobal>
#include <cstdio>

QtTerminalWindow::QtTerminalWindow()
    : QFrame(NULL)
    , m_pty(NULL)
    , m_previousCharacter('\0')
    , m_fontMetrics(0)
    , m_font(0)
    , m_size(80, 25)
{
    setFont(new QFont("Consolas"));
    connect(this, SIGNAL(updateNeeded()),
        this, SLOT(handleUpdateNeeded()));
}

void QtTerminalWindow::setFont(QFont* font)
{
    delete m_font;
    delete m_fontMetrics;

    m_font = font;
    m_fontMetrics = new QFontMetrics(*m_font);

    QSize increment(m_fontMetrics->maxWidth(), m_fontMetrics->height());
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
    currentBaseline += m_fontMetrics->height();

    QString text = QString::fromUtf8(line->chars());
    painter.drawText(0, currentBaseline, text);
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
    int currentBaseline = 0;
    for (int i = linesToDraw; i > 0; i--) {
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
    if (m_pty)
        m_pty->setSize(size().width() / sizeIncrement().width(),
                       size().height() / sizeIncrement().height());
    QWidget::resizeEvent(resizeEvent);
}
