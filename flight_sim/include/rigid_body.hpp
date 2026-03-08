#pragma once

#include <iostream>
#include "physics_body.hpp"
#include <Eigen/Dense>
#include <Eigen/Geometry>

class RigidBody : public PhysicsBody {
    private:

        // Fields
        double mass;
        Eigen::Matrix3d inertiaTensor;
        Eigen::Vector3d velocity;
        Eigen::Vector3d ang_velocity;
        Eigen::Vector3d acceleration;
        Eigen::Vector3d ang_acceleration;

    public:
        // Constructors

        // Default
        RigidBody() : PhysicsBody(), 
            mass(1.0), 
            inertiaTensor(Eigen::Matrix3d::Identity()), 
            velocity(Eigen::Vector3d::Zero()), 
            ang_velocity(Eigen::Vector3d::Zero()), 
            acceleration(Eigen::Vector3d::Zero()), 
            ang_acceleration(Eigen::Vector3d::Zero()) {}

        // Params
        RigidBody(
            const Eigen::Vector3d& position, 
            const Eigen::Quaterniond& orientation,
            double scale,
            const std::string& name,
            double mass,
            const Eigen::Matrix3d& inertiaTensor) :
            
            PhysicsBody(position, orientation, scale, name), 
            mass(mass), 
            inertiaTensor(inertiaTensor), 
            velocity(Eigen::Vector3d::Zero()), 
            ang_velocity(Eigen::Vector3d::Zero()), 
            acceleration(Eigen::Vector3d::Zero()), 
            ang_acceleration(Eigen::Vector3d::Zero()) {}


};
