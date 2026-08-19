#pragma once

#include <chrono>

#include <QObject>
#include <QtQml/qqml.h>

#include "operation.h"

namespace core {

/**
 * @brief QML singleton that forwards log messages from QML to the spdlog "data_logger".
 *
 * Exposed to QML as `Logger`.  Used by questionnaire pages and other QML components
 * to record user interactions (e.g., survey answers) alongside the structured JSONL log.
 */
class qml_logger : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Logger)
    QML_SINGLETON
  public:
    explicit qml_logger(QObject *parent = nullptr);
    /** @brief Appends @p message as a plain-text JSONL entry to the data log. */
    Q_INVOKABLE void message(const QString &message);
};

}// namespace core
