#include <stdio.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include "physics/rigid_body.hpp"
#include "physics/drone.hpp"
#include "physics/PIDcalculator.hpp"
#include "physics/positionController.hpp"
#include "physics/velocityController.hpp"

void initializeLogging();
void logData(double t, const Eigen::Vector3d& desiredPos, 
             const Eigen::Vector3d& currentPos, const Eigen::Vector3d& error, 
             const Eigen::Vector3d& controlOutput);
void closeLogging();

std::ofstream logFile;

int main() {
    
	Drone* drone = new Drone(
    		new RigidBody(1.0, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,0,0)), // body
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(1,0,0)), //motors
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(-1,0,0)), 
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,1,0)), 
    		new RigidBody(0.5, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,-1,0))  
	);

    RigidBody* ground = new RigidBody();
    ground->setBounds(50, 50, 5); // Plane perpendicular to the z-axis with dimensions 100x100x10

	using clock = std::chrono::high_resolution_clock;
	using duration = std::chrono::duration<double>;

  //Time is in seconds. 
	const double DELTATIME = 1.0/60.0; 
	const double SIM_TIME = 30.0;
	double elapsedTime = 0.0;
	auto previousTime = std::chrono::high_resolution_clock::now(); 

  Eigen::Vector3d posGains(0.5, 0.5, 2.0);
	positionController* positionControl = new positionController(posGains);
  Eigen::Vector3d vKp(1.0, 1.0, 3.0);
  Eigen::Vector3d vKi(0.1, 0.1, 0.5);
  Eigen::Vector3d vKd(0.05, 0.05, 0.2);
	velocityController* velocityControl = new velocityController(vKp, vKi, vKd);
	Eigen::Vector3d Target(100.0, 100.0, 100.0);
	positionControl -> setTarget (Target);
	
    // Current condition set to break when the drone collides with the ground 
//	while( !drone->isColliding(ground) ){ got rid of collisions for now 
	initializeLogging();
	while (elapsedTime < SIM_TIME) {
		auto currentTime = clock::now();
		duration dt = currentTime - previousTime;
		
		if(dt.count() >= DELTATIME){
			//TODO: logic and the actual physics 
			drone->applyForce( Eigen::Vector3d( 0, 0, -9.81 * drone->getMass() ) );
			//Eigen::Vector3d force = control->compute(drone->getPosition(), DELTATIME);
			Eigen::Vector3d targetVelocity = positionControl->compute(drone->getPosition(), DELTATIME);
			Eigen::Vector3d force = velocityControl->compute(drone->getVelocity(), targetVelocity, DELTATIME);
			force *= drone->getMass();
			force.z() += drone->getMass() * 9.81;
			drone->applyForce(force);
			drone->update(DELTATIME);
			
			std::cout << "pos: (" << drone->getPosition().transpose() << ")";

			std::cout << "\tvel: (" << drone->getVelocity().transpose() << ")";

			std::cout << "\tori: [" << drone->orientation.coeffs().transpose() << "]";

			std::cout << "error: " << (positionControl->getTarget() - drone->getPosition()).transpose();

			std::cout << "\ttimestep: " << elapsedTime << std::endl; 

      Eigen::Vector3d posError = positionControl->getTarget() - drone->getPosition();
      logData(elapsedTime, positionControl->getTarget(), drone->getPosition(), posError, force);
			previousTime = currentTime;
			elapsedTime += DELTATIME;

		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));


        
	}



    // Delete RigidBody pointers
    delete drone;
    delete ground;
    delete positionControl;
    delete velocityControl;
    closeLogging(); 


    return 0;
}


	void initializeLogging() {
		logFile.open("pid_tuning.csv");
	}

  void logData(double t, const Eigen::Vector3d& desiredPos, const Eigen::Vector3d& currentPos,
              const Eigen::Vector3d& error, const Eigen::Vector3d& controlOutput) {
      logFile << t << ","
              << currentPos.x() << "," << currentPos.y() << "," << currentPos.z() << ","
              << error.z() << "," << controlOutput.z() << "\n";
  }
	void closeLogging(){
		logFile.close();
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
