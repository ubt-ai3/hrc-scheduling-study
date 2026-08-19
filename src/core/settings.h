#pragma once


#include <QObject>
#include <QtQml/qqml.h>
#include <nlohmann/json.hpp>

#include <filesystem>
namespace core {

/**
 * @brief QML singleton that holds all runtime experiment configuration.
 *
 * Settings are either loaded from a configuration JSON file via load_json() (Option A,
 * recommended for study runs) or adjusted manually via the Settings dialog (Option B).
 * Changes to any property are propagated to the QML engine via the corresponding
 * *Changed signals.
 *
 * Key properties:
 *   - task_description     — URL of the task_description.json for the current task variant
 *   - robot_actor          — whether the robot participates in this run
 *   - human_actor          — whether the human participates in this run
 *   - interrupt_human_after — list of operation indices at which to trigger an interruption
 *   - reschedule_after_interrupt — whether the scheduler re-plans after the human returns
 */
class settings : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Settings)
    QML_SINGLETON
    Q_PROPERTY(bool robot_actor MEMBER robot_actor NOTIFY robot_actorChanged)
    Q_PROPERTY(bool human_actor MEMBER human_actor NOTIFY human_actorChanged)
    Q_PROPERTY(bool reschedule_after_interrupt MEMBER reschedule_after_interrupt NOTIFY reschedule_after_interruptChanged)
    Q_PROPERTY(QVariantList interrupt_human_after MEMBER interrupt_human_after NOTIFY
            interrupt_human_afterChanged)
    Q_PROPERTY(QUrl task_description READ task_description WRITE set_task_description NOTIFY
            task_descriptionChanged)
    Q_PROPERTY(QUrl config_file_path MEMBER config_file_path NOTIFY config_file_pathChanged)

  public:
    settings(QObject *parent = nullptr);
    Q_INVOKABLE void load_json(const QString &pathname);

    QUrl task_description() const;
    void set_task_description(const QUrl &new_path);

    bool robot_actor = true;
    bool human_actor = true;
    bool reschedule_after_interrupt = false;


    QVariantList interrupt_human_after = { 1, 5 };
    QUrl config_file_path;


  signals:
    void robot_actorChanged();
    void human_actorChanged();
    void reschedule_after_interruptChanged();
    void task_descriptionChanged();
    void interrupt_human_afterChanged();
    void config_file_pathChanged();
    void completedWrite();

  private:
    QUrl task_description_ = QUrl();
};

void to_json(nlohmann::json &j, const settings &s);
void from_json(const nlohmann::json &j, settings &s);


}// namespace core