#include "alice.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Alice w;
    w.show();

    return a.exec();
}
