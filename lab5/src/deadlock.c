#include <stdio.h>
#include <pthread.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void* t1(void* arg) {
    pthread_mutex_lock(&m1);
    printf("DEADLOCK! часть 1\n");
    pthread_mutex_lock(&m2);//DEADLOCK!
    return NULL;
}

void* t2(void* arg) {
    pthread_mutex_lock(&m2);
    printf("DEADLOCK! часть 2\n");
    pthread_mutex_lock(&m1);//DEADLOCK!
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    pthread_create(&thread1, NULL, t1, NULL);
    pthread_create(&thread2, NULL, t2, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Никогда в жизни не прочитаем вывод этой программы\n");
    return 0;
}