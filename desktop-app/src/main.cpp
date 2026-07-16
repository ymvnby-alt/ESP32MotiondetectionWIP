#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("CSI Motion Detector");
    QApplication::setOrganizationName("CsiMotionDetector");

    MainWindow window;
    window.show();

    return app.exec();
}
