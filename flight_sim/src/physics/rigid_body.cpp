#include "include/slayter/physics/rigid_body.hpp"

RigidBody::RigidBody()
  : PhysicsBody(
        1.0,                            // mass
        Eigen::Vector3d::Zero(),        // position
        Eigen::Vector3d::Zero(),        // velocity
        Eigen::Vector3d::Zero(),        // acceleration
        Eigen::Quaterniond::Identity()  // orientation
    ),
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

Eigen::Vector3d RigidBody::getPosition () const {
	return position; 
}

Eigen::Vector3d RigidBody::getVelocity() const {
	return velocity; 
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

// Collision Logic
bool RigidBody::isColliding(RigidBody* col_body) {
    
    // Implenting a quite crude solution right now
    // When rotations are involved, there needs to be a more elegant solution
    // Consider and oriented bounding box implementation using rotation matrices

    // Bounds sums for this object and passed in object
    double bound_sum_x = this->getXBound() + col_body->getXBound();
    double bound_sum_y = this->getYBound() + col_body->getYBound();
    double bound_sum_z = this->getZBound() + col_body->getZBound();

    // Distances between this and col_body
    double x_dist = this->position[0] - col_body->position[0];
    double y_dist = this->position[1] - col_body->position[1];
    double z_dist = this->position[2] - col_body->position[2];

    // If the distance between the bodies' positions are less than or equal to the sum of their bounds, return true
    // Otherwise, return false
    
    // DEBUG PRINT STATEMENTS

    /*std::cout << std::endl << std::endl;
    std::cout << "X Distance: " << this->position[0] - col_body->position[0] << "\nBound Sum " << bound_sum_x << std::endl;
    std::cout << "Y Distance: " << this->position[1] - col_body->position[1] << "\nBound Sum " << bound_sum_y << std::endl;
    std::cout << "Z Distance: " << this->position[2] - col_body->position[2] << "\nBound Sum " << bound_sum_z << std::endl;
    std::cout << std::endl << std::endl;*/
    
    // Collision check SHOULD have AND (&&)
    // 2 objects can have the same z value
    // BUT they can be leagues apart on the x-y plane
    bool collision = 
        (x_dist <= bound_sum_x) &&
        (y_dist <= bound_sum_y) &&
        (z_dist <= bound_sum_z);
    
    // Collision detected
    if(collision) std::cout << "Collision detected" << std::endl;

    return collision;
}

// Tester methods for collision logic
void RigidBody::applyGravity(RigidBody* body, RigidBody* ground) {
    std::cout << body->getPosition() << std::endl << std::endl;

    while ( !body->isColliding(ground) ) {
        body->position += body->velocity;
        body->velocity -= body->getGravityVector();
        std::cout << body->getPosition() << std::endl << std::endl;
    }

    std::cout << body->getPosition() << std::endl << std::endl;
    std::cout << "Fallen to the ground" << std::endl;
}

void RigidBody::goToXWall(RigidBody* body, RigidBody* x_wall) {
    std::cout << body->getPosition() << std::endl << std::endl;

    while ( !body->isColliding(x_wall) ) {
        body->position += body->velocity;
        body->velocity += Eigen::Vector3d(5,0,0);
        std::cout << body->getPosition() << std::endl << std::endl;
    }

    std::cout << body->getPosition() << std::endl << std::endl;
    std::cout << "Touched X Wall" << std::endl;
}

void RigidBody::goToYWall(RigidBody* body, RigidBody* y_wall) {
    std::cout << body->getPosition() << std::endl << std::endl;
    
    while ( !body->isColliding(y_wall) ) {
        
        // TODO: Replace raw vector math with "addForce() + update()" and fix the X-Y wall methods
        // (collision detection itself works fine)

        body->position += body->velocity;
        body->velocity += Eigen::Vector3d(0,5,0);
        std::cout << body->getPosition() << std::endl << std::endl;
    }

    std::cout << body->getPosition() << std::endl << std::endl;
    std::cout << "Touched Y Wall" << std::endl;
}
