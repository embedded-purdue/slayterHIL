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
        orientation(orientation) {}

    virtual void applyForce(const Eigen::Vector3d& force) = 0;
    virtual void update(double dt) = 0;
    virtual ~PhysicsBody() = default;

    double getMass() const { return mass; }
    Eigen::Vector3d getPosition() const { return position; }
    Eigen::Vector3d getVelocity() const { return velocity; }
    Eigen::Vector3d getAcceleration() const { return acceleration; }
    Eigen::Vector3d getNetForce() const { return total_force; }
    Eigen::Quaterniond getOrientation() const { return orientation; }

    void setPosition(const Eigen::Vector3d& vec) { position = vec; }
    void setVelocity(const Eigen::Vector3d& vec) { velocity = vec; }
    void setAcceleration(const Eigen::Vector3d& vec) { acceleration = vec; }
    void setOrientation(const Eigen::Quaterniond& qtn) { orientation = qtn; }
};
