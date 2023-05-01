#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
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

#define MAX_FDS 100

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

using namespace std;

struct udp_msg {
  char topic[50];
  char tip_date;
  union continut {
    struct zero {
      char semn;
      uint32_t numar;
    } a;

    uint16_t b;

    struct doi {
      char semn;
      uint32_t numar;
      uint8_t negpow;
    } c;

    char d[1500];
  } payload;
};

struct client {
  char id[11];
  int fd;
  map<string, bool> topics;
  queue<udp_msg> sf_queue;
};
