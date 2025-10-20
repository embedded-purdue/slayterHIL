#include "PIDcalculator.hpp"

PIDcalculator::PIDcalculator () : 
	kp(0.0), ki(0.0), kd(0.0),
	integralError(Eigen::Vector3d::Zero()),
	prevError(Eigen::Vector3d::Zero())
{}

PIDcalculator::PIDcalculator (double p, double i, double d) : 
	kp(p), ki(i), kd(d),
	integralError(Eigen::Vector3d::Zero()),
	prevError(Eigen::Vector3d::Zero())
{}

void PIDcalculator::setTarget (const Eigen::Vector3d& targetPos) {
	desiredPos = targetPos; 
}

Eigen::Vector3d PIDcalculator::getTarget() {
	return desiredPos;
}

Eigen::Vector3d PIDcalculator::compute(const Eigen::Vector3d& currentPos, double dt) {
	Eigen::Vector3d error = desiredPos - currentPos;
	integralError += error * dt;
	Eigen::Vector3d derivative = (error - prevError) / dt;
	prevError = error;

	return kp * error + ki * integralError + kd * derivative;
}
