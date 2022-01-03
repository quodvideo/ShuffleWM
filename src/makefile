CFLAGS = -Wall

objects = shuffle.o decode.o atoms.o targets.o window.o manager.o root.o icon.o session.o moveresize.o

all: shuffle

shuffle: $(objects)
	cc -o shuffle $(objects) -lX11 -lXext

shuffle.o: shuffle.c

decode.o: decode.c

atoms.o: atoms.c

targets.o: targets.c

window.o: window.c

manager.o: manager.c

root.o: root.c

icon.o: icon.c

session.o: session.c

moveresize.o: moveresize.c

.PHONY: clean
clean:
	-rm -f shuffle $(objects)

