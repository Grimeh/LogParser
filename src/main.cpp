#include <QApplication>
#include <QPushButton>
#include <QObject>
#include <iostream>

int main(int argc, char *argv[]) {
    
    qSetMessagePattern(
        "%{time yyyy-MM-dd hh:mm:ss} "
        "[%{type}] "
        "%{file}:%{line} %{function} "
        "-- %{message}"
    );

    QApplication app(argc, argv);

    QPushButton button("Press me");
    QObject::connect(&button, &QPushButton::clicked,  [](){
        std::cout << "Hello, world!!" << std::endl;
        qInfo() << "foo";
    });

    button.resize(200, 60); // optional
    button.show();
    return app.exec();
}