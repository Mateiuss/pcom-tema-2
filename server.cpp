#include "utils.h"

using namespace std;

uint16_t port;

int udp_sock;
int listen_sock;

map<string, client> clients;

void run_server() {
  vector<pollfd> poll_fds;

  int rc;

  rc = listen(listen_sock, MAX_FDS);
  DIE(rc < 0, "listen");

  // Adding the listener in the poll
  poll_fds.push_back({listen_sock, POLLIN, 0});

  // Adding the udp socket
  poll_fds.push_back({udp_sock, POLLIN, 0});

  // Also, adding the STDIN fd
  poll_fds.push_back({STDIN_FILENO, POLLIN, 0});

  // Here comes the funny business
  while (1) {
    rc = poll(poll_fds.data(), poll_fds.size(), -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < poll_fds.size(); i++) {
      if (!(poll_fds[i].revents & POLLIN)) continue;

      if (poll_fds[i].fd == STDIN_FILENO) { // Received a message at STDIN
        char msg[100];
        scanf("%s", msg);

        // Checking if the message is "exit"
        if (strcmp(msg, "exit") == 0) {
          goto exit;
        } else {
          printf("Unknown command.\n");
        }
      } else if (poll_fds[i].fd == listen_sock) { // A new connection
        sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);

        // Accepting the connection
        int newsockfd = accept(listen_sock, (sockaddr*)&cli_addr, &cli_len);
        DIE(newsockfd < 0, "accept");

        // Receiving the client's ID
        char id[11];
        recv_all(newsockfd, id, 11);

        map<string, client>::iterator j = clients.find(id);
        if (j == clients.end()) { // Adding a new client (that didn't connect before)
          client tmp_client;
          tmp_client.fd = newsockfd;
          strcpy(tmp_client.id, id);

          clients[id] = tmp_client;

          poll_fds.push_back({newsockfd, POLLIN, 0});

          printf("New client %s connected from %s:%d.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        } else if (j->second.fd == -1) { // Adding a new client (that connected before)
          // Updating the client's fd to the new socket
          j->second.fd = newsockfd;

          poll_fds.push_back({newsockfd, POLLIN, 0});

          // Sending all stored packets
          while (!j->second.sf_queue.empty()) {
            tcp_packet tmp = j->second.sf_queue.front();
            j->second.sf_queue.pop();

            rc = send_all(newsockfd, &tmp, sizeof(tcp_packet));
            DIE(rc < 0, "send_all");
          }

          printf("New client %s connected from %s:%d.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        } else { // Client already connected
          printf("Client %s already connected.\n", id);

          close(newsockfd);
        }
      } else if (poll_fds[i].fd == udp_sock) { // Message from an UDP client
        tcp_packet packet;
        sockaddr_in udp_client;
        socklen_t udp_len = sizeof(udp_client);

        // Receiving the packet
        rc = recvfrom(poll_fds[i].fd, &packet.payload, sizeof(packet.payload), 0, (sockaddr*)&udp_client, &udp_len);
        DIE(rc < 0, "recvfrom");

        for (auto j = clients.begin(); j != clients.end(); j++) {
          map<string, bool>::iterator is_topic = j->second.topics.find(packet.payload.topic);

          // Checking if the client is subscribed to the topic
          if (is_topic == j->second.topics.end()) continue;

          if (j->second.fd == -1) {
            if (is_topic->second == false) {
              continue;
            }

            // Adding the packet to the queue if store and forward is enabled
            j->second.sf_queue.push(packet);
          } else {
            // Sending the packet to the client (if client is connected)
            send_all(j->second.fd, &packet, sizeof(packet));
          }
        }
      } else { // Message from a TCP client
        notify_packet packet;
        
        uint8_t packet_len = 0;
        recv_all(poll_fds[i].fd, &packet_len, sizeof(packet_len));

        recv_all(poll_fds[i].fd, &packet, packet_len);

        if (strcmp(packet.command, "exit") == 0) { // Exit
          const char *id;

          // Updating the client's fd to -1
          for (auto client : clients) {
            if (client.second.fd == poll_fds[i].fd) {
              clients[client.first].fd = -1;
              id = client.first.data();
              break;
            }
          }

          // Removing the client from the poll
          poll_fds.erase(poll_fds.begin() + i);

          printf("Client %s disconnected.\n", id);
        } else if (strcmp(packet.command, "subscribe") == 0) { // Subscribe
          for (auto j = clients.begin(); j != clients.end(); j++) {
            if (j->second.fd == poll_fds[i].fd) {
              clients[j->first].topics[packet.topic] = packet.sf;
              break;
            }
          }
        } else if (strcmp(packet.command, "unsubscribe") == 0) { // Unsubscribe
          for (auto j = clients.begin(); j != clients.end(); j++) {
            if (j->second.fd == poll_fds[i].fd) {
              clients[j->first].topics.erase(packet.topic);
              break;
            }
          }
        } else { // Unknown command
          printf("Unknown command.\n");
        }
      }
    }
  }

exit:
  // Closing all connections
  for (int i = 3; i < poll_fds.size(); i++) {
    close(poll_fds[i].fd);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
      cout << "Usage: " << argv[0] << " <port>\n";
      return 1;
  }

  int rc;

  // Disable stdout buffering
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Take de port from the arguments
  port = atoi(argv[1]);

  // Server addr
  sockaddr_in serveraddr;
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = INADDR_ANY;
  serveraddr.sin_port = htons(port);

  // Initialise the UDP socket
  udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_sock < 0, "udp socket failed");

  // Bind the udp socked
  rc = bind(udp_sock, (const sockaddr*)&serveraddr, sizeof(serveraddr));
  DIE(rc < 0, "udp bind failed");

  // Listening socket
  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listen_sock < 0, "tcp listening socket failed");

  // Reuse the address
  int enable = 1;
  if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  // Disable Nagle's algorithm
  if (setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
    perror("setsockopt(TCP_NODELAY) failed");

  // Bind the tcp socket
  rc = bind(listen_sock, (const sockaddr*)&serveraddr, sizeof(serveraddr));
  DIE(rc < 0, "tcp bind failed");

  run_server(); // <=====================

  // Closing sockets
  close(listen_sock);
  close(udp_sock);

  return 0;
}