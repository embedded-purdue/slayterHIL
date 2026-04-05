#ifndef POSITION_CONTROLLER_HPP
#define POSITION_CONTROLLER_HPP

#include <Eigen/Dense>

class positionController {
public:
    positionController();
    positionController(const Eigen::Vector3d& p);

    Eigen::Vector3d compute(const Eigen::Vector3d& currentPos, double dt);

    void setTarget(const Eigen::Vector3d& target);
    Eigen::Vector3d getTarget();

    Eigen::Vector3d kp;
    Eigen::Vector3d desiredPos;

private:
    Eigen::Vector3d currentPos;
};

#endif // POSITION_CONTROLLER_HPP
