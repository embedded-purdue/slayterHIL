#pragma once

#include "physics_body.hpp"
#include <Eigen/Dense>
#include <Eigen/Geometry>

/*
  RigidBody implements a 6-DOF rigid body integrated with RK4:
   - mass (scalar)
   - inertia (3x3 body-frame inertia matrix)
   - forces are applied in WORLD frame (accumulated into total_force)
   - torques are applied in BODY frame (accumulated into total_torque)
   - angular velocity (omega) is stored in BODY frame
   - orientation is a quaternion mapping BODY -> WORLD
*/

struct RigidBodyDerivative {
    Eigen::Vector3d dPosition;        // world-frame velocity
    Eigen::Vector3d dVelocity;        // world-frame acceleration
    Eigen::Vector3d dAngularVelocity; // body-frame angular acceleration
    Eigen::Quaterniond dOrientation;  // quaternion derivative
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

    // RK4 integration step
    void update(double dt) override;

    // get/set angular velocity in body frame
    Eigen::Vector3d getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(const Eigen::Vector3d& w) { angularVelocity = w; }

    // inertia (body frame)
    Eigen::Matrix3d inertiaBody;    // I
    Eigen::Matrix3d inertiaBodyInv; // I^-1

    // accumulated torque in BODY frame
    Eigen::Vector3d total_torque_body;

    // angular velocity (body frame)
    Eigen::Vector3d angularVelocity;

    void setAccelerationWorld(const Eigen::Vector3d& a) { acceleration = a; }

private:
    Eigen::Vector3d computeLinearAcceleration() const;
    Eigen::Vector3d computeAngularAcceleration() const;

    // RK4 helpers
    RigidBodyDerivative computeDerivative() const;
    RigidBody integrateWithDerivative(const RigidBodyDerivative& deriv, double dt) const;
};
