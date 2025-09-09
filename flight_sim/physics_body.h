#ifndef SLAYTERHIL_PHYSICS_SIM_H_
#define SLAYTERHIL_PHYSICS_SIM_H_

#include <Eigen/Dense>
#include <Eigen/Geometry>

class physics_body{
public: 
	double mass;
	Eigen::Vector3d position;
	Eigen::Vector3d velocity;
	Eigen::Vector3d acceleration;
	Eigen::Vector3d totalForce;
	Eigen::Quaterniond orientation;

	physics_body();
	physics_body(double mass,
	      const Eigen::Vector3d& position,
	      const Eigen::Vector3d& velocity,
	      const Eigen::Vector3d& acceleration,
	      const Eigen::Quaterniond& orientation);

	virtual void applyForce(const Eigen::Vector3d& force) = 0; //probobally just implement when making drone object 
	virtual void update(double dt) = 0; 
	virtual ~physics_body() = default; 
};

#endif
