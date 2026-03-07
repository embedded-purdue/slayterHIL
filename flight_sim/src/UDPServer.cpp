#include "spi_interface.h"
#include "sensor_data.pb.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#define PORT 8080
#define MAXLINE 1024

// Choose backend
#ifdef USE_SPI
#include "spi_linux.cpp"
#else
#include "spi_stub.cpp"
#endif

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Select SPI implementation
    SpiInterface* spi = new
#ifdef USE_SPI
        SpiLinux();
#else
        SpiStub();
#endif
    if (!spi->init()) {
        std::cerr << "SPI init failed\n";
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        return -1;
    }

    char buffer[MAXLINE];
    sockaddr_in cliaddr{};
    socklen_t cli_len = sizeof(cliaddr);

    while (true) {
        int bytesReceived = recvfrom(sockfd, buffer, MAXLINE, 0,
                                     (struct sockaddr*)&cliaddr, &cli_len);
        if (bytesReceived <= 0) continue;

        SensorPacket packet;
        if (!packet.ParseFromArray(buffer, bytesReceived)) {
            std::cerr << "Failed to parse protobuf\n";
            continue;
        }

        float payload[9] = {
            packet.imu_data().acceleration().x(),
            packet.imu_data().acceleration().y(),
            packet.imu_data().acceleration().z(),
            packet.imu_data().angular_velocity().x(),
            packet.imu_data().angular_velocity().y(),
            packet.imu_data().angular_velocity().z(),
            packet.altimeter_data().pressure(),
            packet.altimeter_data().altitude(),
            packet.altimeter_data().temperature()
        };

        if (!spi->transmit(reinterpret_cast<uint8_t*>(payload), sizeof(payload))) {
            std::cerr << "SPI transmit failed\n";
        }

        std::cout << "[Bridge] Parsed packet, pressure="
                  << packet.altimeter_data().pressure()
                  << ", timestamp=" << packet.timestamp() << "\n";
    }

    close(sockfd);
    delete spi;
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
