#include "drone.hpp";

// So far I could only think of these 2 types of constructors as practical.
// If there is another type of constructor needed with different parameters,
// I'll be sure to implement it.

Drone::Drone() : RigidBody::RigidBody() {
    
    // TO DO: Come up with reasonable positional offsets for the drone parts 
    // e.i. body at (0,0,0) or even (0,0,-3) or something, we decide
    
    this->body = new RigidBody();
    this->m1 = new RigidBody();
    this->m2 = new RigidBody();
    this->m3 = new RigidBody();
    this->m4 = new RigidBody();

    // No need to calculate or set other values because everything is zeroed out (I think)
}

Drone::Drone (
        RigidBody* body,
        RigidBody* m1,
        RigidBody* m2,
        RigidBody* m3,
        RigidBody* m4
)   {
    
    // Set fields
    this->body = body;
    this->m1 = m1;
    this->m2 = m2;
    this->m3 = m3;
    this->m4 = m4;

    // Mass
    this->mass = calculateMass();
    
    // Intertia Bodies (right now PARTICULAR to drone)
    this->inertiaBody = Eigen::Matrix3d::Identity();
    this->inertiaBodyInv = inertiaBody.inverse();
    
    // Position, velocity, and acceleration (RELATIVE to the env)
    this->position.setZero();
    this->velocity.setZero();
    this->acceleration.setZero();

    // Force
    this->total_force = calculateNetForce();

    // Torque
    this->total_torque_body = calculateNetTorque();
    
    // Orientation (PARTICULAR to drone and RELATIVE to env)
    this->orientation = Eigen::Quaterniond::Identity();

    // Angular Velocity
    this->angularVelocity = calculateAngularVelocity();

}

// Helpers

double Drone::calculateMass() {
    return this->body->getMass() + 
           this->m1->getMass() + 
           this->m2->getMass() + 
           this->m3->getMass() + 
           this->m4->getMass();
}

// Faith in Eigen's Vector Addition
Eigen::Vector3d Drone::calculateNetForce() {
    return this->body->getNetForce() +
           this->m1->getNetForce() +
           this->m2->getNetForce() +
           this->m3->getNetForce() +
           this->m4->getNetForce();
}

Eigen::Vector3d Drone::calculateNetTorque() {
    return this->body->getNetTorque() +
           this->m1->getNetTorque() +
           this->m2->getNetTorque() +
           this->m3->getNetTorque() +
           this->m4->getNetTorque();
}

Eigen::Vector3d Drone::calculateAngularVelocity() {
    return this->body->getAngularVelocity() +
           this->m1->getAngularVelocity() +
           this->m2->getAngularVelocity() +
           this->m3->getAngularVelocity() +
           this->m4->getAngularVelocity();
}


// Update

void Drone::update(double dt) {
    
    // Calls parents update method
    RigidBody::update(dt);

    // Drone updates

    // (Mass stays constant so no need for dynamic updates)

    // Net Force
    this->total_force = calculateNetForce();

    // Net Torque
    this->total_torque_body = calculateNetTorque();

    // Angular Velocity Vector
    this->angularVelocity = calculateAngularVelocity();
    
}