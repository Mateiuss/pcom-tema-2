#include "utils.h"

using namespace std;

uint16_t port;

int udp_sock;
int listen_sock;

map<string, client> clients;

void run_server() {
  vector<pollfd> poll_fds;
  int num_clients = 3;
  int rc;

  rc = listen(listen_sock, MAX_FDS);
  DIE(rc < 0, "listen");

  // Adding the listener in the poll
  poll_fds.push_back(pollfd());
  poll_fds[0].fd = listen_sock;
  poll_fds[0].events = POLLIN;

  // Adding the udp socket
  poll_fds.push_back(pollfd());
  poll_fds[1].fd = udp_sock;
  poll_fds[1].events = POLLIN;

  // Also, adding the STDIN fd
  poll_fds.push_back(pollfd());
  poll_fds[2].fd = STDIN_FILENO;
  poll_fds[2].events = POLLIN;

  // Here comes the funny business
  while (1) {
    rc = poll(poll_fds.data(), num_clients, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_clients; i++) {
      if (!(poll_fds[i].revents & POLLIN)) continue;

      if (poll_fds[i].fd == STDIN_FILENO) { // Received a message at STDIN
        char msg[100];
        scanf("%s", msg);

        if (strcmp(msg, "exit") == 0) {
          goto exit;
        } else {
          exit(1);
        }
      } else if (poll_fds[i].fd == listen_sock) { // A new connection
        sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);

        int newsockfd = accept(listen_sock, (sockaddr*)&cli_addr, &cli_len);
        DIE(newsockfd < 0, "accept");

        char id[11];
        recv(newsockfd, id, 11, 0);

        map<string, client>::iterator j = clients.find(id);
        if (j == clients.end()) { // Adding a new client
          client tmp_client;
          tmp_client.fd = newsockfd;
          strcpy(tmp_client.id, id);

          clients[id] = tmp_client;

          poll_fds.push_back({newsockfd, POLLIN, 0});

          num_clients++;

          printf("New client %s connected from %s:%d.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        } else if (j->second.fd == -1) {
          j->second.fd = newsockfd;

          poll_fds.push_back({newsockfd, POLLIN, 0});

          num_clients++;

          while (!j->second.sf_queue.empty()) {
            tcp_packet tmp = j->second.sf_queue.front();
            j->second.sf_queue.pop();

            rc = send_all(newsockfd, &tmp, sizeof(tcp_packet));
            DIE(rc < 0, "send_all");
          }

          printf("New client %s connected from %s:%d.\n", id, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        } else {
          printf("Client %s already connected.\n", id);

          close(newsockfd);
        }
      } else if (poll_fds[i].fd == udp_sock) { // Message from an UDP client
        char buffer[2000];
        memset(buffer, 0, 2000);

        sockaddr_in udp_client;
        socklen_t udp_len = sizeof(udp_client);

        rc = recv(poll_fds[i].fd, buffer, 2000, 0);
        DIE(rc < 0, "recvfrom");

        udp_payload *msg = (udp_payload*)buffer;

        tcp_packet tcp_msg;
        memcpy(&tcp_msg.payload, msg, sizeof(udp_payload));

        char topic[50];
        strcpy(topic, buffer);

        for (auto j = clients.begin(); j != clients.end(); j++) {
          map<string, bool>::iterator is_topic = j->second.topics.find(topic);

          if (is_topic == j->second.topics.end()) continue;

          if (j->second.fd == -1) {
            if (is_topic->second == false) {
              continue;
            }

            j->second.sf_queue.push(tcp_msg);
          } else {
            send_all(j->second.fd, &tcp_msg, sizeof(tcp_msg));
          }
        }
      } else { // Message from a TCP client
        notify_packet msg;
        recv_all(poll_fds[i].fd, &msg, sizeof(msg));

        if (strncmp(msg.message, "exit", 4) == 0) { // exit command
          const char *id;

          for (auto client : clients) {
            if (client.second.fd == poll_fds[i].fd) {
              clients[client.first].fd = -1;
              id = client.first.data();
              break;
            }
          }

          poll_fds.erase(poll_fds.begin() + i);

          num_clients--;

          printf("Client %s disconnected.\n", id);
        } else if (strncmp(msg.message, "subscribe", 9) == 0) { // subscribe command
          char topic[50];
          int sf;

          sscanf(msg.message, "%*s %s %d", topic, &sf);

          for (auto j = clients.begin(); j != clients.end(); j++) {
            if (j->second.fd == poll_fds[i].fd) {
              clients[j->first].topics[topic] = sf;
              break;
            }
          }
        } else if (strncmp(msg.message, "unsubscribe", 11) == 0) { // unsubscribe command
          char topic[50];

          sscanf(msg.message, "%*s %s", topic);

          for (auto j = clients.begin(); j != clients.end(); j++) {
            if (j->second.fd == poll_fds[i].fd) {
              clients[j->first].topics.erase(topic);
              break;
            }
          }
        }
      }
    }
  }

exit:
  // Closing all connections
  for (int i = 3; i < num_clients; i++) {
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

  run_server();

  close(listen_sock);
  close(udp_sock);

  return 0;
}