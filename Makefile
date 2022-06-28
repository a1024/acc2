SRC = $(wildcard src/*.c)
FLAGS = -g
UNAME_P =


ifeq ($(OS), Windows_NT)
	FLAGS += -no-pie
else
	UNAME_P = $(shell uname -p)
	ifeq ($(UNAME_P), unknown)#termux (arm/aarch64)
		FLAGS += -fPIE
	else
		FLAGS += -no-pie
	endif
endif


build:
	gcc $(FLAGS) $(SRC) -o acc -lm


test:
	gdb --args acc hello.c

#https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks
bughunt:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./acc hello.c


test-gcc:
	gcc -S -masm=intel hello.c -o hello-gcc.s

test-acc:
	./acc hello.c
