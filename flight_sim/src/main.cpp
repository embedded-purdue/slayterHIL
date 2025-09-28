#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "physics/rigid_body.hpp"

int main() {
	RigidBody drone(5.0, Eigen::Matrix3d::Identity(), Eigen::Vector3d(0,0,100));

	using clock = std::chrono::high_resolution_clock;
	using duration = std::chrono::duration<double>;
	
	const double DELTATIME = 1.0/60.0;	
	auto previousTime = std::chrono::high_resolution_clock::now(); 

	bool exit = false; 

	std::thread inputThread([&exit](){
		std:: cin.get();
		exit = true;
	});

	//update loop, use enter to exit 
	while(!exit){
	
		auto currentTime = clock::now();
		duration elapsedTime = currentTime - previousTime;
		
		if(elapsedTime.count() >= DELTATIME){
			//TODO: logic and the actual physics 
			drone.applyForce(Eigen::Vector3d(0,0, -9.81 * drone.mass));
			drone.update(DELTATIME);

			std::cout << "pos: (" << drone.position.x() << ", "
					      << drone.position.y() << ", "
					      << drone.position.z() << ")";

			std::cout << " vel: (" << drone.velocity.x() << ", "
					       << drone.velocity.y() << ", "
					       << drone.velocity.z() << ")";

			std::cout << " ori: [" << drone.orientation.coeffs().transpose() << "]";
			std::cout << std::endl;

			previousTime = currentTime;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

}
