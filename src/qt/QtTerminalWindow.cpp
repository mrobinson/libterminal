#include "QtTerminalWindow.h"
#include "../pty.h"
#include <QKeyEvent>
#include <cstdio>

QtTerminalWindow::QtTerminalWindow()
    : QTextEdit(NULL)
    , m_pty(NULL)
    , m_previousCharacter('\0')
{
    connect(this, SIGNAL(appendCharacterSignal(char)),
        this, SLOT(handleAppendCharacter(char)));
}

void QtTerminalWindow::handleAppendCharacter(char character)
{
    if (character != '\n' || m_previousCharacter != '\r')
        insertPlainText(QString(character));
    m_previousCharacter = character;
}

void QtTerminalWindow::appendCharacter(char character)
{
    emit appendCharacterSignal(character);
}

void QtTerminalWindow::changeColor(int, int)
{
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
