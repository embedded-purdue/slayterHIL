#ifndef POSITION_CONTROLLER_HPP
#define POSITION_CONTROLLER_HPP

#include <Eigen/Dense>

class positionController {
public:
    // Constructors
    positionController();
<<<<<<< HEAD
    positionController(const Eigen::Vector3d& p);
=======
    positionController(double p);
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3

    // Compute velocity target based on position error
    Eigen::Vector3d compute(const Eigen::Vector3d& currentPos, double dt);

    void setTarget(const Eigen::Vector3d& target);
    Eigen::Vector3d getTarget();
    // Position PID gain (usually only P or PD)
<<<<<<< HEAD
    Eigen::Vector3d kp;
=======
    double kp;
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3

    // Desired position
    Eigen::Vector3d desiredPos;

<<<<<<< HEAD
=======
private:
    Eigen::Vector3d currentPos;
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3
};

#endif // POSITION_CONTROLLER_HPP

