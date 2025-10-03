CC=gcc
CFLAGS=-I.
# target по умолчанию - собирает всё
all: sequential_min_max parallel_min_max run_sequential
# target для сборки последовательной версии
sequential_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)
# target для сборки параллельной версии 
parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)
# target для сборки программы запуска
run_sequential : 
	$(CC) -o run_sequential run_sequential.c $(CFLAGS)
# компиляция utils.c в объектный файл
utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)
# компиляция find_min_max.c в объектный файл
find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)
# очистка скомпилированных файлов
clean :
	rm utils.o find_min_max.o sequential_min_max parallel_min_max