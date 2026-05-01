CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = validara
SRC = main.c validara.c
TEST_SRC = test_validara.c validara.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC) validara.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TEST_SRC) validara.h
	$(CC) $(CFLAGS) -o test_runner $(TEST_SRC)
	./test_runner

clean:
	rm -f $(TARGET) test_runner *.o /tmp/validara-report.md
