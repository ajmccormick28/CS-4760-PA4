#macros 

CC = gcc
CFLAGS = -g
TARGET = master
TARGET2 = palin
LIBS = 
SRC = main.c palin.c detachAndRemove.c
OBJFILEREAD = main.o detachAndRemove.o
OBJCHILD = palin.o

#explicit rule

all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJFILEREAD)
	$(CC) -o $(TARGET) $(OBJFILEREAD)

$(TARGET2): $(OBJCHILD)
	$(CC) -o $(TARGET2) $(OBJCHILD)

#implicit rules 

main.o: main.c optArg.h inputHold.h detachAndRemove.h
	$(CC) -c $(CFLAGS) main.c

user.o: palin.c inputHold.h
	$(CC) -c $(CFLAGS) user.c

detachAndRemove.o: detachAndRemove.c detachAndRemove.h
	$(CC) -c $(CFLAGS) detachAndRemove.c

#cleaning up

clean:
	/bin/rm -f *.o $(TARGET) $(TARGET2)
