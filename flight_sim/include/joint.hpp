#pragma once

#include <iostream>
#include "physics_body.hpp"

class Joint : PhysicsBody {

    private:

        // Fields
        PhysicsBody* b1;
        PhysicsBody* b2;
    public:
        // Constructors
        Joint(PhysicsBody* body1, PhysicsBody* body2) : PhysicsBody(), b1(body1), b2(body2) {}

};