#include "pty.h"
#include "VT100.h"
#include "QtTerminalWindow.h"
#include <cstdio>
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    QtTerminalWindow* window = new QtTerminalWindow();
    VT100 vt100(window);

    Pty* pty = new Pty(&vt100);

    window->show();
    return application.exec();
}
