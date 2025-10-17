CC=gcc
CFLAGS=-I.

# target по умолчанию - собирает всё
all: parallel_min_max process psum

# target для сборки программы из задания 1
parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) -o parallel_min_max parallel_min_max.o utils.o find_min_max.o $(CFLAGS)

# target для сборки программы из задания 3
process: process_memory.c
	$(CC) -o process process_memory.c $(CFLAGS)

# target для сборки программы из задания 5 (pthread)
psum: parallel_sum.o utils.o sum.o
	$(CC) -o psum parallel_sum.o utils.o sum.o -pthread $(CFLAGS)

# компиляция parallel_min_max.c в объектный файл
parallel_min_max.o: parallel_min_max.c utils.h find_min_max.h
	$(CC) -o parallel_min_max.o -c parallel_min_max.c $(CFLAGS)

# компиляция parallel_sum.c в объектный файл (pthread)
parallel_sum.o: parallel_sum.c utils.h sum.h
	$(CC) -o parallel_sum.o -c parallel_sum.c $(CFLAGS)

# компиляция utils.c в объектный файл
utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# компиляция find_min_max.c в объектный файл
find_min_max.o: find_min_max.c utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# компиляция sum.c в объектный файл (для pthread)
sum.o: sum.c sum.h
	$(CC) -o sum.o -c sum.c $(CFLAGS)

# очистка скомпилированных файлов
clean:
	rm -f *.o parallel_min_max process psum
