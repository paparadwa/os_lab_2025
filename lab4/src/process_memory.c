/* Program to display address information about the process */
/* Adapted from Gray, J., program 1.4 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Below is a macro definition */
// Вывод адреса переменной, исправлено на %p и каст к (void*) для корректного отображения указателей
#define SHW_ADR(ID, I) (printf("ID %s \t is at virtual address: %p\n", ID, (void*)&I))

extern int etext, edata, end; /* Global variables for process memory */

char *cptr = "This message is output by the function showit()\n"; /* Static */
char buffer1[25];
void showit(char *p); /* Function prototype, исправлено: современный стиль и void */

/* main function */
int main() { // Исправлено: добавлен тип int
    int i = 0; /* Automatic variable */

    /* Printing addressing information */
    // Исправлено: вывод адресов через %p вместо %X
    printf("\nAddress etext: %p\n", (void*)&etext);
    printf("Address edata: %p\n", (void*)&edata);
    printf("Address end  : %p\n", (void*)&end);

    SHW_ADR("main", main);
    SHW_ADR("showit", showit);
    SHW_ADR("cptr", cptr);
    SHW_ADR("buffer1", buffer1);
    SHW_ADR("i", i);

    strcpy(buffer1, "A demonstration\n");   /* Library function */
    write(1, buffer1, strlen(buffer1));     /* System call, исправлено: без +1 для '\0' */

    showit(cptr);

    return 0;
} /* end of main function */

/* A function follows */
void showit(char *p) { // Исправлено: современный стиль, возвращает void
    char *buffer2;
    SHW_ADR("buffer2", buffer2);

    if ((buffer2 = (char *)malloc(strlen(p) + 1)) != NULL) { // Выделяем память в куче
        // Исправлено: вывод адреса выделенной памяти через %p
        printf("Allocated memory at %p\n", (void*)buffer2);
        strcpy(buffer2, p);    /* copy the string */
        printf("%s", buffer2); /* Display the string */
        free(buffer2);         /* Release location */
    } else {
        printf("Allocation error\n");
        exit(1);
    }
}

