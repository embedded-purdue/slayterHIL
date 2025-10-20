#pragma once

#include <iostream>
#include "rigid_body.hpp"

/*
  This class is a wrapper for 5 RigidBodies:
  - 4x Drone Blades
  - The drone body itself

  This will be a system representation of all of these rigid bodies combined.
*/

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

	Eigen::Vector3d getPosition() const;
    // Destructor
    ~Drone() {
        delete body;
        delete m1;
        delete m2;
        delete m3;
        delete m4;
    }

    // Position will work hierarchically, like it does in Godot
    // The position of each child body will be relative to its parent body
    // Therefore, drone will have its own separate position (as well as velocity and acceleration*)
    
    
    // *Velocity and acceleration are also separate because of internal net forces and whatnot

    void update(double dt);

private:
    
    // Main drone body
    RigidBody* body;

    // Motors 1-4
    RigidBody* m1;
    RigidBody* m2;
    RigidBody* m3;
    RigidBody* m4;

    // Helpers (for Drone characteristic fields that depend on the RigidBody drone parts)
    double calculateMass();
    Eigen::Vector3d calculateNetForce();
    Eigen::Vector3d calculateNetTorque();
    Eigen::Vector3d calculateAngularVelocity();

};
