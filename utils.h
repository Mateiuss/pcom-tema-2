#pragma once
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <map>
#include <queue>
#include <iomanip>
#include <cmath>

#define MAX_FDS 2000
#define TOPIC_LEN 50
#define PAYLOAD_LEN 1501
#define ID_LEN 11
#define COMMAND_LEN 12

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

using namespace std;

// The info that is sent through UDP
struct udp_payload {
  char topic[TOPIC_LEN];
  uint8_t tip_date;
  char payload[PAYLOAD_LEN];
};

// The info that the server sends through TCP
struct tcp_packet {
  sockaddr_in udp_client;
  udp_payload payload;
};

// The info that the client sends through TCP
struct notify_packet {
  char command[COMMAND_LEN];
  char topic[TOPIC_LEN];
  uint8_t sf;
};

// The client structure
struct client {
  char id[ID_LEN];
  int fd;
  map<string, bool> topics;
  queue<tcp_packet> sf_queue;
};

int recv_all(int sockfd, void *buffer, size_t len);
int send_all(int sockfd, void *buffer, size_t len);
