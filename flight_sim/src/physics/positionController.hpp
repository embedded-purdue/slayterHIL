#ifndef POSITION_CONTROLLER_HPP
#define POSITION_CONTROLLER_HPP

#include <Eigen/Dense>

class positionController {
public:
    // Constructors
    positionController();
    positionController(double p);

    // Compute velocity target based on position error
    Eigen::Vector3d compute(const Eigen::Vector3d& currentPos, double dt);

    void setTarget(const Eigen::Vector3d& target);
    Eigen::Vector3d getTarget();
    // Position PID gain (usually only P or PD)
    double kp;

    // Desired position
    Eigen::Vector3d desiredPos;

private:
    Eigen::Vector3d currentPos;
};

#endif // POSITION_CONTROLLER_HPP

