#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("RPG Maker Decrypter"));
    QApplication::setApplicationDisplayName(QStringLiteral("RPG Maker Decrypter"));
    QApplication::setOrganizationName(QStringLiteral("RPGMakerDecrypter"));

    RPGMakerDecrypter::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
