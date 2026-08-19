#pragma once

#include <QObject>
#include <QVector3D>
#include <QtQml/qqml.h>

#include <nlohmann/json.hpp>
#include "enums.h"

namespace core {

/**
 * @brief QML-exposed data object describing a single assembly baseplate in the scene.
 *
 * A task description contains one or more baseplates, each with a kind (gear base,
 * conductor base, etc.), a 3D position in the assembly area, and a yaw rotation.
 * The 3D renderer uses these values to position the corresponding 3D model in the scene.
 */
class Baseplate : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVector3D position READ position WRITE set_position NOTIFY positionChanged)
    Q_PROPERTY(core::BaseplateType::Kind kind READ kind WRITE set_kind NOTIFY kindChanged)
    Q_PROPERTY(double rotation READ rotation WRITE set_rotation NOTIFY rotationChanged)


  public:
    Baseplate(QObject *parent = nullptr);


    QVector3D position() const;
    BaseplateType::Kind kind() const;
    double rotation() const;

    void set_position(QVector3D pos);
    void set_kind(BaseplateType::Kind k);
    void set_rotation(double z);

  signals:
    void positionChanged();
    void kindChanged();
    void rotationChanged();

  private:
    BaseplateType::Kind kind_;
    double rotation_;
    QVector3D position_;
};

void to_json(nlohmann::json &j, const Baseplate &o);
void from_json(const nlohmann::json &j, Baseplate &o);


}// namespace core
