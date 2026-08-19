#include <string>
#include <vector>

#include "vision.h"

/**
 * @brief Maintains a persistent map of detected assembly parts across multiple camera views.
 *
 * Each time new detections arrive (via update()), the model either merges them
 * with an existing entry of the same kind and color (if the translational distance
 * is within m_part_equality_eps) or inserts them as new entries.  Merging uses a
 * weighted average of positions and a shortest-path average of orientations, which
 * smooths out per-frame detection noise.
 *
 * serialize() produces a JSON array suitable for passing to the Python scheduler.
 */
class world_model
{
  public:
    /**
     * @param part_equality_eps  Maximum distance (metres) between two detections of the
     *                           same kind/color that are considered the same physical part.
     */
    explicit world_model(double part_equality_eps) noexcept : m_part_equality_eps(part_equality_eps) {}

    /**
     * @brief Integrates a new set of detections into the world model.
     *
     * Matches each entity in @p seen_entities to the closest existing entity of the
     * same kind and color within m_part_equality_eps; merges if found, inserts otherwise.
     */
    void update(const std::vector<benchmark::Entity> &seen_entities);

    /** @brief Returns all currently known entities. */
    [[nodiscard]] const std::vector<benchmark::Entity> &entities() const;

    /**
     * @brief Serializes the world model to a JSON string for the Python scheduler.
     *
     * Only gear and conductor entities are included; baseplates, corner markers,
     * circuit connectors and gear slots are omitted.
     */
    [[nodiscard]] std::string serialize() const;

    /**
     * @brief Finds the closest stored entity matching @p entity in kind, color, and position.
     * @param distance_cutoff  Maximum allowed translational distance (metres).
     * @return The closest matching entity, or nullopt if none is within the cutoff.
     */
    [[nodiscard]] std::optional<benchmark::Entity> find_closest_entity(const benchmark::Entity &entity,
        double distance_cutoff) const;

  private:
    const double m_part_equality_eps;

    std::vector<benchmark::Entity> m_entities;

    /** @brief Computes the shortest-path average of two Z-rotation angles in [0, 2*pi). */
    [[nodiscard]] static double avg_angle(const benchmark::Entity &old_e, const benchmark::Entity &new_e);
    /** @brief Computes the shortest-path average of two conductor orientations in [0, pi) (180-degree symmetry). */
    [[nodiscard]] static double avg_conductor_angle(const benchmark::Entity &old_e,
        const benchmark::Entity &new_e);
    /** @brief Returns an entity at the midpoint position and averaged orientation of @p old_e and @p new_e. */
    [[nodiscard]] benchmark::Entity weighted_average_pose(const benchmark::Entity &old_e,
        const benchmark::Entity &new_e) const;
};