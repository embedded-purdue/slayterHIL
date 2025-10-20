#pragma once 

#include <Eigen/Dense>
#include <Eigen/Geometry>

class PIDcalculator {
public:
	PIDcalculator();
	PIDcalculator(double p, double i, double d);

	Eigen::Vector3d compute(const Eigen::Vector3d& currentPos, double dt);
	void setTarget(const Eigen::Vector3d& targetPos);  
	Eigen::Vector3d getTarget(); 
private: 
	double kp, ki, kd; 
	Eigen::Vector3d desiredPos; 
	Eigen::Vector3d integralError;
	Eigen::Vector3d prevError;

		
}; 
