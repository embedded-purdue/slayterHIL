#include <stdio.h>
#include <iostream>
#include "physics/rigid_body.hpp"


int main() {
    RigidBody* body = new RigidBody();
    
    // Set the body to hover
    body->position = Eigen::Vector3d(0, 0, 200.0);

    // Body dimensions
    body->setBounds(10,10,10);

    // Create env bodies
    RigidBody* x_wall = new RigidBody();
    RigidBody* y_wall = new RigidBody();
    RigidBody* ground = new RigidBody();

    // Wall Positions
    x_wall->position = Eigen::Vector3d(300.0, 0, 0);
    y_wall->position = Eigen::Vector3d(0, 300.0, 0);

    // Wall and Ground Bounds
    x_wall->setBounds(10,100,100); // Plane perpendicular to the x-axis with dimensions 10x100x100
    y_wall->setBounds(100,10,100); // Plane perpendicular to the y-axis with dimensions 100x10x100
    ground->setBounds(100,100,10); // Plane perpendicular to the z-axis with dimensions 100x100x10


    // Apply Movements for collision testing
    //applyGravity(body, ground);
    body->goToXWall(body, x_wall);
    //goToYWall(body, y_wall);

}
