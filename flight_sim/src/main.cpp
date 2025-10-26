#include <stdio.h>
#include <chrono>
#include <thread>
#include <iostream>
#include "physics/rigid_body.hpp"
#include "physics/drone.hpp"
#include "physics/PIDcalculator.hpp"

int main() {
    
    // I pointer-ized everything from Sebastian's code

    // What unit is our mass measured in? Let it be grams for now lol
    //RigidBody* drone = new RigidBody(5.0, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,0,200));


	Drone* drone = new Drone(
    		new RigidBody(1.0, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,0,0)), // body
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(1,0,0)), //motors
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(-1,0,0)), 
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,1,0)), 
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,-1,0))  
	);

    // (happy birthday to the) Ground
    RigidBody* ground = new RigidBody();
    ground->setBounds(50, 50, 5); // Plane perpendicular to the z-axis with dimensions 100x100x10

	using clock = std::chrono::high_resolution_clock;
	using duration = std::chrono::duration<double>;
	
    // Is the time update necessarily in terms of a minute? Or does it work this way with chrono? its in seconds.
	const double DELTATIME = 1.0/60.0; 
	const double SIM_TIME = 10.0;
	double elapsedTime = 0.0;
	auto previousTime = std::chrono::high_resolution_clock::now(); 

	PIDcalculator* control = new PIDcalculator (0.1, 0, 0.05);
	Eigen::Vector3d Target(10.0, 10.0, 15.0);
	control -> setTarget (Target);
	std::cout << control -> getTarget() << std::endl << drone->getPosition();
    // Commented out thread for now
    // ( was creating a "terminate called without an active exception error" )

	//bool exit = false; 
    /*std::thread inputThread([&exit](){
		std::cin.get();
		exit = true;
	});*/

	// update loop
    // Current condition set to break when the drone collides with the ground 
//	while( !drone->isColliding(ground) ){ got rid of collisions for now 
	while (elapsedTime < SIM_TIME) {
		auto currentTime = clock::now();
		duration dt = currentTime - previousTime;
		
		if(dt.count() >= DELTATIME){
			//TODO: logic and the actual physics 
			drone->applyForce( Eigen::Vector3d( 0, 0, -9.81 * drone->getMass() ) );
			Eigen::Vector3d force = control->compute(drone->getPosition(), DELTATIME);
			drone->applyForce(force);
			drone->update(DELTATIME);
			
			std::cout << "pos: (" << drone->getPosition().transpose() << ")";

			std::cout << "\tvel: (" << drone->getVelocity().transpose() << ")";

			std::cout << "\tori: [" << drone->orientation.coeffs().transpose() << "]";

			std::cout << "error: " << (control->getTarget() - drone->getPosition()).transpose() << std::endl;

			previousTime = currentTime;
			elapsedTime += DELTATIME;
           /* std::cout << "\tcollision: " << drone->isColliding(ground); // Might print collision detected twice oops lol

			std::cout << std::endl;

			previousTime = currentTime; */
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));


        
	}



    // Delete RigidBody pointers
    delete drone;
    delete ground;
	delete control;

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
