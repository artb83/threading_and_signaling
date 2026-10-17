#For ARM64 cross-compilation add 'CROSS_COMPILE = aarch64-none-linux-gnu-' when making aesdsocket.

TARGET  ?= workers
SRCS    ?= workers.c
HFILES  ?= workers.h
OBJS    ?= $(SRCS:.c=.o)
#INC_DIR ?= -I$(PWD)/../aesd-char-driver

ifeq ($(origin CC),default)
CC = $(CROSS_COMPILE)gcc
endif

CFLAGS += -g -Wall -Werror -Wextra -pthread
LDLIBS += -pthread

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

%.o:%.c $(HFILES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS)
