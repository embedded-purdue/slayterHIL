#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../build/sensor_data.pb.h"

#define PORT     8080
#define MAXLINE 1024

// Driver code
int main() {

    // Client side setup
    int sockfd;
    struct sockaddr_in     servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n;
    socklen_t len;


    float acceleration_x = 0.01f;
    float acceleration_y = 0.02f;
    float acceleration_z = -9.81f;

    float angular_velocity_x = 0.03f;
    float angular_velocity_y = 0.04f;
    float angular_velocity_z = 0.05f;

    // In Pa
    float pressure = 123000.11f;

    float altitude = 100.15f;

    // In Celcius
    float temperature = 22.5f;

    auto packet = std::make_unique<SensorPacket>();
    
    // Accessing sub-messages works identically
    google::protobuf::Timestamp* timestamp = packet->mutable_timestamp();
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now - seconds);
    timestamp->set_seconds(seconds.time_since_epoch().count());
    timestamp->set_nanos(nanoseconds.count());
    
    // Populate nested data
    ImuData* imu = packet->mutable_imu_data();
    Vector3D* accel = imu->mutable_acceleration();
    accel->set_x(acceleration_x);
    accel->set_y(acceleration_y);
    accel->set_z(acceleration_z);

    Vector3D* gyro = imu->mutable_angular_velocity();
    gyro->set_x(angular_velocity_x);
    gyro->set_y(angular_velocity_y);
    gyro->set_z(angular_velocity_z);

    AltimeterData* altimeter = packet->mutable_altimeter_data();
    altimeter->set_pressure(pressure);
    altimeter->set_altitude(altitude);
    altimeter->set_temperature(temperature);

    printf("Acceleration x,y,z: %f, %f, %f\n", acceleration_x, acceleration_y, acceleration_z);
    printf("Angular Velocity x,y,z: %f, %f, %f\n", angular_velocity_x, angular_velocity_y, angular_velocity_z);
    printf("Pressure: %f, Altitude: %f, Temperature: %f\n", pressure, altitude, temperature);
    
    // Serialize data into string to send over UDP
    std::string serialized_data;
    if (!packet->SerializeToString(&serialized_data)) {
        std::cerr << "Failed to serialize packet." << std::endl;
        return -1;
    }

    printf("Flight sim sending: %s\n", serialized_data.c_str());

    // For testing purposes, try to parse the data
    SensorPacket parsed_packet;
    if (!parsed_packet.ParseFromString(serialized_data)) {
        std::cerr << "Failed to parse packet from string." << std::endl;
        return -1;
    }

    float pressure_returned = parsed_packet.altimeter_data().pressure();
    printf("Pressure deserialized: %f\n", pressure_returned);


    while (true) {
        std::string line;
        std::cout << "Enter SEND to send packet";
        std::cin >> line;
        int bytesSent = sendto(sockfd, (const char *)serialized_data.c_str(), serialized_data.size(),
                        0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        if (bytesSent < 0) {
            std::cerr << "Failed to send packet." << std::endl;
            return -1;
        }
        printf("Bytes sent: %d\n", bytesSent);
    }


    close(sockfd);
    return 0;
}
