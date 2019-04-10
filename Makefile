UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CC = gcc
	CFLAGS = -Wall -Wextra -pedantic -lpthread
endif
ifeq ($(UNAME_S),Darwin)
	CC = clang
	CFLAGS = -Wall -Wextra -pedantic -lpthread
endif

default: darkchat

darkchat: darkchat.c darkchat.h
	$(CC) $(CFLAGS) -g -o darkchat darkchat.c darkchat.h

