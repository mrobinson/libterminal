#include "QtTerminalWindow.h"
#include "../pty.h"
#include <QKeyEvent>
#include <QPainter>
#include <QtGlobal>
#include <cstdio>

QtTerminalWindow::QtTerminalWindow()
    : QFrame(NULL)
    , m_pty(NULL)
    , m_previousCharacter('\0')
    , m_fontMetrics(font())
{
    connect(this, SIGNAL(updateNeeded()),
        this, SLOT(handleUpdateNeeded()));
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

void QtTerminalWindow::renderLine(QPainter& painter, std::vector<TerminalContentNode*>* line, int& currentBaseline)
{
    currentBaseline += m_fontMetrics.height();

    int currentX = 0;
    for (size_t i = 0; i < line->size(); i++) {
        QString text = QString::fromUtf8(line->at(i)->text(), line->at(i)->textLength());
        painter.drawText(currentX, currentBaseline, text);
        currentX += m_fontMetrics.tightBoundingRect(text).width();
    }
}

void QtTerminalWindow::paintEvent(QPaintEvent* event)
{
    if (isHidden())
        return;

    QPainter painter(this);
    painter.fillRect(event->rect(), QBrush(Qt::white));

    int totalLines = numberOfLines();
    int linesToDraw = qMin(totalLines, rect().height() / m_fontMetrics.height());
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
