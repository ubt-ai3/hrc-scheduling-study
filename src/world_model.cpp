#include "world_model.h"

#include <sstream>

void world_model::update(const std::vector<benchmark::Entity> &seen_entities)
{
    for (const auto &new_entity : seen_entities) {
        if (const auto old_entity = find_closest_entity(new_entity, m_part_equality_eps)) {
            auto entry(std::find(m_entities.begin(), m_entities.end(), *old_entity));
            *entry = weighted_average_pose(*old_entity, new_entity);
        } else {
            m_entities.push_back(new_entity);
        }
    }
}

const std::vector<benchmark::Entity> &world_model::entities() const { return m_entities; }

std::string world_model::serialize() const
{
    std::stringstream world_model_string;

    world_model_string << "[" << std::endl;

    std::vector<std::pair<int, benchmark::Entity>> entities_to_serialize;
    for (int i = 0; i < m_entities.size(); i++) {
        if (m_entities[i].kind_ == benchmark::Entity::kind::circuit_connector
            || m_entities[i].kind_ == benchmark::Entity::kind::corner_marker
            || m_entities[i].kind_ == benchmark::Entity::kind::gear_slot) {
            continue;
        }
        entities_to_serialize.emplace_back(i, m_entities[i]);
    }

    for (int i = 0; i < entities_to_serialize.size(); i++) {
        auto item = entities_to_serialize[i];

        world_model_string << "{";

        world_model_string << "\"id\": " << item.first << ",";

        world_model_string << "\"kind\": ";
        switch (item.second.kind_) {
        case benchmark::Entity::kind::gear:
            world_model_string << "\"gear\"";
            break;
        case benchmark::Entity::kind::conductor:
            world_model_string << "\"conductor\"";
            break;
        default:
            throw std::logic_error("Invalid entity type!");
        }
        world_model_string << ",";

        world_model_string << "\"color\": ";
        switch (item.second.color_) {
        case benchmark::Entity::color::orange:
            world_model_string << "\"orange\"";
            break;
        case benchmark::Entity::color::green:
            world_model_string << "\"green\"";
            break;
        case benchmark::Entity::color::blue:
            world_model_string << "\"blue\"";
            break;
        default:
            throw std::logic_error("Invalid entity color!");
        }
        world_model_string << ",";

        world_model_string << "\"position\": ";
        world_model_string << "[";
        world_model_string << item.second.pose_.translation().x();
        world_model_string << ", ";
        world_model_string << item.second.pose_.translation().y();
        world_model_string << "]";

        world_model_string << "}";

        if (i != entities_to_serialize.size() - 1) {
            world_model_string << ",";
        }

        world_model_string << std::endl;
    }

    world_model_string << "]";

    return world_model_string.str();
}

std::optional<benchmark::Entity> world_model::find_closest_entity(const benchmark::Entity &entity,
    double distance_cutoff) const
{
    std::optional<benchmark::Entity> result;
    double min_distance = std::numeric_limits<double>::max();

    for (const auto &world_e : m_entities) {
        if (world_e.color_ == entity.color_ && world_e.kind_ == entity.kind_) {
            const auto distance = (world_e.pose_.translation() - entity.pose_.translation()).norm();

            if (distance < distance_cutoff && distance < min_distance) {
                result = world_e;
                min_distance = distance;
            }
        }
    }

    return result;
}

double world_model::avg_angle(const benchmark::Entity &old_e, const benchmark::Entity &new_e)
{
    const auto old_z = old_e.angle_z();
    const auto new_z = new_e.angle_z();

    // Difference in angles counterclockwise, [0, 360[
    const auto diff = std::fmod(new_z - old_z + 2.0 * M_PI, 2 * M_PI);

    if (diff < M_PI) {
        return std::fmod(old_z + 0.5 * diff, 2.0 * M_PI);
    }
    return std::fmod(new_z + 0.5 * (2.0 * M_PI - diff), 2.0 * M_PI);
}
double world_model::avg_conductor_angle(const benchmark::Entity &old_e,
    const benchmark::Entity &new_e)
{
    const auto old_z = std::fmod(old_e.angle_z(), M_PI);
    const auto new_z = std::fmod(new_e.angle_z(), M_PI);

    // Difference in angles counterclockwise, [0, 360[
    const auto diff = std::fmod(new_z - old_z + M_PI, M_PI);

    if (diff < M_PI / 2.0) {
        return std::fmod(old_z + 0.5 * diff, M_PI);
    }
    return std::fmod(new_z + 0.5 * (M_PI - diff), M_PI);
}

benchmark::Entity world_model::weighted_average_pose(const benchmark::Entity &old_e,
    const benchmark::Entity &new_e) const
{
    assert(old_e.kind_ == new_e.kind_);
    assert(old_e.color_ == new_e.color_);

    double new_angle_z = 0;
    if (old_e.kind_ == benchmark::Entity::kind::conductor) {
        new_angle_z = avg_conductor_angle(old_e, new_e);
    } else {
        new_angle_z = avg_angle(old_e, new_e);
    }

    const auto dir = new_e.pose_.translation() - old_e.pose_.translation();
    const Eigen::Vector3d new_translation = old_e.pose_.translation() + 0.5 * dir;

    return {old_e.kind_, old_e.color_, new_translation, new_angle_z};
}