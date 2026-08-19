#include "qml_logger.h"
#include "spdlog/spdlog.h"

namespace core {
qml_logger::qml_logger(QObject *parent) : QObject(parent) {}


Q_INVOKABLE void core::qml_logger::message(const QString &message)
{
	spdlog::get("data_logger")->info(message.toStdString());
}

}// namespace core