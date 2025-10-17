#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork(); // Создаём дочерний процесс

    if (pid > 0) {
        // Родительский процесс
        printf("PID родительского процесса: %d\n", getpid());
        printf("PID дочернего процесса: %d\n", pid);

        printf("10 секунд, чтоб успеть посмотреть на процесс в таблице...\n");
        sleep(10); // Родитель ждёт, не вызывая wait()
        printf("Родительский процесс завершает работу.\n");

    } else if (pid == 0) {
        // Дочерний процесс
        printf("PID дочерний процесс %d завершает работу\n", getpid());
        exit(0);
    } else {
        perror("Ошибка fork");
        return 1;
    }

    return 0;
}
