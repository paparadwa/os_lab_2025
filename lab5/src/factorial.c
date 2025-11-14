#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

long long result = 1;//результат
int k = 0;//число, факториал которого необходимо вычислить
int pnum = 1;//потоки
int mod = 1;//модуль
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void do_multiply(void *arg);

int main(int argc, char *argv[]) {
    //парсинг аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }
    //проверка ввода
    if (k <= 0 || pnum <= 0 || mod <= 0 || k < pnum) {
        printf("Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
        printf("Условия: k > 0; pnum > 0; mod > 0; k ≥ pnum\n");
        return 1;
    }
    
    printf("Вычисляем %d! mod %d в %d потоках\n", k, mod, pnum);
    
    pthread_t threads[pnum];
    int thread_args[pnum]; // аргументы для каждого потока
    
    //создаем потоки
    for (int i = 0; i < pnum; i++) {
        thread_args[i] = i; // номер потока
        if (pthread_create(&threads[i], NULL, (void *)do_multiply, (void *)&thread_args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    //ждем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }

    printf("Факториал %d! mod %d = %lld\n", k, mod, result);
    return 0;
}

void do_multiply(void *arg) {
    int thread_id = *((int *)arg);
    
    //какой диапазон будет считать поток 
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    
    int start = thread_id * numbers_per_thread + 1;
    int end = (thread_id + 1) * numbers_per_thread;
    
    // Распределяем остаток по первым потокам
    if (thread_id < remainder) {
        start += thread_id;
        end += thread_id + 1;
    } else {
        start += remainder;
        end += remainder;
    }
    
    printf("Поток %d | Умножение от %d до %d\n", thread_id, start, end);
    
    long long partial_result = 1;
    
    //сначала вычисляем промежуточный резульатат
    for (int i = start; i <= end; i++) {
        partial_result = (partial_result * i) % mod;
    }
    
    printf("Поток %d | Промежуточный результат: %lld\n", thread_id, partial_result);
    
    //затем защищаем обновление общего результата мьютексом
    pthread_mutex_lock(&mut);
    result = (result * partial_result) % mod;
    printf("Поток %d | Общий результат: %lld\n", thread_id, result);
    pthread_mutex_unlock(&mut);
}