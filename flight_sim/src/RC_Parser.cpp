// Parses RC Commands and translates them into vectors

// Reference RC Language:
// I --> Idle for 100 ms
// L, R --> Left, Right
// F, B --> Forward, Backward
// U, D --> Up, Down

#include <flight_sim.hpp>
// :TODO: THERE WILL BE . in between timesteps.
// Main translate function
// List of Unit Velocity Vectors
std::vector<Eigen::Vector3d> RCtranslate(std::string cmdString) {
  std::vector<Eigen::Vector3d> velCmds;

  for (char cmd : cmdString) {
    switch (cmd) {
    case 'I':
      velCmds.push_back(Eigen::Vector3d(0, 0, 0)); // Idle
      break;
    case 'L':
      velCmds.push_back(Eigen::Vector3d(-1, 0, 0)); // Left
      break;
    case 'R':
      velCmds.push_back(Eigen::Vector3d(1, 0, 0)); // Right
      break;
    case 'F':
      velCmds.push_back(Eigen::Vector3d(0, 1, 0)); // Forward
      break;
    case 'B':
      velCmds.push_back(Eigen::Vector3d(0, -1, 0)); // Backward
      break;
    case 'U':
      velCmds.push_back(Eigen::Vector3d(0, 0, 1)); // Up
      break;
    case 'D':
      velCmds.push_back(Eigen::Vector3d(0, 0, -1)); // Down
      break;
    default:
      velCmds.push_back(Eigen::Vector3d(0, 0, 0)); // I also stands for Invalid
    }
  }
  return velCmds;
}
