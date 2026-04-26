#pragma once

#include <iostream>
#include "physics_body.hpp"
#include <Eigen/Dense>
#include <Eigen/Geometry>

struct RigidBodyDerivative {
    Eigen::Vector3d dPosition;
    Eigen::Vector3d dVelocity;
    Eigen::Vector3d dAngularVelocity;
    Eigen::Quaterniond dOrientation;
};

class RigidBody : public PhysicsBody {
public:
    RigidBody();
    RigidBody(double mass,
              const Eigen::Matrix3d& inertiaBody,
              const Eigen::Vector3d& initPos = Eigen::Vector3d::Zero(),
              const Eigen::Quaterniond& initOri = Eigen::Quaterniond::Identity());

    void applyForce(const Eigen::Vector3d& force) override;
    void applyTorque(const Eigen::Vector3d& torque);
    void clearAccumulators();
    void update(double dt) override;

    Eigen::Vector3d getPosition() const;
    Eigen::Vector3d getVelocity() const;
    Eigen::Vector3d getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(const Eigen::Vector3d& w) { angularVelocity = w; }
    void setAccelerationWorld(const Eigen::Vector3d& a) { acceleration = a; }

    Eigen::Vector3d getNetTorque() const { return total_torque_body; }
    void setNetTorque(const Eigen::Vector3d& t) { total_torque_body = t; }

    double getXBound() const { return x_bound; }
    double getYBound() const { return y_bound; }
    double getZBound() const { return z_bound; }
    void setBounds(double x, double y, double z) { x_bound = x; y_bound = y; z_bound = z; }
    Eigen::Vector3d getGravityVector() const { return GRAV; }

    bool isColliding(RigidBody* col_body);
    void applyGravity(RigidBody* body, RigidBody* ground);
    void goToXWall(RigidBody* body, RigidBody* x_wall);
    void goToYWall(RigidBody* body, RigidBody* y_wall);

    // Public for RK4 intermediate states
    Eigen::Matrix3d inertiaBody;
    Eigen::Matrix3d inertiaBodyInv;
    Eigen::Vector3d total_torque_body;
    Eigen::Vector3d angularVelocity;

private:
    double x_bound = 0, y_bound = 0, z_bound = 0;
    const Eigen::Vector3d GRAV = Eigen::Vector3d(0.0, 0.0, 9.81);

    Eigen::Vector3d computeLinearAcceleration() const;
    Eigen::Vector3d computeAngularAcceleration() const;
    RigidBodyDerivative computeDerivative() const;
    RigidBody integrateWithDerivative(const RigidBodyDerivative& deriv, double dt) const;
};
