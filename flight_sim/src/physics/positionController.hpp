#ifndef POSITION_CONTROLLER_HPP
#define POSITION_CONTROLLER_HPP

#include <Eigen/Dense>

class positionController {
public:
    // Constructors
    positionController();
    positionController(const Eigen::Vector3d& p);

    // Compute velocity target based on position error
    Eigen::Vector3d compute(const Eigen::Vector3d& currentPos, double dt);

    void setTarget(const Eigen::Vector3d& target);
    Eigen::Vector3d getTarget();
    // Position PID gain (usually only P or PD)
    Eigen::Vector3d kp;

    // Desired position
    Eigen::Vector3d desiredPos;

};

#endif // POSITION_CONTROLLER_HPP

