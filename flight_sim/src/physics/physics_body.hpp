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

    // Getters
    double getMass() const { return this->mass; }
    Eigen::Vector3d getPosition() const { return this->position; }
    Eigen::Vector3d getVelocity() const { return this->velocity; }
    Eigen::Vector3d getAcceleration() const { return this->acceleration; }
    Eigen::Vector3d getNetForce() const { return this->total_force; }
    Eigen::Quaterniond getOrientation() const { return this->orientation; }

    // Setters
    void setPosition(Eigen::Vector3d vec) { this->position = vec; }
    void setVelocity(Eigen::Vector3d vec) { this->velocity = vec; } 
    void setAcceleration(Eigen::Vector3d vec) { this->acceleration = vec; } 
    //void setNetForce(Eigen::Vector3d vec) { this->total_force = vec; } // Commenting this for now; wouldn't this be calculated?
    void setOrientation(Eigen::Quaterniond qtn) { this->orientation = qtn; } 
};
