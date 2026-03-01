#include "positionController.hpp" 

positionController::positionController () : 
<<<<<<< HEAD
	kp(Eigen::Vector3d::Zero()),
	desiredPos(Eigen::Vector3d::Zero())
{}

positionController::positionController (const Eigen::Vector3d& p) :
=======
	kp(0.0),
	desiredPos(Eigen::Vector3d::Zero())
{}

positionController::positionController (double p) :
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3
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
<<<<<<< HEAD
	return kp.cwiseProduct(pos_error);
=======
	return kp * pos_error;
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3
}
