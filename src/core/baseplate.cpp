#include "baseplate.h"

#include "util.h"

using b = core::BaseplateType;
const std::map<std::string, core::BaseplateType::Kind> stringToBaseplate = {
    { "circuits-BasePlateBattery", b::ConductorBaseBattery },
    { "circuits-BasePlate", b::ConductorBase },
    { "general-cornerMarker", b::GreenCorner },
    { "gears-BasePlate", b::GearBase },
    { "cubes-Baseplate", b::CubeBase }
};

const std::map<core::BaseplateType::Kind, std::string> baseplateToString =
    core::util::reverse_map(stringToBaseplate);


void core::to_json(nlohmann::json &j, const Baseplate &b)
{
    const auto pose = util::VectorAndRotationToTransform(b.position(), b.rotation());

    j = nlohmann::json{ { "part_type", baseplateToString.at(b.kind()) },
        { "pose_relative_to_corner", pose } };
}

void core::from_json(const nlohmann::json &j, Baseplate &b)
{
    std::string part_type;
    j.at("part_type").get_to(part_type);
    b.set_kind(stringToBaseplate.at(part_type));

    Eigen::Affine3d pose;
    j.at("pose_relative_to_corner").get_to(pose);
    if(j["y-up"] == true)
        pose = util::from_y_up_to_z_up(pose);



    auto [vec, angle] = util::TransformToVectorAndRotation(pose);
    b.set_rotation(angle);
    b.set_position(vec);
}



core::Baseplate::Baseplate(QObject *parent) : QObject(parent){}

QVector3D core::Baseplate::position() const { return position_; }

core::BaseplateType::Kind core::Baseplate::kind() const { return kind_; }

double core::Baseplate::rotation() const { return rotation_; }

void core::Baseplate::set_position(QVector3D pos)
{
    position_ = pos;
    emit positionChanged();
}

void core::Baseplate::set_kind(BaseplateType::Kind k)
{
    kind_ = k;
    emit kindChanged();
}

void core::Baseplate::set_rotation(double z)
{
    rotation_ = z;
    emit rotationChanged();
}
