#ifndef VELOCITY_CONTROLLER_HPP
#define VELOCITY_CONTROLLER_HPP

#include <Eigen/Dense>

class velocityController {
public:
    // Constructors
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

private:
    Eigen::Vector3d velocityIntegral;
    Eigen::Vector3d previousError;
    Eigen::Vector3d velocityError;
};

#endif // VELOCITY_CONTROLLER_HPP

