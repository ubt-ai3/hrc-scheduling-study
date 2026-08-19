#pragma once

#include <QVector3D>

#include <nlohmann/json.hpp>
#include <Eigen/Dense>


namespace core::util {
/// Given a map from keys to values, creates a new map from values to keys
template<typename K, typename V> static std::map<V, K> reverse_map(const std::map<K, V> &m)
{
    std::map<V, K> r;
    for (const auto &kv : m) {
        r[kv.second] = kv.first;
    }
    return r;
}

std::pair<QVector3D, double> TransformToVectorAndRotation(const Eigen::Affine3d &transform);
Eigen::Affine3d VectorAndRotationToTransform(const QVector3D &vec, const double angle);


// parse Poses from the AI3 Benchmark Builder Website into our robot coordinate system
Eigen::Affine3d from_y_up_to_z_up(const Eigen::Affine3d &t);

}// namespace core::util


namespace nlohmann {
/// <summary>
/// Serializer and deserializer for Eigen::Affine3d
/// </summary>
template<> struct adl_serializer<Eigen::Affine3d>
{
    static void to_json(json &j, const Eigen::Affine3d &value)
    {
        j = nlohmann::json::array();
        for (const auto &v : value.matrix().reshaped()) {
            j.push_back(v);
        }
    }

    static void from_json(const json &j, Eigen::Affine3d &value)
    {
        std::vector<double> data;
        for (const auto &v : j) {
            data.push_back(v);
        }
        value.matrix() = Eigen::Map<Eigen::Matrix4d>(data.data());
    }
};
}// namespace nlohmann
