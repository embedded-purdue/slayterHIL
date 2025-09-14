#include "physics_body.hpp"

physics_body::physics_body()
    :   mass(0.0),
    position(Eigen::Vector3d::Zero()),
    velocity(Eigen::Vector3d::Zero()),
    acceleration(Eigen::Vector3d::Zero()),
    total_force(Eigen::Vector3d::Zero()),
    orientation(Eigen::Quaterniond::Identity())
{}

physics_body::physics_body(double mass,
        const Eigen::Vector3d& position,
        const Eigen::Vector3d& velocity,
        const Eigen::Vector3d& acceleration,
        const Eigen::Quaterniond& orientation)
    :   mass(mass),
    position(position),
    velocity(velocity),
    acceleration(acceleration),
    total_force(Eigen::Vector3d::Zero()),
    orientation(orientation)
{}
