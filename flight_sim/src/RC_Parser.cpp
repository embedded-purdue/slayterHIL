// Parses RC Commands and translates them into vectors

// Reference RC Language:
// I --> Idle for 100 ms
// L, R --> Left, Right
// F, B --> Forward, Backward
// U, D --> Up, Down

#include <flight_sim.hpp>

#define TEST_PATH_ROOT ("../../tests/flightpaths/")

Eigen::Vector3d rc_parse(std::string);

std::vector<Eigen::Vector3d> rc_read(std::string test_name) {
    std::string path = TEST_PATH_ROOT;
    path += test_name;

    FILE *fp = fopen(path.c_str(), "r");
    assert(fp);

    std::vector<Eigen::Vector3d> ret;

    for (;;) {
        char cmd[3] = {};

        size_t scan = fscanf(fp, "%2[^.].", cmd);

        if (scan != 1) {
            if (scan == EOF) {
                break;
            }

            assert(false);
        }

        ret.push_back(rc_parse(cmd));
    }

    return ret;
}

Eigen::Vector3d rc_parse(std::string cmdString) {
    Eigen::Vector3d ret = Eigen::Vector3d(0, 0, 0);

    for (char cmd : cmdString) {
        switch (cmd) {
            case 'L':
                ret += Eigen::Vector3d(-1, 0, 0);
                break;
            case 'R':
                ret += Eigen::Vector3d(1, 0, 0);
                break;
            case 'F':
                ret += Eigen::Vector3d(0, 1, 0);
                break;
            case 'B':
                ret += Eigen::Vector3d(0, -1, 0);
                break;
            case 'U':
                ret += Eigen::Vector3d(0, 0, 1);
                break;
            case 'D':
                ret += Eigen::Vector3d(0, 0, -1);
                break;
        }
    }
    return ret;
}
