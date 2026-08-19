#pragma once

#include <filesystem>

#include <QObject>
#include <QtQml/qqml.h>
#include <nlohmann/json.hpp>


class file_writer : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FileWriter)
    QML_SINGLETON

  public:
    Q_INVOKABLE static void write_json(const QUrl &path, const QString &content);
    Q_INVOKABLE static QString read_file(const QUrl &path);
};
