#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Ratatoskr");
    app.setOrganizationName("ratatoskr");

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Ratatoskr", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
