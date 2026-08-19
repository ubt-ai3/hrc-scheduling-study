#include "settings.h"

#include <iostream>
#include <fstream>

#include "spdlog/spdlog.h"

#include <QSettings>
#include <QMessageBox>


namespace core {
settings::settings(QObject *parent) : QObject(parent) {}

void settings::load_json(const QString &pathname)
{
    try {
        using namespace std::literals;
        spdlog::trace("Loading settings from "s + pathname.toStdString());

        auto path = std::filesystem::path(QUrl(pathname).toLocalFile().toUtf8().toStdString());
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("could not read settings");
        }
        auto json = nlohmann::json();
        file >> json;
        config_file_path = pathname;
        emit config_file_pathChanged();
        json.get_to(*this);
        emit completedWrite();


    } catch (const std::exception &e) {
        QMessageBox messageBox;
        messageBox.critical(nullptr, "Error", e.what());
        messageBox.setFixedSize(500, 200);
    }
}

QUrl settings::task_description() const { return task_description_; }

void settings::set_task_description(const QUrl &new_path)
{
    if (new_path == task_description_) { return;
}

    const auto path = std::filesystem::path(new_path.toLocalFile().toStdString());
    if (!config_file_path.isEmpty() && path.is_absolute()) {
        const auto std_config_path = std::filesystem::path(
            config_file_path.adjusted(QUrl::RemoveFilename).toLocalFile().toStdString());
        const auto relative_path = std::filesystem::relative(path, std_config_path);
        task_description_ = QUrl::fromLocalFile(QString(relative_path.string().c_str()));
    } else {
        task_description_ = new_path;
    }


    emit task_descriptionChanged();
}

void to_json(nlohmann::json &j, const settings &s) {}

void from_json(const nlohmann::json &j, settings &s)
{
    j.at("human_actor_enabled").get_to(s.human_actor);
    j.at("robot_actor_enabled").get_to(s.robot_actor);
    j.at("reschedule_after_interrupt").get_to(s.reschedule_after_interrupt);


    QVariantList i;
    for (const auto& entry : j.at("interruptions")) {
        i << entry.get<int>();
    }
    s.interrupt_human_after = i;

    s.set_task_description(
        QUrl::fromLocalFile(j.at("task_description_path").get<std::string>().c_str()));

    emit s.robot_actorChanged();
    emit s.human_actorChanged();
    emit s.reschedule_after_interruptChanged();
    emit s.interrupt_human_afterChanged();
}


}// namespace core