#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

#define ENABLE_DEBUG (false)
#define FILE_NAME ("../../tests/flightpaths/hover_test.json")

using json = nlohmann::json;

struct TrajectoryPoint {
    int message_id;
    double timestamp;
    double x_pos;
    double y_pos;
    double z_pos;
    double x_vel;
    double y_vel;
    double z_vel;
};

// The Helper Function
void readTrajectoryData(const std::string& filename, std::vector<TrajectoryPoint>* outData) {
    std::ifstream f(filename);

    if (!f.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    // Parse the file into a generic JSON object
    json j_complete;
    try {
        f >> j_complete;
    } catch (const json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return;
    }

    // Ensure we empty the vector before writing to it
    outData->clear();

    // Iterate through the JSON array
    for (const auto& item : j_complete) {
        TrajectoryPoint p;

        // Fill the struct variables
        // We use .value() to be safe (provides a default 0 if key is missing)
        p.message_id = item.value("Message_id", 0);
        p.timestamp  = item.value("Timestamp", 0.0);

        p.x_pos = item.value("X_pos", 0.0);
        p.y_pos = item.value("Y_pos", 0.0);
        p.z_pos = item.value("Z_pos", 0.0);

        p.x_vel = item.value("X_vel_ext", 0.0);
        p.y_vel = item.value("Y_vel_ext", 0.0);
        p.z_vel = item.value("Z_vel_ext", 0.0);

        // Push into the vector via the pointer
        outData->push_back(p);
    }
}

#if ENABLE_DEBUG

int main() {
    std::vector<TrajectoryPoint> points = {};
    readTrajectoryData(FILE_NAME, &points);

    for (auto i : points) {
        std::cout << i.x_pos << " " << i.y_pos << " " << i.z_pos << std::endl;
    }
}

#endif
