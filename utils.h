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
#define MAX_MSG_LEN 64

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

using namespace std;

struct udp_payload {
  char topic[50];
  uint8_t tip_date;
  char payload[1500];
};

struct tcp_packet {
  sockaddr_in udp_client;
  udp_payload payload;
};

struct notify_packet {
  uint16_t len;
  char message[MAX_MSG_LEN];
};

struct client {
  char id[11];
  int fd;
  map<string, bool> topics;
  queue<tcp_packet> sf_queue;
};

int recv_all(int sockfd, void *buffer, size_t len);
int send_all(int sockfd, void *buffer, size_t len);
