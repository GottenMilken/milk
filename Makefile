CC = cc

CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lm

TARGET = milk

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

SRC = \
	src/main.c \
	src/lexer.c \
	src/parser.c \
	src/ast.c \
	src/value.c \
	src/environment.c \
	src/evaluator.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all install uninstall clean
