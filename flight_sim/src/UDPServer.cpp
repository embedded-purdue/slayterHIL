#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../build/sensor_data.pb.h"

#define PORT (8080)
#define MAXLINE (1024)

int main() {
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len;

    len = sizeof(cliaddr);

    // Server loop for accepting information. On Raspberry Pi this will likely be changed.
    while (true) {
        int bytesReceived = recvfrom(sockfd, buffer, MAXLINE, 
            MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        printf("Bytes received: %d\n", bytesReceived);
        SensorPacket parsed_packet;
        // if (!parsed_packet.ParseFromString(buffer)) {
        //     std::cerr << "Failed to parse packet from string." << std::endl;
        //     return -1;
        // }
        parsed_packet.ParseFromString(buffer);
        float pressure_returned = parsed_packet.altimeter_data().pressure();
        int timestamp = parsed_packet.timestamp();
        printf("Pressure deserialized: %f\n", pressure_returned);
        printf("Timestamp parsed: %d\n", timestamp);
    }

    return 0;
}
