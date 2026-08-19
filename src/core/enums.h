#pragma once

#include <QObject>
#include <QtQml/qqml.h>


#include "entity.h"


namespace core {

/** @brief QML-accessible enum wrapper for assembly part types. */
class EntityType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    enum Type {
        Gear = int(benchmark::Entity::kind::gear),
        Conductor = int(benchmark::Entity::kind::conductor),
        Cube = int(benchmark::Entity::kind::cube),
        LongCube = int(benchmark::Entity::kind::long_cube)
    };
    Q_ENUM(Type)
};

/** @brief QML-accessible enum for the experiment state machine. */
class TaskState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    enum task_state { NotStarted, Running, Interrupted, Done };
    Q_ENUM(task_state)
};

/** @brief QML-accessible enum for the baseplate model variant. */
class BaseplateType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    enum Kind { ConductorBase, ConductorBaseBattery, GreenCorner, GearBase, CubeBase };
    Q_ENUM(Kind);
};

/** @brief QML-accessible enum wrapper for part colors. */
class EntityColor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    enum Color {
        Orange = int(benchmark::Entity::color::orange),
        Green = int(benchmark::Entity::color::green),
        Blue = int(benchmark::Entity::color::blue)
    };
    Q_ENUM(Color)
};

/**
 * @brief QML-accessible enum describing the scheduling state of a single Operation.
 *
 *   Open      — not yet assigned to any actor
 *   HumanNext — assigned to the human; timer/logging started
 *   RobotNext — assigned to the robot; timer/logging started
 *   Closed    — completed; duration logged
 */
class OperationState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    explicit OperationState() = default;

    enum State { Open, HumanNext, RobotNext, Closed };
    Q_ENUM(State)
};


}// namespace core