UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CC = gcc
	CFLAGS = -lbsd -lncurses -Wall -Wextra -pedantic -lpthread
endif
ifeq ($(UNAME_S),Darwin)
	CC = clang
	CFLAGS = -Wall -Wextra -pedantic -lpthread
endif

default: darksync

darksync: darksync.c darksync.h aes.c aes.h
	$(CC) $(CFLAGS) -g -o darksync darksync.c darksync.h aes.c aes.h

