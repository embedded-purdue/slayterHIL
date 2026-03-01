#include "velocityController.hpp"

velocityController::velocityController () : 
<<<<<<< HEAD
	kp(Eigen::Vector3d::Zero()), ki(Eigen::Vector3d::Zero()), kd(Eigen::Vector3d::Zero()),
	velocityIntegral(Eigen::Vector3d::Zero()),
	previousError(Eigen::Vector3d::Zero()),
  maxForce(0)
{}

velocityController::velocityController (const Eigen::Vector3d& p, const Eigen::Vector3d& i, const Eigen::Vector3d& d, double maxF) :
	kp(p), ki(i), kd(d), maxForce(maxF),
=======
	kp(0.0), ki(0.0), kd(0.0),
	velocityIntegral(Eigen::Vector3d::Zero()),
	previousError(Eigen::Vector3d::Zero())
{}

velocityController::velocityController (double p, double i, double d) :
	kp(p), ki(i), kd(d),
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3
	velocityIntegral(Eigen::Vector3d::Zero()),
	previousError(Eigen::Vector3d::Zero())
{}

Eigen::Vector3d velocityController::compute (const Eigen::Vector3d& currentVelocity, 
					     const Eigen::Vector3d& targetVelocity, 
					     double dt) {
	velocityError = targetVelocity - currentVelocity;
<<<<<<< HEAD

  Eigen::Vector3d pTerm = kp.cwiseProduct(velocityError);
  Eigen::Vector3d dTerm = kd.cwiseProduct((velocityError - previousError) / dt);
  previousError = velocityError;

  Eigen::Vector3d rawOutput = pTerm + ki.cwiseProduct(velocityIntegral) + dTerm;

  if (rawOutput.norm() < maxForce) {
        velocityIntegral += velocityError * dt;
  }

  Eigen::Vector3d finalOutput = pTerm + ki.cwiseProduct(velocityIntegral) + dTerm;

  if (finalOutput.norm() > maxForce) {
        finalOutput = finalOutput.normalized() * maxForce;
  }

  return finalOutput;
=======
	velocityIntegral += velocityError * dt;
	Eigen::Vector3d velocityDerivative = (velocityError - previousError) /dt;
	previousError = velocityError;

	return kp * velocityError + ki * velocityIntegral + kd * velocityDerivative;

>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3
}

