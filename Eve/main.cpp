#include "eve.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Eve w;
    w.show();

    return a.exec();
}
