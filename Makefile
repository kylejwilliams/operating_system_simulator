# reference: http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC = gcc
CFLAGS = -I.
DEPS =  # *.h files go here
OBJ = oss.o # *.o files go here

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

oss: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

debug: *.c
	$(CC) -g $^

clean:
	rm -f *.o oss
