#include "rigid_body.hpp"

// default ctor with unit mass and identity inertia
RigidBody::RigidBody() 
  : PhysicsBody(1.0,
		Eigen::Vector3d::Zero(),
		Eigen::Vector3d::Zero(),
		Eigen::Vector3d::Zero(),
		Eigen::Quaterniond::Identity()),
    inertiaBody(Eigen::Matrix3d::Identity()),
    inertiaBodyInv(Eigen::Matrix3d::Identity()),
    total_torque_body(Eigen::Vector3d::Zero()),
    angularVelocity(Eigen::Vector3d::Zero())
{}


RigidBody::RigidBody(double m,
                     const Eigen::Matrix3d& inertia,
                     const Eigen::Vector3d& initPos,
                     const Eigen::Quaterniond& initOri)
  : PhysicsBody(m,
		initPos,
		Eigen::Vector3d::Zero(),
		Eigen::Vector3d::Zero(),
		initOri.normalized()),
    inertiaBody(inertia),
    inertiaBodyInv(inertia.inverse()),
    total_torque_body(Eigen::Vector3d::Zero()),
    angularVelocity(Eigen::Vector3d::Zero())
{}


void RigidBody::applyForce(const Eigen::Vector3d& force) {
    total_force += force; // world frame
}

void RigidBody::applyTorque(const Eigen::Vector3d& torque) {
    total_torque_body += torque; // body frame
}

void RigidBody::clearAccumulators() {
    total_force.setZero();
    total_torque_body.setZero();
}

Eigen::Vector3d RigidBody::computeLinearAcceleration() const {
    return total_force / mass; // world frame
}

Eigen::Vector3d RigidBody::computeAngularAcceleration() const {
    Eigen::Vector3d omega = angularVelocity; // body frame
    Eigen::Vector3d Iomega = inertiaBody * omega;
    Eigen::Vector3d alpha = inertiaBodyInv * (total_torque_body - omega.cross(Iomega));
    return alpha; // body frame
}

RigidBodyDerivative RigidBody::computeDerivative() const {
    RigidBodyDerivative deriv;

    // translational
    deriv.dPosition = velocity;
    deriv.dVelocity = computeLinearAcceleration();

    // rotational
    deriv.dAngularVelocity = computeAngularAcceleration();

    // quaternion derivative
    Eigen::Quaterniond omega_q(0.0,
                               angularVelocity.x(),
                               angularVelocity.y(),
                               angularVelocity.z());
    Eigen::Quaterniond qDot = orientation * omega_q;
    qDot.coeffs() *= 0.5;
    deriv.dOrientation = qDot;

    return deriv;
}

RigidBody RigidBody::integrateWithDerivative(const RigidBodyDerivative& deriv, double dt) const {
    RigidBody result = *this;

    result.position += deriv.dPosition * dt;
    result.velocity += deriv.dVelocity * dt;
    result.angularVelocity += deriv.dAngularVelocity * dt;

    Eigen::Quaterniond qNew;
    qNew.coeffs() = result.orientation.coeffs() + deriv.dOrientation.coeffs() * dt;
    result.orientation = qNew.normalized();

    return result;
}

void RigidBody::update(double dt) {
    if (dt <= 0.0) return;

    // k1
    RigidBodyDerivative k1 = computeDerivative();

    // k2
    RigidBody state2 = integrateWithDerivative(k1, dt * 0.5);
    RigidBodyDerivative k2 = state2.computeDerivative();

    // k3
    RigidBody state3 = integrateWithDerivative(k2, dt * 0.5);
    RigidBodyDerivative k3 = state3.computeDerivative();

    // k4
    RigidBody state4 = integrateWithDerivative(k3, dt);
    RigidBodyDerivative k4 = state4.computeDerivative();

    // combine
    position += (dt / 6.0) * (k1.dPosition + 2.0*k2.dPosition + 2.0*k3.dPosition + k4.dPosition);
    velocity += (dt / 6.0) * (k1.dVelocity + 2.0*k2.dVelocity + 2.0*k3.dVelocity + k4.dVelocity);
    angularVelocity += (dt / 6.0) * (k1.dAngularVelocity + 2.0*k2.dAngularVelocity + 2.0*k3.dAngularVelocity + k4.dAngularVelocity);

    Eigen::Vector4d qInc =
        k1.dOrientation.coeffs() +
        2.0*k2.dOrientation.coeffs() +
        2.0*k3.dOrientation.coeffs() +
        k4.dOrientation.coeffs();
    qInc *= (dt / 6.0);

    Eigen::Quaterniond qNew;
    qNew.coeffs() = orientation.coeffs() + qInc;
    orientation = qNew.normalized();

    // clear accumulators for next step
    clearAccumulators();
}
