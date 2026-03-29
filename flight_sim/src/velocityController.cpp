#include "velocityController.hpp"

velocityController::velocityController()
    : kp(Eigen::Vector3d::Zero()), ki(Eigen::Vector3d::Zero()),
      kd(Eigen::Vector3d::Zero()), velocityIntegral(Eigen::Vector3d::Zero()),
      previousError(Eigen::Vector3d::Zero()), maxForce(0) {}

velocityController::velocityController(const Eigen::Vector3d &p,
                                       const Eigen::Vector3d &i,
                                       const Eigen::Vector3d &d, double maxF)
    : kp(p), ki(i), kd(d), maxForce(maxF),
      velocityIntegral(Eigen::Vector3d::Zero()),
      previousError(Eigen::Vector3d::Zero()) {}

Eigen::Vector3d
velocityController::compute(const Eigen::Vector3d &currentVelocity,
                            const Eigen::Vector3d &targetVelocity, double dt) {
  velocityError = targetVelocity - currentVelocity;

  Eigen::Vector3d pTerm = kp.cwiseProduct(velocityError);
  Eigen::Vector3d dTerm = kd.cwiseProduct((velocityError - previousError) / dt);
  previousError = velocityError;

  Eigen::Vector3d rawOutput = pTerm + ki.cwiseProduct(velocityIntegral) + dTerm;

  if (rawOutput.norm() < maxForce) {
    velocityIntegral += velocityError * dt;
  }

  Eigen::Vector3d finalOutput =
      pTerm + ki.cwiseProduct(velocityIntegral) + dTerm;

  if (finalOutput.norm() > maxForce) {
    finalOutput = finalOutput.normalized() * maxForce;
  }

  return finalOutput;
}
