#include "rc_parser.hpp"
#include <cctype>
#include <fstream>
#include <stdexcept>

std::vector<Eigen::Vector3d> rc_read(std::string path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read flightpath: " + path);
    std::vector<Eigen::Vector3d> result;
    Eigen::Vector3d target = Eigen::Vector3d::Zero();
    unsigned count = 0;
    for (char ch; file.get(ch);) {
        if (std::isspace(static_cast<unsigned char>(ch))) continue;
        if (ch == '.') {
            if (!count) throw std::runtime_error("Empty flightpath command");
            result.push_back(target);
            target.setZero();
            count = 0;
            continue;
        }
        if (++count > 2) throw std::runtime_error("Flightpath commands allow at most two letters");
        switch (ch) {
        case 'L': target.x() -= 1; break;
        case 'R': target.x() += 1; break;
        case 'F': target.y() += 1; break;
        case 'B': target.y() -= 1; break;
        case 'U': target.z() += 1; break;
        case 'D': target.z() -= 1; break;
        case 'I': break;
        default: throw std::runtime_error("Unknown flightpath command");
        }
    }
    if (!file.eof()) throw std::runtime_error("Failed to read flightpath");
    if (count) throw std::runtime_error("Flightpath must end with '.'");
    if (result.empty()) throw std::runtime_error("Flightpath contains no waypoints");
    return result;
}
