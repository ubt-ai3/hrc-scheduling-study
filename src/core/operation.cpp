#include "operation.h"
#include "util.h"

using k = benchmark::Entity::kind;
using c = benchmark::Entity::color;

const std::map<std::string, std::pair<benchmark::Entity::kind, benchmark::Entity::color>>
    stringToKind = { { "circuits-ConductorBlue", { k::conductor, c::blue } },
        { "circuits-ConductorGreen", { k::conductor, c::green } },
        { "circuits-ConductorOrange", { k::conductor, c::orange } },
        { "cubes-bigCubeBlue", { k::long_cube, c::blue } },
        { "cubes-CubeGreen", { k::cube, c::green } },
        { "cubes-CubeOrange", { k::cube, c::orange } },
        { "gears-GearBlue", { k::gear, c::blue } },
        { "gears-GearGreen", { k::gear, c::green } },
        { "gears-GearOrange", { k::gear, c::orange } } };


const std::map<std::pair<benchmark::Entity::kind, benchmark::Entity::color>, std::string>
    kindToString = core::util::reverse_map(stringToKind);


void core::to_json(nlohmann::json &j, const Operation &o)
{
    const auto start_pose =
        util::VectorAndRotationToTransform(o.start_position(), o.start_rotation());
    const auto end_pose = util::VectorAndRotationToTransform(o.end_position(), o.end_rotation());


    j = nlohmann::json{ { "id", o.id() },
        { "label", std::string("Operation ") + std::to_string(o.id()) },
        { "part_type", kindToString.at({ k{ o.type() }, c{ o.color() } }) },
        { "start_pose_relative_to_corner", start_pose },
        { "goal_pose_relative_corner", end_pose } };
}

void core::from_json(const nlohmann::json &j, Operation &o)
{
    o.set_id(j.at("id").get<int>());

    std::string part_type;
    j.at("part_type").get_to(part_type);
    const auto [kind, color] = stringToKind.at(part_type);

    o.set_type(static_cast<core::EntityType::Type>(kind));
    o.set_color(static_cast<core::EntityColor::Color>(color));
    o.set_state(core::OperationState::State::Open);

    auto start_pos = Eigen::Affine3d();
    auto end_pos = Eigen::Affine3d();
    j.at("start_pose_relative_to_corner").get_to(start_pos);
    j.at("goal_pose_relative_corner").get_to(end_pos);

    
    if(j["y-up"] == true){
        start_pos = util::from_y_up_to_z_up(start_pos);
        end_pos = util::from_y_up_to_z_up(end_pos);
    }

    auto [start_vec, start_rot] = util::TransformToVectorAndRotation(start_pos);
    auto [end_vec, end_rot] = util::TransformToVectorAndRotation(end_pos);

    o.set_start_position(start_vec);
    o.set_start_rotation(start_rot);
    o.set_end_position(end_vec);
    o.set_end_rotation(end_rot);
}

Eigen::Affine3d core::corner_frame(const benchmark::Entity &corner)
{
    const auto corner_trans = corner.pose_.translation();
    auto corner_frame = Eigen::Translation3d(corner_trans)
                        * Eigen::AngleAxisd(-M_PI / 4, Eigen::Vector3d::UnitZ())
                        * Eigen::Translation3d(-corner_trans) * corner.pose_;
    return corner_frame;
}

int core::Operation::id() const { return id_; }

void core::Operation::set_id(int id) { id_ = id; }

core::EntityType::Type core::Operation::type() const { return type_; }

void core::Operation::set_type(EntityType::Type type) { type_ = type; }

core::EntityColor::Color core::Operation::color() const { return color_; }

void core::Operation::set_color(EntityColor::Color color) { color_ = color; }

double core::Operation::start_rotation() const { return start_rotation_; }

void core::Operation::set_start_rotation(double start_rotation)
{
    start_rotation_ = start_rotation;
    emit opChanged();
}

QVector3D core::Operation::start_position() const { return start_position_; }

void core::Operation::set_start_position(const QVector3D &start_position)
{
    start_position_ = start_position;
    emit opChanged();
}

double core::Operation::end_rotation() const { return end_rotation_; }

void core::Operation::set_end_rotation(double end_rotation) { end_rotation_ = end_rotation; }

QVector3D core::Operation::end_position() const { return end_position_; }

void core::Operation::set_end_position(const QVector3D &end_position)
{
    end_position_ = end_position;
}

core::OperationState::State core::Operation::state() const { return state_; }

void core::Operation::set_state(OperationState::State state)
{
    state_ = state;
    emit stateChanged();
}

bool core::Operation::reached() const
{
    for (const auto pre : left) {
        if (pre->state() != OperationState::Closed) return false;
    }
    return true;
}

std::pair<benchmark::Entity, benchmark::Entity> core::Operation::to_world_frame(
    const benchmark::Entity &corner)
{
    const auto k = static_cast<benchmark::Entity::kind>(type_);
    const auto c = static_cast<benchmark::Entity::color>(color_);

    auto from_corner = [&corner, &k](const auto &t, const auto &r) {
        auto relative_pose =
            Eigen::Affine3d(Eigen::AngleAxisd(r - M_PI_2, Eigen::Vector3d::UnitZ()));
        relative_pose.translation() << t.x(), t.y(), t.z();// relative position
        relative_pose.translation().z() -=
            0.02;// corner in webview has origin 2cm lower in this context
        const auto cf = corner_frame(corner);
        return benchmark::Entity::pcl_T_web.at(k) * cf * relative_pose;
    };
    return {
        benchmark::Entity{ k, c, from_corner(start_position_, start_rotation_) },
        benchmark::Entity{ k, c, from_corner(end_position_, end_rotation_) },
    };
}


core::Operation::Operation(QObject *parent) : QObject(parent) {}
