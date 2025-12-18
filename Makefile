#
# Makefile for Microsoft 6502 BASIC C Port
# GNU Make for macOS and Linux
#

CC = cc
# Suppress warnings for K&R style code (required for 2.11 BSD compatibility)
CFLAGS = -O2 -Wall -Wextra -Wno-deprecated-non-prototype -Wno-unused-variable -Wno-unused-but-set-variable -Wno-self-assign
LDFLAGS = -lm

# Source files
SRCS = main.c error.c strings.c variables.c arrays.c \
       tokenize.c eval.c parse.c execute.c repl.c \
       functions.c statements.c

OBJS = $(SRCS:.c=.o)

TARGET = m6502basic

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c m6502basic.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

# Dependencies
main.o: main.c m6502basic.h
error.o: error.c m6502basic.h
strings.o: strings.c m6502basic.h
variables.o: variables.c m6502basic.h
arrays.o: arrays.c m6502basic.h
tokenize.o: tokenize.c m6502basic.h
eval.o: eval.c m6502basic.h
parse.o: parse.c m6502basic.h
execute.o: execute.c m6502basic.h
repl.o: repl.c m6502basic.h
functions.o: functions.c m6502basic.h
statements.o: statements.c m6502basic.h
