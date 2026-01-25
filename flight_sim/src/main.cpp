/*
 * This is the CMAKE target file, delete later on after
 * changing target rules
 */
#define ENET_IMPLEMENTATION


#include <stdio.h>
#include <chrono>
#include <thread>
#include <iostream>
#include "physics/rigid_body.hpp"


int main() {

    // I pointer-ized everything from Sebastian's code

    // What unit is our mass measured in? Let it be grams for now lol
    RigidBody* drone = new RigidBody(5.0, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,0,200));

    // (happy birthday to the) Ground
    RigidBody* ground = new RigidBody();
    ground->setBounds(50, 50, 5); // Plane perpendicular to the z-axis with dimensions 100x100x10

    using clock = std::chrono::high_resolution_clock;
    using duration = std::chrono::duration<double>;

    // Is the time update necessarily in terms of a minute? Or does it work this way with chrono?
    const double DELTATIME = 1.0/60.0;
    auto previousTime = std::chrono::high_resolution_clock::now();

    // Commented out thread for now
    // ( was creating a "terminate called without an active exception error" )

    //bool exit = false;
    /*std::thread inputThread([&exit](){
      std::cin.get();
      exit = true;
      });*/

    // update loop
    // Current condition set to break when the drone collides with the ground
    while( !drone->isColliding(ground) ){

        auto currentTime = clock::now();
        duration elapsedTime = currentTime - previousTime;

        if(elapsedTime.count() >= DELTATIME){
            //TODO: logic and the actual physics
            drone->applyForce( Eigen::Vector3d( 0, 0, -9.81 * drone->getMass() ) );
            drone->update(DELTATIME);

            std::cout << "pos: (" << drone->position.x() << ", "
                << drone->position.y() << ", "
                << drone->position.z() << ")";

            std::cout << "\tvel: (" << drone->velocity.x() << ", "
                << drone->velocity.y() << ", "
                << drone->velocity.z() << ")";

            std::cout << "\tori: [" << drone->orientation.coeffs().transpose() << "]";

            std::cout << "\tcollision: " << drone->isColliding(ground); // Might print collision detected twice oops lol

            std::cout << std::endl;

            previousTime = currentTime;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));



    }



    // Delete RigidBody pointers
    delete drone;
    delete ground;

    return 0;
}

// Old Collision detection test code

/*RigidBody* body = new RigidBody();

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
    x_wall->setBounds(5, 50, 50); // Plane perpendicular to the x-axis with dimensions 10x100x100
    y_wall->setBounds(50, 5, 50); // Plane perpendicular to the y-axis with dimensions 100x10x100
    ground->setBounds(50, 50, 5); // Plane perpendicular to the z-axis with dimensions 100x100x10


    // Apply Movements for collision testing
    //applyGravity(body, ground);
    body->goToXWall(body, x_wall);
    //goToYWall(body, y_wall);*/
