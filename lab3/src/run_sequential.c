#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // дочерний процесс
        printf("Child process: Starting sequential_min_max with seed=%s, array_size=%s\n", 
               argv[1], argv[2]);
        
        // запускаем sequential_min_max с переданными аргументами
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        
        // если execl вернул управление, значит произошла ошибка
        perror("execl failed");
        exit(1);
    } else {
        // родительский процесс
        int status;
        waitpid(pid, &status, 0);  // ожидаем завершения дочернего процесса
        
        if (WIFEXITED(status)) {
            printf("Parent process: Child exited with status %d\n", WEXITSTATUS(status)); //если всё ок
        } else {
            printf("Parent process: Child process terminated with error\n"); //если не всё ок
        }
    }
    
    return 0;
}