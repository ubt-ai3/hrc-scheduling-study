#include "util.h"

std::pair<QVector3D, double> core::util::TransformToVectorAndRotation(
    const Eigen::Affine3d &transform)
{
    auto angle = std::atan2(transform.matrix()(1, 0), transform.matrix()(0, 0));
    angle = std::fmod(angle + 2 * M_PI, 2 * M_PI);


    const auto &t = transform.translation();
    const auto vec = QVector3D(t.x(), t.y(), t.z());

    return { vec, angle };
}

Eigen::Affine3d core::util::VectorAndRotationToTransform(const QVector3D &vec, const double angle)
{
    auto traffo = Eigen::Affine3d(Eigen::AngleAxisd(angle, Eigen::Vector3d::UnitZ()));
    traffo.translation() << vec.x(), vec.y(), vec.z();
    return traffo;
}

Eigen::Affine3d core::util::from_y_up_to_z_up(const Eigen::Affine3d &t)
{
    const auto ea = t.rotation().eulerAngles(0, 1, 2);

    auto res = Eigen::Affine3d(Eigen::AngleAxisd(ea[1], Eigen::Vector3d::UnitZ()));
    res.translation() << t.translation().z(), t.translation().x(), t.translation().y();
    return res;
}