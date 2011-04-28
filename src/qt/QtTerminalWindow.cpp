#include "QtTerminalWindow.h"

QtTerminalWindow::QtTerminalWindow()
    : QTextEdit(NULL)
{
    connect(this, SIGNAL(appendCharacterSignal(char)),
        this, SLOT(handleAppendCharacter(char)));
}

void QtTerminalWindow::handleAppendCharacter(char character)
{
    insertPlainText(QString(character));
}

void QtTerminalWindow::appendCharacter(char character)
{
    emit appendCharacterSignal(character);
}

void QtTerminalWindow::changeColor(int, int)
{
}
