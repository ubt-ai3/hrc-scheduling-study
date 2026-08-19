#pragma once

#include <string>

#include <QObject>
#include <QVector3D>

#include <Eigen/Dense>
#include <nlohmann/json.hpp>

#include "entity.h"
#include "enums.h"

namespace core {

/**
 * @brief Represents a single pick-and-place step in the assembly task graph.
 *
 * Each operation has a start entity (where to pick) and an end entity (where to place),
 * both described by a type, color, 3D position (in the assembly coordinate frame) and
 * yaw rotation.  Operations form a directed dependency graph: left/right hold predecessor
 * and successor operations whose ordering constraints the scheduler must respect.
 *
 * The state property drives the UI and the json_logger:
 *   Open → HumanNext or RobotNext (assigned by scheduler) → Closed (completed).
 */
class Operation : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(EntityType::Type type READ type)
    Q_PROPERTY(EntityColor::Color color READ color)
    Q_PROPERTY(double start_rotation READ start_rotation NOTIFY opChanged)
    Q_PROPERTY(QVector3D start_position READ start_position NOTIFY opChanged)
    Q_PROPERTY(double end_rotation READ end_rotation NOTIFY opChanged)
    Q_PROPERTY(QVector3D end_position READ end_position NOTIFY opChanged)
    Q_PROPERTY(OperationState::State state READ state WRITE set_state NOTIFY stateChanged)
    Q_PROPERTY(bool display MEMBER display NOTIFY displayChanged)

  public:
    Operation(QObject *parent = nullptr);

    int id() const;
    void set_id(int id);
    EntityType::Type type() const;
    void set_type(EntityType::Type type);
    EntityColor::Color color() const;
    void set_color(EntityColor::Color color);
    double start_rotation() const;
    void set_start_rotation(double start_rotation);
    QVector3D start_position() const;
    void set_start_position(const QVector3D &start_position);
    double end_rotation() const;
    void set_end_rotation(double end_rotation);
    QVector3D end_position() const;
    void set_end_position(const QVector3D &end_position);
    OperationState::State state() const;
    void set_state(OperationState::State state);


    bool reached() const;
    QVector<Operation *> left;
    QVector<Operation *> right;
    bool display = true;
    std::pair<benchmark::Entity, benchmark::Entity> to_world_frame(const benchmark::Entity &corner);
  signals:
    void opChanged();
    void stateChanged();
    void displayChanged();

  private:
    int id_;
    EntityType::Type type_;
    EntityColor::Color color_;
    double start_rotation_;
    QVector3D start_position_;

    double end_rotation_;
    QVector3D end_position_;

    OperationState::State state_;
};

void to_json(nlohmann::json &j, const Operation &o);
void from_json(const nlohmann::json &j, Operation &o);

Eigen::Affine3d corner_frame(const benchmark::Entity &corner);
}// namespace core