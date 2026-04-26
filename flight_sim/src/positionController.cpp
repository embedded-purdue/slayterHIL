#include <flight_sim.hpp>

positionController::positionController() :
    kp(Eigen::Vector3d::Zero()),
    desiredPos(Eigen::Vector3d::Zero())
{}

positionController::positionController(const Eigen::Vector3d& p) :
    kp(p),
    desiredPos(Eigen::Vector3d::Zero())
{}

void positionController::setTarget(const Eigen::Vector3d& target) {
    desiredPos = target;
}

Eigen::Vector3d positionController::getTarget() {
    return desiredPos;
}

Eigen::Vector3d positionController::compute(const Eigen::Vector3d& currentPos, double dt) {
    Eigen::Vector3d pos_error = desiredPos - currentPos;
    return kp.cwiseProduct(pos_error);
}
