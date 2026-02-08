#include "velocityController.hpp"

velocityController::velocityController () : 
	kp(Eigen::Vector3d::Zero()), ki(Eigen::Vector3d::Zero()), kd(Eigen::Vector3d::Zero()),
	velocityIntegral(Eigen::Vector3d::Zero()),
	previousError(Eigen::Vector3d::Zero())
{}

velocityController::velocityController (const Eigen::Vector3d& p, const Eigen::Vector3d& i, const Eigen::Vector3d& d) :
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

	return kp.cwiseProduct(velocityError) + ki.cwiseProduct(velocityIntegral) + kd.cwiseProduct(velocityDerivative);

}

