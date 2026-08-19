#include <pybind11/embed.h>


#include <iostream>

#include <thread>


#include <QApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QtGui>

#include "enums.h"
#include "task.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <QtQml/qqmlextensionplugin.h>
Q_IMPORT_QML_PLUGIN(ComponentsPlugin)
Q_IMPORT_QML_PLUGIN(HrcModelsPlugin)
Q_IMPORT_QML_PLUGIN(MockPlugin)

int main(int argc, char *argv[])
{
    using namespace std::literals;
    // timestamp for filename
    auto timestamp = std::chrono::system_clock::now();
    std::time_t now_tt = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm = *std::localtime(&now_tt);

    std::stringstream filename;
    filename << "./logs/" << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << ".jsonl";

    // data logger for recording of study information
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.back()->set_pattern("[%T] [%^%n%$] %v");

    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename.str()));
    sinks.back()->set_pattern("{\"time\": \"%c\", \"message\": %v}");
    auto data_logger = std::make_shared<spdlog::logger>("data_logger", sinks.begin(), sinks.end());
    data_logger->set_level(spdlog::level::trace);
    data_logger->flush_on(spdlog::level::trace);// always write data directly to file
    spdlog::register_logger(data_logger);

    // default console logger for debug messages
    auto logger = spdlog::stdout_color_mt("debug_logger");
    logger->set_pattern("[%T] [%^%l%$] %v");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::trace);


    pybind11::initialize_interpreter();
    try {
        pybind11::exec(R"(
        import networkx
    )");
    } catch (const std::exception &e) {
        std::cout << e.what();
        return 1;
    }

    try {
        QApplication app(argc, argv);
        app.setOrganizationName("University Bayreuth AI3");
        app.setOrganizationDomain("mysoft.com");
        app.setApplicationName("HRC Benchmark");

        QQmlApplicationEngine engine;
        engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

        // wip: test imports
        const QStringList importPaths = engine.importPathList();
        qDebug() << "QML Import Paths:";
        for (const QString &path : importPaths) {
            qDebug() << path;
        }



        engine.load(QUrl(QStringLiteral("qrc:/qt/qml/App/main.qml")));

        if (engine.rootObjects().isEmpty()) return -1;

        const auto res = app.exec();
        pybind11::finalize_interpreter();
        return res;

    } catch (const std::exception &e) {
        std::cout << e.what();
    }
    return 1;
}
