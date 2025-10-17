#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>
#include "utils.h"  // Для GenerateArray
#include "sum.h"    // Для Sum и struct SumArgs

/* Thread function */
void *ThreadSum(void *args) {
    const struct SumArgs *sum_args = (const struct SumArgs *)args; // Используем const, чтобы соответствовать Sum()
    // Считаем сумму для заданного диапазона
    int sum = Sum(sum_args);
    return (void *)(size_t)sum;
}

int main(int argc, char **argv) {
    /*
     *  TODO:
     *  threads_num by command line arguments
     *  array_size by command line arguments
     *  seed by command line arguments
     */

    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;

    /* Парсим аргументы командной строки */
    while (1) {
        static struct option options[] = {
            {"threads_num", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"seed", required_argument, 0, 0},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;

        switch (option_index) {
            case 0:
                threads_num = atoi(optarg);
                break;
            case 1:
                array_size = atoi(optarg);
                break;
            case 2:
                seed = atoi(optarg);
                break;
            default:
                printf("Unknown option\n");
                return 1;
        }
    }

    if (threads_num == 0 || array_size == 0 || seed == 0) {
        printf("Usage: %s --threads_num num --array_size num --seed num\n", argv[0]);
        return 1;
    }

    /* TODO: Generate array here */
    int *array = malloc(sizeof(int) * array_size);
    if (!array) {
        perror("malloc failed");
        return 1;
    }

    /* Генерация массива (не учитывается в замере времени) */
    GenerateArray(array, array_size, seed);

    /* Создаем потоки и разбиваем массив на части */
    pthread_t threads[threads_num];
    struct SumArgs args[threads_num]; // Используем структуру из sum.h

    int chunk = array_size / threads_num;
    int remainder = array_size % threads_num;

    struct timeval start_time, finish_time;
    gettimeofday(&start_time, NULL); /* Начало замера времени */

    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * chunk;
        args[i].end = (i == threads_num - 1) ? ((i + 1) * chunk + remainder) : ((i + 1) * chunk);
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
            printf("Error: pthread_create failed!\n");
            free(array);
            return 1;
        }
    }

    int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        int sum = 0;
        pthread_join(threads[i], (void **)&sum);
        total_sum += sum; /* суммируем результаты всех потоков */
    }

    gettimeofday(&finish_time, NULL); /* Конец замера времени */

    free(array);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("Total sum: %d\n", total_sum);
    printf("Elapsed time: %f ms\n", elapsed_time);

    return 0;
}
