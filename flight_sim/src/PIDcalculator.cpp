#include "PIDcalculator.hpp"

PIDcalculator::PIDcalculator () : 
	kp(0.0), ki(0.0), kd(0.0),
	integralError(Eigen::Vector3d::Zero()),
	prevError(Eigen::Vector3d::Zero()),
	prevPos(Eigen::Vector3d::Zero()),
	error(Eigen::Vector3d::Zero())
{}

PIDcalculator::PIDcalculator (double p, double i, double d) : 
	kp(p), ki(i), kd(d),
	integralError(Eigen::Vector3d::Zero()),
	prevError(Eigen::Vector3d::Zero()),
	prevPos(Eigen::Vector3d::Zero()),
	error(Eigen::Vector3d::Zero())
{}

void PIDcalculator::setTarget (const Eigen::Vector3d& targetPos) {
	desiredPos = targetPos; 
}

Eigen::Vector3d PIDcalculator::getTarget() {
	return desiredPos;
}

Eigen::Vector3d PIDcalculator::getError() {
	return error;
}

Eigen::Vector3d PIDcalculator::compute(const Eigen::Vector3d& currentPos, double dt) {
	error = desiredPos - currentPos;
	integralError += error * dt;
//	velocity_error = error - currentVel;
//	accel_target = kp * velocity_error + 
//	Eigen::Vector3d derivative = (error - prevError) / dt;
	Eigen::Vector3d currentVelocity = (currentPos - prevPos) / dt; 
	Eigen::Vector3d derivative = -currentVelocity;
	prevError = error;
	prevPos = currentPos; 

	return kp * error + ki * integralError + kd * derivative;
}

//Eigen::Vector3d PIDcalculator::computeVelocity(const Eigen::vector3d& currentPos, double dt) {
	
//}
