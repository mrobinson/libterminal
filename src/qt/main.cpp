#include "VT100.h"
#include "QtTerminalWindow.h"
#include "pty.h"
#include <cstdio>
#include <QApplication>

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    QtTerminalWindow* window = new QtTerminalWindow();
    VT100 vt100(window);
    Pty pty(&vt100);

    window->setPty(&pty);
    window->show();
    return application.exec();
}
