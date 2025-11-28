#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "MultModuloCommon.h" 

struct Server {
  char ip[255];
  int port;
};

// Структура для передачи данных в поток
struct ThreadData {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

// Функция для работы с сервером в отдельном потоке
void* ThreadServer(void* args) {
  struct ThreadData* data = (struct ThreadData*)args;
  
  printf("Connecting to server %s:%d\n", data->server.ip, data->server.port);
  
  struct hostent *hostname = gethostbyname(data->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(data->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", data->server.ip, data->server.port);
    close(sck);
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &data->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    close(sck);
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    close(sck);
    return NULL;
  }

  memcpy(&data->result, response, sizeof(uint64_t));
  printf("Received from server %s:%d: %lu\n", data->server.ip, data->server.port, data->result);

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        strncpy(servers, optarg, sizeof(servers) - 1);
        servers[sizeof(servers) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  printf("Parameters: k=%lu, mod=%lu, servers_file=%s\n", k, mod, servers);

  // Чтение файла с серверами
  FILE* servers_file = fopen(servers, "r");
  if (servers_file == NULL) {
    perror("fopen failed");
    fprintf(stderr, "Cannot open file: %s\n", servers);
    return 1;
  }

  struct Server* to = NULL;
  unsigned int servers_num = 0;
  char line[255];
  
  printf("Reading servers from file...\n");
  
  while (fgets(line, sizeof(line), servers_file)) {
    // Убрать символ новой строки
    line[strcspn(line, "\n")] = 0;
    
    if (strlen(line) == 0) continue; // Пропустить пустые строки
    
    printf("Processing server line: %s\n", line);
    
    servers_num++;
    to = realloc(to, sizeof(struct Server) * servers_num);
    if (to == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      fclose(servers_file);
      return 1;
    }
    
    char* colon = strchr(line, ':');
    if (colon == NULL) {
      fprintf(stderr, "Invalid server format (missing colon): %s\n", line);
      fclose(servers_file);
      free(to);
      return 1;
    }
    *colon = '\0';
    strncpy(to[servers_num-1].ip, line, sizeof(to[servers_num-1].ip) - 1);
    to[servers_num-1].ip[sizeof(to[servers_num-1].ip) - 1] = '\0';
    to[servers_num-1].port = atoi(colon+1);
    
    printf("Added server: %s:%d\n", to[servers_num-1].ip, to[servers_num-1].port);
  }
  fclose(servers_file);

  if (servers_num == 0) {
    fprintf(stderr, "No servers found in file\n");
    return 1;
  }

  printf("Total servers: %d\n", servers_num);

  // Разделение работы между серверами
  pthread_t threads[servers_num];
  struct ThreadData thread_data[servers_num];

  uint64_t numbers_per_server = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current_start = 1;

  printf("Distributing work...\n");
  
  for (int i = 0; i < servers_num; i++) {
    thread_data[i].server = to[i];
    thread_data[i].begin = current_start;
    thread_data[i].end = current_start + numbers_per_server - 1 + (i < remainder ? 1 : 0);
    thread_data[i].mod = mod;
    
    printf("Server %d: numbers from %lu to %lu\n", i, thread_data[i].begin, thread_data[i].end);
    
    current_start = thread_data[i].end + 1;

    if (pthread_create(&threads[i], NULL, ThreadServer, (void*)&thread_data[i])) {
      fprintf(stderr, "Error creating thread\n");
      free(to);
      return 1;
    }
  }

  // Ожидание завершения всех потоков
  printf("Waiting for threads to complete...\n");
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
  }

  // Объединение результатов
  uint64_t total = 1;
  for (int i = 0; i < servers_num; i++) {
    total = MultModulo(total, thread_data[i].result, mod);
  }

  printf("Final answer: %lu\n", total);
  free(to);

  return 0;
}