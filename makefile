CC = gcc
CFLAGS = -Wall -g 
TARGET = main
SOURCES = main.c
LIBS = -lraylib -lm -ldl -lpthread

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET) *.o

.PHONY: run clean
