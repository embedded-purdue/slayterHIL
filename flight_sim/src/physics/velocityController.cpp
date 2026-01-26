#include "include/slayter/physics/velocityController.hpp"

velocityController::velocityController () : 
	kp(0.0), ki(0.0), kd(0.0),
	velocityIntegral(Eigen::Vector3d::Zero()),
	previousError(Eigen::Vector3d::Zero())
{}

velocityController::velocityController (double p, double i, double d) :
	kp(p), ki(i), kd(d),
	velocityIntegral(Eigen::Vector3d::Zero()),
	previousError(Eigen::Vector3d::Zero())
{}

Eigen::Vector3d velocityController::compute (const Eigen::Vector3d& currentVelocity, 
					     const Eigen::Vector3d& targetVelocity, 
					     double dt) {
	velocityError = targetVelocity - currentVelocity;
	velocityIntegral += velocityError * dt;
	Eigen::Vector3d velocityDerivative = (velocityError - previousError) /dt;
	previousError = velocityError;

	return kp * velocityError + ki * velocityIntegral + kd * velocityDerivative;

}

