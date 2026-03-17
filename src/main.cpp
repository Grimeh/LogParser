#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QColor>
#include <QtLogging>
#include "app/MainWindow.h"
#include <iostream>

static void applyDarkPalette(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    const QColor window(45,45,45);
    const QColor base(37,37,38);
    const QColor text(220,220,220);
    const QColor disabled(127,127,127);
    const QColor hl(53,132,228);

    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Disabled, QPalette::Text, disabled);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Highlight, hl);
    p.setColor(QPalette::HighlightedText, QColor(0,0,0));
    p.setColor(QPalette::PlaceholderText, QColor(150,150,150));

    app.setPalette(p);
}

int main(int argc, char* argv[]) {
    qSetMessagePattern(
        "%{time yyyy-MM-dd hh:mm:ss} "
        "[%{type}] "
        "%{file}:%{line} %{function} "
        "-- %{message}"
    );

    QApplication app(argc, argv);
    applyDarkPalette(app);

    MainWindow w;
    w.resize(1000, 1000);
    w.show();

    // initiate tabs with command line input paths
    if (argc > 1) {
        char** pathList = &argv[1];
        w.openLogList(argc-1, pathList);
    }

    return app.exec();
}