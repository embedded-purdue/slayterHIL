// Generalized Collection Class, for future use/reference
// Drone does NOT inherit from this, consider it a specific subset than a child class.

#pragma once

#include <iostream>
#include "rigid_body.hpp"
#include "joint.hpp"
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <vector>


class Collection : RigidBody {

    private:

        // Fields
        std::vector<PhysicsBody*> bodies;
        std::vector<Joint*> joints;

    public:
        // Constructors
        Collection(std::vector<PhysicsBody*> bodies) : RigidBody(), bodies(bodies) {}

};