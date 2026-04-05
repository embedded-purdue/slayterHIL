#ifndef VELOCITY_CONTROLLER_HPP
#define VELOCITY_CONTROLLER_HPP

#include <Eigen/Dense>

class velocityController {
public:
    velocityController();
    velocityController(const Eigen::Vector3d& p,
                       const Eigen::Vector3d& i,
                       const Eigen::Vector3d& d,
                       double maxForce);

    Eigen::Vector3d compute(const Eigen::Vector3d& currentVelocity,
                            const Eigen::Vector3d& targetVelocity,
                            double dt);

    Eigen::Vector3d kp;
    Eigen::Vector3d ki;
    Eigen::Vector3d kd;
    double maxForce;

private:
    Eigen::Vector3d velocityIntegral;
    Eigen::Vector3d previousError;
    Eigen::Vector3d velocityError;
};

#endif // VELOCITY_CONTROLLER_HPP
