CC = cc

CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lm

TARGET = milk

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

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
