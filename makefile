CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -L/usr/local/lib64
TARGET = main
SOURCES = main.c
LIBS = -lraylib -lX11 -lXrandr -lXinerama -lXi -lXcursor -lGL -lopenal -lrt -lm -ldl -lpthread

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

clean:
	rm -f $(TARGET) *.o

.PHONY: run clean
