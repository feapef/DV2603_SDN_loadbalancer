CC = gcc
CFLAGS = -Wall
LDFLAGS =
OBJFILES = linked_list.o loadbalancer.o
TARGET = lb


all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
