// Drone Class (subset of collection, see collection.hpp)

#pragma once

#include <iostream>
#include "rigid_body.hpp"
#include "joint.hpp"
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Geometry>

class Drone : RigidBody {

    private:

        // Fields (mx = motor x)
        RigidBody* body;
        RigidBody* m1;
        RigidBody* m2;
        RigidBody* m3;
        RigidBody* m4;

        // Joints (for each motor to body)
        Joint* j1;
        Joint* j2;
        Joint* j3;
        Joint* j4;

    public:
        // Constructors
        Drone() {
            m1 = new RigidBody();
            m2 = new RigidBody();
            m3 = new RigidBody();
            m4 = new RigidBody();
            body = new RigidBody();

            j1 = new Joint(body, m1);
            j2 = new Joint(body, m2);
            j3 = new Joint(body, m3);
            j4 = new Joint(body, m4);
        }

};