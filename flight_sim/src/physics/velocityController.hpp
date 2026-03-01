#ifndef VELOCITY_CONTROLLER_HPP
#define VELOCITY_CONTROLLER_HPP

#include <Eigen/Dense>

class velocityController {
public:
    // Constructors
<<<<<<< HEAD
    double maxForce;
    velocityController();
    velocityController(const Eigen::Vector3d& p, const Eigen::Vector3d& i, const Eigen::Vector3d& d, double maxF);

    // Compute PID output: acceleration command
    Eigen::Vector3d compute(const Eigen::Vector3d& currentVelocity,
                            const Eigen::Vector3d& targetVelocity, double dt);

    // PID gains
    Eigen::Vector3d kp;
    Eigen::Vector3d ki;
    Eigen::Vector3d kd;
=======
    velocityController();
    velocityController(double p, double i, double d);

    // Compute PID output: acceleration command
    Eigen::Vector3d compute(const Eigen::Vector3d& currentVelocity,
                            const Eigen::Vector3d& targetVelocity,
                            double dt);

    // PID gains
    double kp;
    double ki;
    double kd;
>>>>>>> a7e1c188b7135259affc3bbdf332b8db098123d3

private:
    Eigen::Vector3d velocityIntegral;
    Eigen::Vector3d previousError;
    Eigen::Vector3d velocityError;
};

#endif // VELOCITY_CONTROLLER_HPP

