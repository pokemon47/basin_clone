CFLAGS =

ifneq (, $(shell which dcc))
CC	?= dcc
else
CC	?= clang
CFLAGS += -Wall
endif

EXERCISES	  += basin

SRC = basin.c basin_main.c basin_provided.c
INCLUDES = basin.h

# if you add extra .c files, add them here
SRC +=

# if you add extra .h files, add them here
INCLUDES +=


basin:	$(SRC) $(INCLUDES)
	$(CC) $(CFLAGS) $(SRC) -o $@
