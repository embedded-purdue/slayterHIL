#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "physics/rigid_body.hpp"

int main() {
	RigidBody drone;

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
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime);
			std::cout << "update " << ms.count() << std::endl;

			previousTime = currentTime;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

}
