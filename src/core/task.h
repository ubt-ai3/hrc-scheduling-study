#pragma once

#include <QObject>
#include <QVector>
#include <QUrl>
#include <QtQml/qqml.h>

#include "baseplate.h"
#include "operation.h"
#include "hrc_schedule.h"
#include "settings.h"
#include "json_logger.h"
#include "robot_handler.h"

namespace core {

/** @brief Encodes a directed dependency edge between two operations by their IDs. */
struct edge
{
    int start_id;
    int end_id;
};

/**
 * @brief Central QML element that owns and drives a complete HRC experiment run.
 *
 * Exposed to QML as `Task`.  Responsibilities:
 *   - Loads a task description (JSON) and configuration file at startup.
 *   - Starts hardware exploration to detect part positions, then maps
 *     task-description poses to real-world entities.
 *   - Calls the Python scheduler to build a schedule and assigns operations
 *     to the human and robot actors.
 *   - Dispatches robot operations asynchronously via robot_handler.
 *   - Responds to human events (human_op_completed, human_returned_from_interrupt)
 *     forwarded from QML buttons.
 *   - Re-schedules after an interruption if reschedule_after_interrupt is enabled.
 *   - Drives json_logger to record all timing and state transitions.
 *
 * Set connect_to_hardware = false (in Experiment.qml) to run in simulation mode
 * without a physical robot or camera.
 */
class task : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Task)
    Q_PROPERTY(QVector<core::Baseplate *> baseplates READ baseplates WRITE set_baseplates NOTIFY
            baseplatesChanged)
    Q_PROPERTY(QVector<core::Operation *> operations READ operations WRITE set_operations NOTIFY
            operationsChanged)
    Q_PROPERTY(TaskState::task_state state MEMBER state NOTIFY stateChanged)

    Q_PROPERTY(core::settings *settings MEMBER settings_ WRITE set_settings NOTIFY settingsChanged
            REQUIRED)
    Q_PROPERTY(
        bool connect_to_hardware READ connect_to_hardware WRITE set_connect_to_hardware REQUIRED)
  public:
    task(QObject *parent = nullptr);

    QVector<core::Baseplate *> baseplates() const;
    void set_baseplates(const QVector<core::Baseplate *> &baseplates);

    QVector<core::Operation *> operations() const;
    void set_operations(const QVector<core::Operation *> &operations);

    void set_settings(settings *new_settings);

    bool connect_to_hardware() const;
    void set_connect_to_hardware(bool connect);

    benchmark::Entity map_ops_to_world(const std::vector<benchmark::Entity> &explo_result);
    void snap_ops_to_detected_entities(std::vector<benchmark::Entity> explo_result,
    const benchmark::Entity &corner);
    QUrl get_config_file();
    Q_INVOKABLE void human_op_completed();
    Q_INVOKABLE void human_returned_from_interrupt();
    Q_INVOKABLE void start();

    TaskState::task_state state;
    // Application state, overridden from qml


  signals:
    void baseplatesChanged();
    void operationsChanged();
    void stateChanged();
    void settingsChanged();

    void new_robot_operation(const Operation *op);

  private:
    bool connect_to_hardware_;

    settings *settings_;
    int operations_completed_by_human_ = 0;
    void load_json(const QUrl &task_description_path, const QUrl &config_file_path);
    void reset();
    void set_operations(const std::map<int, Operation *> &operations);

    void reschedule(bool interrupted = false);
    void assign_next_operations();

    std::mutex schedule_lock_;
    void robot_op_completed();

    QVector<Baseplate *> baseplates_;
    QVector<Operation *> operations_;

    bool load_exploration();
    std::map<Operation *, std::pair<benchmark::Entity, benchmark::Entity>> worldPose_from_op_;


    json_logger logger_;
    schedule schedule_;
    std::unique_ptr<robot_handler> robot_;

    QUrl task_description_;
    QUrl config_file_;
};

}// namespace core
