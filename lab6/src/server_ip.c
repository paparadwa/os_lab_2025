#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "MultModuloCommon.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;

  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }

  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  return (void *)(uint64_t *)Factorial(fargs);
}

void print_network_info(int port) {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("=== Server Network Information ===\n");
    printf("Server hostname: %s\n", hostname);
    printf("Server listening on port: %d\n", port);
    printf("Server IP: 0.0.0.0 (all network interfaces)\n");
    printf("Clients can connect using your computer's IP address\n");
    printf("==================================\n");
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        if (port <= 0 || port > 65535) {
          fprintf(stderr, "Invalid port number: %d. Port must be between 1 and 65535\n", port);
          return 1;
        }
        break;
      case 1:
        tnum = atoi(optarg);
        if (tnum <= 0) {
          fprintf(stderr, "Invalid thread number: %d. Must be positive\n", tnum);
          return 1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket! Error: %s\n", strerror(errno));
    return 1;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY); // Принимаем соединения с любых IP

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Cannot bind to socket! Error: %s\n", strerror(errno));
    fprintf(stderr, "Make sure port %d is available and not used by another program\n", port);
    close(server_fd);
    return 1;
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Cannot listen on socket! Error: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }

  print_network_info(port);
  printf("Server is ready for connections...\n");
  printf("To connect from another computer, use your IP address and port %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Failed to accept new connection\n");
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("New connection from: %s:%d\n", client_ip, ntohs(client.sin_port));

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int read_bytes = recv(client_fd, from_client, buffer_size, 0);

      if (read_bytes == 0) {
        printf("Client %s:%d disconnected\n", client_ip, ntohs(client.sin_port));
        break;
      }
      if (read_bytes < 0) {
        fprintf(stderr, "Failed to read from client\n");
        break;
      }
      if (read_bytes < buffer_size) {
        fprintf(stderr, "Client sends wrong data format\n");
        break;
      }

      pthread_t threads[tnum];

      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Received from %s: %lu %lu %lu\n", client_ip, begin, end, mod);

      struct FactorialArgs args[tnum];
      uint64_t part = (end - begin + 1) / tnum;
      uint64_t remainder = (end - begin + 1) % tnum;
      uint64_t current = begin;

      for (uint32_t i = 0; i < tnum; i++) {
        args[i].begin = current;
        args[i].end = current + part - 1 + (i < remainder ? 1 : 0);
        args[i].mod = mod;
        current = args[i].end + 1;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          fprintf(stderr, "Error: pthread_create failed!\n");
          close(client_fd);
          break;
        }
      }

      uint64_t total = 1;
      for (uint32_t i = 0; i < tnum; i++) {
        uint64_t result = 0;
        pthread_join(threads[i], (void **)&result);
        total = MultModulo(total, result, mod);
      }

      printf("Total for %s: %lu\n", client_ip, total);

      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Cannot send data to client\n");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  close(server_fd);
  return 0;
}