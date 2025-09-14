#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>

class PhysicsBody {
public:
    double mass;
    Eigen::Vector3d position;
    Eigen::Vector3d velocity;
    Eigen::Vector3d acceleration;
    Eigen::Vector3d total_force;
    Eigen::Quaterniond orientation;

    /* CTORs */
    PhysicsBody() :
        mass(0.0),
        position(Eigen::Vector3d::Zero()),
        velocity(Eigen::Vector3d::Zero()),
        acceleration(Eigen::Vector3d::Zero()),
        total_force(Eigen::Vector3d::Zero()),
        orientation(Eigen::Quaterniond::Identity()) {}

    PhysicsBody(double mass,
                 const Eigen::Vector3d& position,
                 const Eigen::Vector3d& velocity,
                 const Eigen::Vector3d& acceleration,
                 const Eigen::Quaterniond& orientation) :
        mass(mass),
        position(position),
        velocity(velocity),
        acceleration(acceleration),
        total_force(Eigen::Vector3d::Zero()),
        orientation(orientation)
    {}

    /* Methods */
    virtual void applyForce(const Eigen::Vector3d& force) = 0; //probobally just implement when making drone object
    virtual void update(double dt) = 0;
    virtual ~PhysicsBody() = default;
};
