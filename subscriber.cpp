#include "utils.h"

using namespace std;

int sockfd = -1;
uint16_t port;
char id[11];

void run_client() {
    send(sockfd, id, strlen(id), 0);

    char buf[200];

    int rc;

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

            if (poll_fds[i].fd == sockfd) {
                rc = recv(sockfd, buf, 2000, 0);

                // TODO: receive messages from server

                if (rc == 0) {
                    return;
                }
            } else {
                cin.getline(buf, 2000);

                send(sockfd, buf, strlen(buf), 0);

                if (strncmp(buf, "exit", 4) == 0) {
                    return;
                } else if (strncmp(buf, "subscribe", 9) == 0) {
                    cout << "Subscribed to topic.\n";
                } else if (strncmp(buf, "unsubscribe", 11) == 0) {
                    cout << "Unsubscribed from topic.\n";
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

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    
    // ID
    rc = sscanf(argv[1], "%s", id);
    DIE(rc != 1, "Given ID is invalid");

    // PORT
    rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Creating the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

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

    // Running the client
    run_client();

    // Closing the connection
    close(sockfd);

    return 0;
}