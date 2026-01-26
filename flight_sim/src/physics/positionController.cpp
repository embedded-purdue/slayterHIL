#include "include/slayter/physics/positionController.hpp" 

positionController::positionController () : 
	kp(0.0),
	desiredPos(Eigen::Vector3d::Zero())
{}

positionController::positionController (double p) :
	kp(p),
	desiredPos(Eigen::Vector3d::Zero())
{}

void positionController::setTarget (const Eigen::Vector3d& target) {
	desiredPos = target; 
}

Eigen::Vector3d positionController::getTarget() {
	return desiredPos;
}

Eigen::Vector3d positionController::compute(const Eigen::Vector3d& currentPos, double dt) {
	Eigen::Vector3d pos_error = desiredPos - currentPos;
	return kp * pos_error;
}
