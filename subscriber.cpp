#include "utils.h"

using namespace std;

int sockfd = -1;
uint16_t port;
char id[11];

void run_client() {
    // Sending the client's ID
    send_all(sockfd, id, 11);

    int rc;

    // Adding the socket and STDIN to the poll
    pollfd poll_fds[2];

    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    while (1) {
        rc = poll(poll_fds, 2, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < 2; i++) {
            if (!(poll_fds[i].revents & POLLIN)) continue;

            if (poll_fds[i].fd == sockfd) { // Received something from the server
                tcp_packet packet;
                uint16_t packet_len = 0;

                rc = recv_all(sockfd, &packet_len, sizeof(packet_len));

                if (rc == 0) return;

                rc = recv_all(sockfd, &packet, packet_len);

                printf("%s:%d - %s - ", inet_ntoa(packet.udp_client.sin_addr), packet.udp_client.sin_port, packet.payload.topic);
                
                if (packet.payload.tip_date == 0) {
                    cout << "INT - ";
                    if (packet.payload.payload[0] == 1) {
                        cout << "-";
                    }
                    cout << ntohl(*((uint32_t*)(packet.payload.payload + 1))) << "\n";
                } else if (packet.payload.tip_date == 1) {
                    cout << "SHORT_REAL - " << fixed << setprecision(2) << (float)ntohs(*((uint16_t*)packet.payload.payload)) / 100 << "\n";
                } else if (packet.payload.tip_date == 2) {
                    cout << "FLOAT - ";
                    if (packet.payload.payload[0] == 1) {
                        cout << "-";
                    }
                    uint32_t val = ntohl(*((uint32_t*)(packet.payload.payload + 1)));
                    uint8_t exp = packet.payload.payload[5];

                    char val_str[10];

                    // Converting the value to a string
                    sprintf(val_str, "%d", val);

                    int len = strlen(val_str);

                    // The position of the decimal point
                    int pos = len - exp;

                    if (pos <= 0) { // The decimal point is before the first digit
                        cout << "0.";
                        for (int i = 0; i < -pos; i++) {
                            cout << "0";
                        }
                        cout << val_str;
                    } else { // The decimal point is after the first digit
                        for (int i = 0; i < pos; i++) {
                            cout << val_str[i];
                        }
                        cout << ".";
                        for (int i = pos; i < len; i++) {
                            cout << val_str[i];
                        }
                    }
                    cout << "\n";
                } else if (packet.payload.tip_date == 3) {
                    cout << "STRING - " << packet.payload.payload << "\n";
                }
            } else { // Received something from STDIN
                notify_packet packet;

                cin >> packet.command;

                if (strcmp(packet.command, "exit") == 0) {
                    uint8_t packet_len = strlen(packet.command) + 1;

                    send_all(sockfd, &packet_len, sizeof(uint8_t));
                    send_all(sockfd, packet.command, packet_len);

                    return;
                } else if (strcmp(packet.command, "subscribe") == 0) {
                    cin >> packet.topic;
                    cin >> packet.sf;

                    packet.sf -= '0';

                    // Check for invalid SF
                    if (packet.sf != 0 && packet.sf != 1) {
                        cout << "Invalid SF\n";
                        continue;
                    }

                    uint8_t packet_len = sizeof(packet);

                    send_all(sockfd, &packet_len, sizeof(uint8_t));
                    send_all(sockfd, &packet, packet_len);

                    cout << "Subscribed to topic.\n";
                } else if (strcmp(packet.command, "unsubscribe") == 0) {
                    cin >> packet.topic;

                    uint8_t packet_len = sizeof(packet.command) + strlen(packet.topic) + 2;

                    send_all(sockfd, &packet_len, sizeof(uint8_t));
                    send_all(sockfd, &packet, packet_len);

                    cout << "Unsubscribed from topic.\n";
                } else {
                    cout << "Invalid command\n";
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n";
        return 1;
    }

    int rc;

    // Disabling buffering for stdout
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    
    // ID
    strcpy(id, argv[1]);

    // Port
    rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Creating the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
    perror("setsockopt(TCP_NODELAY) failed");

    // Server address
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // Connecting to the server
    rc = connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

    run_client(); // <====================

    // Closing the connection
    close(sockfd);

    return 0;
}