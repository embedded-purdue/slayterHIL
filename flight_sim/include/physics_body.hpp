#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>

class PhysicsBody {
    private:
    
        // Fields
        Eigen::Vector3d position;
        Eigen::Quaterniond orientation;
        double scale;
        int bounding_box[3]  = {0}; // List of distance lengths of bounding box corresponding to x, y, z
        std::string name;

    public:
        // Constructors

        // Default
        PhysicsBody() : position(Eigen::Vector3d::Zero()), orientation(Eigen::Quaterniond::Identity()), scale(1.0), name("Unnamed") {}

        // Params
        PhysicsBody(const Eigen::Vector3d& position, const Eigen::Quaterniond& orientation, double scale, const std::string& name) : 
            position(position), orientation(orientation), scale(scale), name(name) {}
};
