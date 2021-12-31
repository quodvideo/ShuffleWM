CFLAGS = -Wall

objects = demo.o

all: demo

demo: $(objects)
	cc -o demo $(objects) -lX11 -lXext

demo.o: demo.c 

.PHONY: clean
clean:
	-rm -f demo $(objects)

