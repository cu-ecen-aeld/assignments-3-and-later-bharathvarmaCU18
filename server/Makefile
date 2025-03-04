# cross compile variable. Default value is nothing, can be overwritten on command line or in the Makefile
CROSS_COMPILE ?=

# compiler and compiler flags
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -g -Werror
LDFLAGS ?= -lpthread -lrt
EXEC ?= aesdsocket
SOURCEFILES = aesdsocket.c
OBJFILES = aesdsocket.o


# Automatic variables
# $@ is the target, $^ is all prerequisites or dependencies


all: $(EXEC)

$(EXEC): $(OBJFILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJFILES): $(SOURCEFILES)
	$(CC) $(CFLAGS) -c $^ -o $@ $(LDFLAGS)


# PHONY allows make to clean if there is a naming conflict due to a file named clean
.PHONY: clean
clean:
	rm -rf $(EXEC) $(OBJFILES)
