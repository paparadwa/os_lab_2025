#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

// Добавлено: глобальные переменные для тайм-аута
pid_t *child_pids_global = NULL;
int pnum_global = 0;
volatile sig_atomic_t timeout_triggered = 0; // Добавлено: флаг таймаута

// Добавлено: обработчик сигнала SIGALRM
void timeout_handler(int signum) {
    timeout_triggered = 1; // Добавлено: ставим флаг
    printf("\nTimeout reached! Killing all child processes...\n");
    for (int i = 0; i < pnum_global; i++) {
        if (child_pids_global[i] > 0) {
            kill(child_pids_global[i], SIGKILL); // Добавлено
        }
    }
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;
    int timeout = 0; // Добавлено: значение тайм-аута (0 = без тайм-аута)

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 0}, // Добавлено
            {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 3:
                        with_files = true;
                        break;
                    case 4: // Добавлено: обработка timeout
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("timeout must be a positive number\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
               argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    int active_child_processes = 0;
    int pipe_fd[2];
    pid_t *child_pids = malloc(pnum * sizeof(pid_t));
    child_pids_global = child_pids; // Добавлено
    pnum_global = pnum;              // Добавлено

    if (!with_files) {
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            free(array);
            free(child_pids);
            return 1;
        }
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Добавлено: установка обработчика сигнала SIGALRM
    signal(SIGALRM, timeout_handler);
    if (timeout > 0) {
        alarm(timeout); // Добавлено: запускаем таймер
    }

    // Создание дочерних процессов
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            child_pids[i] = child_pid;

            if (child_pid == 0) {
                int begin = i * (array_size / pnum);
                int end = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
                struct MinMax local_min_max = GetMinMax(array, begin, end);

                if (with_files) {
                    char filename[32];
                    sprintf(filename, "min_max_%d.txt", getpid());
                    FILE *file = fopen(filename, "w");
                    if (file != NULL) {
                        fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                        fclose(file);
                    }
                } else {
                    close(pipe_fd[0]); // Close read end in child
                    write(pipe_fd[1], &local_min_max, sizeof(struct MinMax));
                    close(pipe_fd[1]);
                }

                free(array);
                free(child_pids);
                exit(0);
            }
        } else {
            printf("Fork failed!\n");
            free(array);
            free(child_pids);
            return 1;
        }
    }

    if (!with_files) {
        close(pipe_fd[1]); // Close write end in parent
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    // -----------------------------
    // Добавлено: сбор результатов с использованием WNOHANG
    // -----------------------------
    int remaining = pnum;
    while (remaining > 0) {
        for (int i = 0; i < pnum; i++) {
            if (child_pids[i] > 0) {
                int status;
                pid_t result = waitpid(child_pids[i], &status, WNOHANG); // Добавлено: неблокирующий wait
                if (result > 0) {
                    // Процесс завершился
                    if (with_files) {
                        char filename[32];
                        sprintf(filename, "min_max_%d.txt", child_pids[i]);
                        FILE *file = fopen(filename, "r");
                        if (file != NULL) {
                            int min, max;
                            if (fscanf(file, "%d %d", &min, &max) == 2) {
                                if (min < min_max.min) min_max.min = min;
                                if (max > min_max.max) min_max.max = max;
                            }
                            fclose(file);
                            remove(filename);
                        } else {
                            printf("Warning: couldn't open %s\n", filename);
                        }
                    } else {
                        struct MinMax local_min_max;
                        read(pipe_fd[0], &local_min_max, sizeof(struct MinMax));
                        if (local_min_max.min < min_max.min) min_max.min = local_min_max.min;
                        if (local_min_max.max > min_max.max) min_max.max = local_min_max.max;
                    }

                    child_pids[i] = 0; // Помечаем процесс как завершённый
                    remaining--;
                }
            }
        }
    }

    if (!with_files) {
        close(pipe_fd[0]);
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}
