#include "ui_theme.h"
#include "window.h"
#include "native_dialogue_smoke.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Impera Workshop");
    app.setOrganizationName("Impera");
    app.setApplicationVersion("0.1");
    Workshop::applyTheme(app);
    QCommandLineParser parser;
    parser.setApplicationDescription("Native Ultima 5 mod-package authoring for Impera");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"playtest-smoke-test", "Check the packaged native dialogue runtime and exit"});
    parser.addOption({"game", "Read original DOS resources from this folder", "directory"});
    parser.addOption({"project", "Open a saved Workshop project", "file"});
    parser.addOption({"resource", "Initially select this resource", "name"});
    parser.addOption(
        {"smoke-test", "Open every resource editor and exit (uses --game or --project)"});
    parser.addOption(
        {"screenshot", "Save a UI screenshot before exiting (for development)", "file"});
    parser.process(app);
    if (parser.isSet("playtest-smoke-test"))
        return nativeDialogueSmokeTest();
    WorkshopWindow window;
    if (parser.isSet("game") && !window.openGame(parser.value("game")))
        return 1;
    if (parser.isSet("project") && !window.openProject(parser.value("project")))
        return 1;
    if (!parser.isSet("game") && !parser.isSet("project"))
        window.restoreGameDirectory();
    if (parser.isSet("resource")) {
        if (!window.projectForTests().resources.contains(parser.value("resource")))
            return 1;
        window.selectResource(parser.value("resource"));
    }
    window.show();
    if (parser.isSet("smoke-test")) {
        if (window.projectForTests().resources.isEmpty())
            return 1;
        QTimer::singleShot(0, &window, [&] {
            for (auto name : window.projectForTests().resources.keys()) {
                window.selectResource(name);
                app.processEvents();
            }
            app.quit();
        });
    }
    if (parser.isSet("screenshot"))
        QTimer::singleShot(250, &window, [&] {
            bool ok = window.grab().save(parser.value("screenshot"), "PNG");
            app.exit(ok ? 0 : 1);
        });
    return app.exec();
}
