// Drone Class (subset of collection, see collection.hpp)

#pragma once

#include <iostream>
#include "rigid_body.hpp"
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Geometry>

class Drone : public RigidBody {
public:
    Drone();
    Drone(
        RigidBody* body,
        RigidBody* m1,
        RigidBody* m2,
        RigidBody* m3,
        RigidBody* m4
    );

    ~Drone() {
        delete body;
        delete m1;
        delete m2;
        delete m3;
        delete m4;
    }

    void applyForce(const Eigen::Vector3d& force) override;
    void update(double dt) override;

    Eigen::Vector3d getPosition() const;
    Eigen::Vector3d getVelocity() const;

private:
    RigidBody* body;
    RigidBody* m1;
    RigidBody* m2;
    RigidBody* m3;
    RigidBody* m4;

    double calculateMass();
    Eigen::Vector3d calculateNetForce();
    Eigen::Vector3d calculateNetTorque();
    Eigen::Vector3d calculateAngularVelocity();
};
