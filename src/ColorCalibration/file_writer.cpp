#include "file_writer.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include "nlohmann/json.hpp"

void file_writer::write_json(const QUrl &path, const QString &content)
{
    std::ofstream file(path.toLocalFile().toStdString());

    const auto json = nlohmann::json::parse(content.toStdString());

    std::stringstream value;
    value << std::setw(4) << json;

    file << value.str();
    std::cout << " Wrote config file:\n"
              << value.str() << "\n"
              << "To path " << path.toLocalFile().toStdString() << "\n";
}

QString file_writer::read_file(const QUrl &path)
{
    const std::ifstream const file(path.toLocalFile().toStdString());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return QString::fromStdString(buffer.str());
}
