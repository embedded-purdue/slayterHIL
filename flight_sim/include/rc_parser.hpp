#pragma once
#include <Eigen/Core>
#include <string>
#include <vector>

// Read the supplied filesystem path. Commands end in '.', e.g. UL.R.I.
std::vector<Eigen::Vector3d> rc_read(std::string path);
